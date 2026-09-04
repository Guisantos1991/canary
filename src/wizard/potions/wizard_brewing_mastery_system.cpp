#include "wizard/potions/wizard_brewing_mastery_system.hpp"

#include "creatures/players/player.hpp"
#include "wizard/potions/wizard_potion_registry.hpp"
#include "wizard/progression/wizard_mastery_curve.hpp"
#include "wizard/progression/wizard_progression_config.hpp"

#include <algorithm>

uint64_t WizardBrewingMasterySystem::calculateThreshold(const uint16_t level) {
	return WizardMasteryCurve::calculateThreshold(level, g_wizardProgression().get().brewingMastery);
}

uint16_t WizardBrewingMasterySystem::calculateLevel(const uint64_t xp) {
	return WizardMasteryCurve::calculateLevel(xp, g_wizardProgression().get().brewingMastery);
}

uint16_t WizardBrewingMasterySystem::addXp(const std::shared_ptr<Player> &player, const uint32_t recipeId, const uint64_t xp) {
	if (!player || !g_wizardPotions().getById(recipeId) || !player->hasLearnedWizardRecipe(recipeId) || xp == 0) return 0;
	auto &progress = player->getOrCreateWizardRecipeProgress(recipeId);
	const auto maximum = calculateThreshold(g_wizardProgression().get().brewingMastery.limits.max);
	progress.masteryXp = xp > maximum - std::min(progress.masteryXp, maximum) ? maximum : progress.masteryXp + xp;
	progress.mastery = calculateLevel(progress.masteryXp);
	return progress.mastery;
}

bool WizardBrewingMasterySystem::setMastery(const std::shared_ptr<Player> &player, const uint32_t recipeId, const int32_t level) {
	if (!player || !g_wizardPotions().getById(recipeId)) return false;
	const auto &limits = g_wizardProgression().get().brewingMastery.limits;
	auto &progress = player->getOrCreateWizardRecipeProgress(recipeId);
	progress.mastery = static_cast<uint16_t>(std::clamp<int32_t>(level, limits.min, limits.max));
	progress.masteryXp = calculateThreshold(progress.mastery);
	return true;
}
