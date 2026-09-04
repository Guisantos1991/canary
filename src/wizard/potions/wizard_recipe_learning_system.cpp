#include "wizard/potions/wizard_recipe_learning_system.hpp"

#include "creatures/players/player.hpp"
#include "wizard/potions/wizard_potion_registry.hpp"

WizardRecipeLearningResult WizardRecipeLearningSystem::check(const std::shared_ptr<Player> &player, const uint32_t recipeId) {
	const auto* recipe = g_wizardPotions().getById(recipeId);
	if (!player || !recipe) return WizardRecipeLearningResult::RECIPE_NOT_FOUND;
	const auto* progress = player->getWizardRecipeProgress(recipeId);
	if (progress && progress->learned) return WizardRecipeLearningResult::ALREADY_LEARNED;
	if (!progress || progress->knowledge < recipe->progression.knowledgeRequired) return WizardRecipeLearningResult::INSUFFICIENT_RECIPE_KNOWLEDGE;
	if (player->getWizardSkills().getMagicalKnowledge() < recipe->progression.magicalKnowledgeRequired) return WizardRecipeLearningResult::INSUFFICIENT_MAGICAL_KNOWLEDGE;
	if ((progress->knowledgeSources & recipe->progression.requiredKnowledgeSources) != recipe->progression.requiredKnowledgeSources) return WizardRecipeLearningResult::SOURCE_REQUIREMENT_NOT_MET;
	return WizardRecipeLearningResult::SUCCESS;
}

WizardRecipeLearningResult WizardRecipeLearningSystem::learn(const std::shared_ptr<Player> &player, const uint32_t recipeId) {
	const auto result = check(player, recipeId);
	if (result == WizardRecipeLearningResult::SUCCESS) player->getOrCreateWizardRecipeProgress(recipeId).learned = true;
	return result;
}

bool WizardRecipeLearningSystem::isLearnable(const std::shared_ptr<Player> &player, const uint32_t recipeId) {
	return check(player, recipeId) == WizardRecipeLearningResult::SUCCESS;
}
