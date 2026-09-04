# Wizard Progression

## Contrato de domínio

A progressão Wizard é formada por quatro contextos independentes e complementares:

- **READING** entrega Magical Knowledge global, Spell Knowledge, Recipe Knowledge e informações.
- **EXPLORATION** entrega descobertas e conhecimento que pode não existir em livros.
- **COMBAT** desenvolve Skill Combat e Spell Mastery depois que a magia foi aprendida.
- **BREWING** desenvolve a maestria específica de cada receita.

Não existe cooldown de XP de maestria, descanso obrigatório ou cronômetro que force alternância de atividade. O balanceamento de longo prazo vem dos requisitos e das fontes permitidas pelo conteúdo.

## Skills globais e progresso por magia

As skills globais continuam sendo `MAGICAL_POWER`, `MAGICAL_CONTROL`, `MAGICAL_KNOWLEDGE` e `SKILL_COMBAT`, todas em `1..100`.

Cada linha de `player_wizard_spells` armazena:

```text
player_id, spell_id, knowledge, learned, mastery, mastery_xp,
knowledge_sources, uses
```

`MAGICAL_KNOWLEDGE`, `knowledge` da magia e `mastery` são domínios distintos. Alterar knowledge nunca altera mastery, XP, uses ou learned. Knowledge suficiente apenas torna a magia aprendível; `WizardLearningSystem` é a única transição normal para `learned=true`.

Os estados de apresentação são derivados:

- `UNKNOWN`: knowledge zero e não aprendida;
- `DISCOVERED`: knowledge parcial e não aprendida;
- `LEARNABLE`: todos os requisitos atendidos e não aprendida;
- `LEARNED`: learned verdadeiro;
- `MASTERED`: learned verdadeiro e mastery no máximo.

## Definição data-driven

Cada entrada de `data/wizard/spells.json` contém:

```json
{
  "progression": {
    "knowledgeRequired": 100,
    "magicalKnowledgeRequired": 20,
    "acquisitionProfile": "HYBRID",
    "allowedKnowledgeSources": ["READING", "EXPLORATION", "ADMIN", "SYSTEM"],
    "requiredKnowledgeSources": []
  }
}
```

Perfis válidos: `ACADEMIC`, `EXPLORATION`, `HYBRID`, `SECRET` e `FORBIDDEN`. Sources de spell válidos: `READING`, `EXPLORATION`, `ADMIN` e `SYSTEM`. `COMBAT` deliberadamente não é source de knowledge. Uma magia exploration-only omite `READING`; uma magia acadêmica omite `EXPLORATION`; uma híbrida permite ambas. `requiredKnowledgeSources` registra requisitos históricos adicionais sem aprender automaticamente.

Ignis usa metadata de desenvolvimento nesta sprint e não estabelece lore final.

## Knowledge e Learning

`WizardKnowledgeSystem` expõe `addKnowledge`, `setKnowledge`, `getKnowledge` e validação de source. O valor é limitado por `progression.json`; o bitmask `knowledge_sources` registra as origens que efetivamente contribuíram. A API é o ponto de conexão da Sprint 4 para livros, scrolls, placas, objetos, locais, observação e investigação.

`WizardLearningSystem` retorna códigos de domínio, não mensagens: spell inexistente, já aprendida, não aprendível, knowledge insuficiente, Magical Knowledge insuficiente, source obrigatório ausente ou sucesso. A camada Lua converte esses códigos em texto para `!wlearn`.

O comando administrativo `/wlearn <player>, <spell>` continua sendo um bypass explícito e separado do fluxo do jogador.

## Spell Mastery

`WizardMasterySystem` usa XP cumulativo e uma curva em bandas. O threshold de um nível é a soma de `níveis_da_banda * xpPerLevel` até o nível desejado. A configuração inicial resulta em:

| Nível | XP cumulativo |
| ---: | ---: |
| 20 | 1.000 |
| 50 | 4.750 |
| 80 | 13.750 |
| 95 | 25.000 |
| 100 | 32.500 |

O setter administrativo de mastery sincroniza `mastery_xp` com o threshold do nível. XP e nível são saturados no máximo.

### Uso significativo

`uses` aumenta quando um cast aceito é efetivamente processado e consome seus recursos, mesmo que o tile esteja vazio ou todos desviem. Mastery XP só é concedido no impacto quando ao menos um alvo válido sofre alteração real de vida.

```text
xpPorCast = baseXp + min(alvosAfetados - 1, bonusTargetCap) * additionalTargetBonus
```

Com os valores iniciais, 1 alvo rende 10 XP, 2 rendem 12 XP e 4 ou mais rendem no máximo 16 XP. A concessão acontece uma vez depois do loop de criaturas. Tile vazio, dodge total, PZ, LOS, range, mana, recovery ou qualquer rejeição rendem zero. Não há throttle por tempo: usos válidos consecutivos continuam progredindo.

## Scaling de execução

O dano base continua sendo escalado por Magical Power. Depois disso, `WizardSpellEffectScalingSystem` aplica:

```text
bonus = min(
  control/100 * controlMaxPotencyBonus
  + mastery/100 * masteryMaxPotencyBonus,
  maxCombinedPotencyBonus
)
potency = floor(basePowerScaledEffect * (1 + bonus))
```

Os caps iniciais são 10% de Control, 15% de Mastery e 20% combinados. Power continua sendo o único responsável pela área progressiva; Mastery e Control não alteram Position, aim, tracking, `maxSquares` ou trajetória.

Mana usa `ceil(baseCost * (1 - capped(controlReduction + masteryReduction)))`, com mínimo absoluto 1. Recovery usa a mesma composição limitada e nunca fica abaixo de `minimumMs`. Skill Combat e Mastery refinam cast time dentro do cap existente, sem autoaim.

## Configuração e startup

`data/wizard/progression.json` centraliza limites, curvas, XP por uso, mana, recovery e caps de efeitos. O loader rejeita limites invertidos, XP não positivo, bandas vazias/sobrepostas/não progressivas, curva sem cobertura até o máximo e caps negativos ou fora do domínio. Falha de configuração impede o carregamento do Wizard Magic com erro explícito.

## Persistência e migração

A migration 61 adiciona `mastery_xp BIGINT UNSIGNED NOT NULL DEFAULT 0` e `knowledge_sources SMALLINT UNSIGNED NOT NULL DEFAULT 0`. Para registros antigos com mastery, o backfill calcula o XP cumulativo correspondente à curva inicial da Sprint 3; assim nenhum nível existente é perdido. Novos saves persistem knowledge, learned, mastery, mastery XP, source mask e uses.

## Comandos

Jogador:

- `!wlearn <spell>` tenta o aprendizado real;
- `!wprogress <spell>` consulta requisitos, learnability, learned, mastery, XP e uses.

GOD:

- `/wskill <player>, <skill>, <value>`;
- `/wlearn <player>, <spell>`;
- `/wknowledge <player>, <spell>, <value>`;
- `/wmastery <player>, <spell>, <value>`;
- `/wspellinfo <player>, <spell>`.

## Extensão para discovery

Fontes concretas não são implementadas nesta sprint. A Sprint 4 poderá chamar o serviço central e persistir estado de discovery por jogador. Objetos pessoais ou temporários podem, portanto, desaparecer para um jogador sem afetar a disponibilidade para outros, sem acoplar essa regra ao registry de spells.
