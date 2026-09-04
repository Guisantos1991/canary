#include "wizard/progression/wizard_knowledge_system.hpp"

#include "creatures/players/player.hpp"
#include "wizard/progression/wizard_progression_config.hpp"
#include "wizard/spells/wizard_spell_registry.hpp"

#include <algorithm>

bool WizardKnowledgeSystem::isSourceAllowed(const uint32_t spellId, const WizardKnowledgeSource source) {
	const auto* spell = g_wizardSpells().getById(spellId);
	return spell && wizardHasKnowledgeSource(spell->progression.allowedKnowledgeSources, source);
}

WizardKnowledgeResult WizardKnowledgeSystem::setKnowledge(const std::shared_ptr<Player> &player, const uint32_t spellId, const int32_t value, const WizardKnowledgeSource source) {
	if (!player || !g_wizardSpells().getById(spellId)) return WizardKnowledgeResult::SPELL_NOT_FOUND;
	if (!isSourceAllowed(spellId, source)) return WizardKnowledgeResult::SOURCE_NOT_ALLOWED;
	const auto &limits = g_wizardProgression().get().knowledge;
	auto &progress = player->getOrCreateWizardSpellProgress(spellId);
	progress.knowledge = static_cast<uint16_t>(std::clamp<int32_t>(value, limits.min, limits.max));
	if (progress.knowledge == 0) progress.knowledgeSources = 0;
	else progress.knowledgeSources |= wizardKnowledgeSourceBit(source);
	return WizardKnowledgeResult::SUCCESS;
}

WizardKnowledgeResult WizardKnowledgeSystem::addKnowledge(const std::shared_ptr<Player> &player, const uint32_t spellId, const int32_t amount, const WizardKnowledgeSource source) {
	if (!player || !g_wizardSpells().getById(spellId)) return WizardKnowledgeResult::SPELL_NOT_FOUND;
	if (!isSourceAllowed(spellId, source)) return WizardKnowledgeResult::SOURCE_NOT_ALLOWED;
	const auto &limits = g_wizardProgression().get().knowledge;
	const auto value = std::clamp<int64_t>(static_cast<int64_t>(getKnowledge(player, spellId)) + amount, limits.min, limits.max);
	return setKnowledge(player, spellId, static_cast<int32_t>(value), source);
}

uint16_t WizardKnowledgeSystem::getKnowledge(const std::shared_ptr<Player> &player, const uint32_t spellId) {
	const auto* progress = player ? player->getWizardSpellProgress(spellId) : nullptr;
	return progress ? progress->knowledge : 0;
}
