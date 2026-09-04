#include "wizard/potions/wizard_recipe_knowledge_system.hpp"

#include "creatures/players/player.hpp"
#include "wizard/potions/wizard_potion_registry.hpp"
#include "wizard/progression/wizard_progression_config.hpp"

#include <algorithm>

bool WizardRecipeKnowledgeSystem::isSourceAllowed(const uint32_t recipeId, const WizardKnowledgeSource source) {
	const auto* recipe = g_wizardPotions().getById(recipeId);
	return recipe && wizardHasKnowledgeSource(recipe->progression.allowedKnowledgeSources, source);
}

WizardRecipeKnowledgeResult WizardRecipeKnowledgeSystem::setKnowledge(const std::shared_ptr<Player> &player, const uint32_t recipeId, const int32_t value, const WizardKnowledgeSource source) {
	if (!player || !g_wizardPotions().getById(recipeId)) return WizardRecipeKnowledgeResult::RECIPE_NOT_FOUND;
	if (!isSourceAllowed(recipeId, source)) return WizardRecipeKnowledgeResult::SOURCE_NOT_ALLOWED;
	const auto &limits = g_wizardProgression().get().recipeKnowledge;
	auto &progress = player->getOrCreateWizardRecipeProgress(recipeId);
	progress.knowledge = static_cast<uint16_t>(std::clamp<int32_t>(value, limits.min, limits.max));
	if (progress.knowledge == 0) progress.knowledgeSources = 0;
	else progress.knowledgeSources |= wizardKnowledgeSourceBit(source);
	return WizardRecipeKnowledgeResult::SUCCESS;
}

WizardRecipeKnowledgeResult WizardRecipeKnowledgeSystem::addKnowledge(const std::shared_ptr<Player> &player, const uint32_t recipeId, const int32_t amount, const WizardKnowledgeSource source) {
	if (!player || !g_wizardPotions().getById(recipeId)) return WizardRecipeKnowledgeResult::RECIPE_NOT_FOUND;
	if (!isSourceAllowed(recipeId, source)) return WizardRecipeKnowledgeResult::SOURCE_NOT_ALLOWED;
	const auto &limits = g_wizardProgression().get().recipeKnowledge;
	const auto value = std::clamp<int64_t>(static_cast<int64_t>(getKnowledge(player, recipeId)) + amount, limits.min, limits.max);
	return setKnowledge(player, recipeId, static_cast<int32_t>(value), source);
}

uint16_t WizardRecipeKnowledgeSystem::getKnowledge(const std::shared_ptr<Player> &player, const uint32_t recipeId) {
	const auto* progress = player ? player->getWizardRecipeProgress(recipeId) : nullptr;
	return progress ? progress->knowledge : 0;
}
