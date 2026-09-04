#include "wizard/progression/wizard_learning_system.hpp"

#include "creatures/players/player.hpp"
#include "wizard/spells/wizard_spell_registry.hpp"

WizardLearningResult WizardLearningSystem::check(const std::shared_ptr<Player> &player, const uint32_t spellId) {
	const auto* spell = g_wizardSpells().getById(spellId);
	if (!player || !spell) return WizardLearningResult::SPELL_NOT_FOUND;
	const auto* progress = player->getWizardSpellProgress(spellId);
	if (progress && progress->learned) return WizardLearningResult::ALREADY_LEARNED;
	if (!spell->learnable) return WizardLearningResult::NOT_LEARNABLE;
	if (!progress || progress->knowledge < spell->progression.knowledgeRequired) return WizardLearningResult::INSUFFICIENT_SPELL_KNOWLEDGE;
	if (player->getWizardSkills().getMagicalKnowledge() < spell->progression.magicalKnowledgeRequired) return WizardLearningResult::INSUFFICIENT_MAGICAL_KNOWLEDGE;
	if ((progress->knowledgeSources & spell->progression.requiredKnowledgeSources) != spell->progression.requiredKnowledgeSources) return WizardLearningResult::SOURCE_REQUIREMENT_NOT_MET;
	return WizardLearningResult::SUCCESS;
}

WizardLearningResult WizardLearningSystem::learn(const std::shared_ptr<Player> &player, const uint32_t spellId) {
	const auto result = check(player, spellId);
	if (result == WizardLearningResult::SUCCESS) player->getOrCreateWizardSpellProgress(spellId).learned = true;
	return result;
}

bool WizardLearningSystem::isLearnable(const std::shared_ptr<Player> &player, const uint32_t spellId) {
	return check(player, spellId) == WizardLearningResult::SUCCESS;
}
