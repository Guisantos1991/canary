# Wizard Potions Foundation

## Modelo

Receitas possuem progressão própria e não reutilizam Spell Mastery:

```text
Recipe Knowledge -> Learnability -> Learning -> Valid Brew
-> Quality/Effects -> Brews + Brewing XP -> Brewing Mastery
```

Cada linha de `player_wizard_recipes` armazena:

```text
player_id, recipe_id, knowledge, learned, mastery, mastery_xp,
knowledge_sources, brews
```

A chave primária composta impede duplicidades. Knowledge suficiente não aprende automaticamente. Brewing inválido não aumenta `brews` nem concede XP.

## Definição data-driven

`data/wizard/potions.json` usa o schema:

```json
{
  "id": 7001,
  "name": "Development Vitality Draught",
  "developmentFixture": true,
  "progression": {
    "knowledgeRequired": 100,
    "magicalKnowledgeRequired": 10,
    "acquisitionProfile": "HYBRID",
    "allowedKnowledgeSources": ["READING", "EXPLORATION", "EXPERIMENTATION", "ADMIN", "SYSTEM"],
    "requiredKnowledgeSources": []
  },
  "ingredients": [{ "itemId": 266, "amount": 1 }],
  "baseQuality": 20.0,
  "baseEffects": {
    "potency": 100.0,
    "durationMs": 60000,
    "yield": 1,
    "stability": 50.0
  },
  "linkedSpellId": null
}
```

A única receita atual é fixture explícita de desenvolvimento, não conteúdo ou lore final. Receitas podem ser reading-only, exploration-only ou híbridas. `EXPERIMENTATION` já existe como source de receita para integração futura, sem implementar o minigame nesta sprint.

## Serviços

`WizardRecipeKnowledgeSystem` valida source, aplica clamp e registra source mask sem alterar learning ou mastery. `WizardRecipeLearningSystem` verifica recipe knowledge, Magical Knowledge e sources obrigatórios, retornando resultado explícito. `WizardBrewingMasterySystem` mantém XP/nível da receita em `0..100` por uma curva própria.

A curva inicial de brewing tem thresholds cumulativos de 800 XP no nível 20, 3.800 no 50 e 27.550 no 100. Um brew válido rende inicialmente 10 XP, sem cooldown temporal; ingredientes são o limitador econômico.

## Contrato de ingredientes

O core aceita:

```cpp
IngredientInput {
  itemId,
  amount,
  quality // 0..100
}
```

O conjunto e as quantidades devem corresponder exatamente à definição. O valor default configurável (`50`) está pronto para adapters de itens que ainda não forneçam metadata de qualidade. Consumo de inventário e estação de crafting permanecem responsabilidade da futura camada de integração; o core desta sprint valida e calcula o resultado.

## Quality

A fórmula é determinística:

```text
ingredientAverage = sum(quality * amount) / sum(amount)
quality = floor(clamp(
  baseQuality
  + ingredientAverage * ingredientWeight
  + control/100 * controlMaxBonus
  + brewingMastery/100 * masteryMaxBonus,
  qualityMin,
  qualityMax
))
```

Configuração inicial: peso de ingrediente 0,50, bônus máximo de Control 15 pontos, bônus máximo de Brewing Mastery 15 pontos e resultado `0..100`. Não há RNG.

## Efeitos

Potency, duration e stability usam um multiplicador limitado:

```text
bonus = min(
  quality/100 * qualityMaxBonus
  + control/100 * controlMaxBonus
  + brewingMastery/100 * masteryMaxBonus
  + linkedSpellBonus,
  maxCombinedBonus
)
multiplier = 1 + max(0, bonus)
```

Os caps iniciais são 15% de quality, 5% de Control, 10% de Brewing Mastery, 5% de linked spell e 25% combinados. Yield permanece no valor base nesta fundação. Stability é limitada a 100.

Receitas comuns ignoram integralmente Spell Mastery. Se `linkedSpellId` for declarado no futuro, apenas a mastery daquela magia específica poderá contribuir; o registry rejeita links para spells inexistentes.

## Falhas e extensão

Nesta sprint, recipe inexistente, recipe não aprendida ou ingredientes inválidos resultam em falha sem XP e sem incremento de brews. A estrutura de resultados permite adicionar menor qualidade, yield reduzido ou perda parcial no futuro sem exigir falha binária punitiva. Interface de caldeirão, animação, consumo real e catálogo final ficam fora do escopo.

## Comandos

Jogador:

- `!wlearnrecipe <recipe>` tenta aprender;
- `!wrecipeprogress <recipe>` consulta progresso.

GOD:

- `/wrecipeknowledge <player>, <recipe>, <value>`;
- `/wrecipemastery <player>, <recipe>, <value>`;
- `/wrecipeinfo <player>, <recipe>`;
- `/wbrewtest <player>, <recipe>` executa somente o cálculo determinístico da fixture com ingredient quality configurada, sem substituir a futura estação.

## Persistência

A migration 61 cria `player_wizard_recipes` com defaults seguros e chave primária `(player_id, recipe_id)`. Save/load faz round-trip de knowledge, learned, mastery, mastery XP, source mask e brews. Discovery visual e disponibilidade de objetos continuarão sendo estado por jogador na Sprint 4, separados desta tabela de progresso da receita.
