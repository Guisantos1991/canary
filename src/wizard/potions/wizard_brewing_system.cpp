#include "wizard/potions/wizard_brewing_system.hpp"

#include "creatures/players/player.hpp"
#include "wizard/potions/wizard_brewing_mastery_system.hpp"
#include "wizard/potions/wizard_potion_registry.hpp"
#include "wizard/progression/wizard_progression_config.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

bool WizardBrewingSystem::validateIngredients(const WizardPotionDefinition &recipe, const std::vector<WizardIngredientInput> &ingredients) {
	if (ingredients.size() != recipe.ingredients.size()) return false;
	std::unordered_map<uint16_t, uint32_t> supplied;
	for (const auto &ingredient : ingredients) {
		if (ingredient.itemId == 0 || ingredient.amount == 0 || ingredient.quality > 100) return false;
		supplied[ingredient.itemId] += ingredient.amount;
	}
	for (const auto &required : recipe.ingredients) {
		const auto found = supplied.find(required.itemId);
		if (found == supplied.end() || found->second != required.amount) return false;
	}
	return true;
}

uint16_t WizardBrewingSystem::calculateQuality(const WizardPotionDefinition &recipe, const std::vector<WizardIngredientInput> &ingredients, const uint16_t magicalControl, const uint16_t brewingMastery) {
	if (!validateIngredients(recipe, ingredients)) return 0;
	uint64_t weightedQuality = 0;
	uint64_t totalAmount = 0;
	for (const auto &ingredient : ingredients) {
		weightedQuality += static_cast<uint64_t>(ingredient.quality) * ingredient.amount;
		totalAmount += ingredient.amount;
	}
	const auto &config = g_wizardProgression().get().potionQuality;
	const double ingredientAverage = totalAmount == 0 ? 0 : static_cast<double>(weightedQuality) / static_cast<double>(totalAmount);
	const double quality = recipe.baseQuality
		+ ingredientAverage * config.ingredientWeight
		+ (static_cast<double>(std::clamp<uint16_t>(magicalControl, 1, 100)) / 100.0) * config.controlMaxBonus
		+ (static_cast<double>(std::clamp<uint16_t>(brewingMastery, 0, 100)) / 100.0) * config.masteryMaxBonus;
	return static_cast<uint16_t>(std::clamp<double>(std::floor(quality), config.min, config.max));
}

WizardPotionEffectResult WizardBrewingSystem::calculateEffects(const WizardPotionDefinition &recipe, const uint16_t quality, const uint16_t magicalControl, const uint16_t brewingMastery, const uint16_t linkedSpellMastery) {
	const auto &config = g_wizardProgression().get().potionEffect;
	const double linkedBonus = recipe.linkedSpellId ? (static_cast<double>(std::clamp<uint16_t>(linkedSpellMastery, 0, 100)) / 100.0) * config.linkedSpellMaxBonus : 0.0;
	const double bonus = std::min(
		(static_cast<double>(std::clamp<uint16_t>(quality, 0, 100)) / 100.0) * config.qualityMaxBonus
			+ (static_cast<double>(std::clamp<uint16_t>(magicalControl, 1, 100)) / 100.0) * config.controlMaxBonus
			+ (static_cast<double>(std::clamp<uint16_t>(brewingMastery, 0, 100)) / 100.0) * config.masteryMaxBonus
			+ linkedBonus,
		config.maxCombinedBonus
	);
	const double multiplier = 1.0 + std::max(0.0, bonus);
	WizardPotionEffectResult result;
	result.potency = recipe.baseEffects.potency * multiplier;
	result.durationMs = static_cast<uint32_t>(std::min<double>(std::floor(static_cast<double>(recipe.baseEffects.durationMs) * multiplier), std::numeric_limits<uint32_t>::max()));
	result.yield = recipe.baseEffects.yield;
	result.stability = std::min(100.0, recipe.baseEffects.stability * multiplier);
	return result;
}

WizardBrewResult WizardBrewingSystem::brew(const std::shared_ptr<Player> &player, const uint32_t recipeId, const std::vector<WizardIngredientInput> &ingredients) {
	WizardBrewResult result;
	const auto* recipe = g_wizardPotions().getById(recipeId);
	if (!player || !recipe) return result;
	if (!player->hasLearnedWizardRecipe(recipeId)) { result.code = WizardBrewResultCode::RECIPE_NOT_LEARNED; return result; }
	if (!validateIngredients(*recipe, ingredients)) { result.code = WizardBrewResultCode::INVALID_INGREDIENTS; return result; }
	auto &progress = player->getOrCreateWizardRecipeProgress(recipeId);
	result.quality = calculateQuality(*recipe, ingredients, player->getWizardSkills().getMagicalControl(), progress.mastery);
	uint16_t linkedMastery = 0;
	if (recipe->linkedSpellId) {
		const auto* linkedProgress = player->getWizardSpellProgress(*recipe->linkedSpellId);
		linkedMastery = linkedProgress ? linkedProgress->mastery : 0;
	}
	result.effects = calculateEffects(*recipe, result.quality, player->getWizardSkills().getMagicalControl(), progress.mastery, linkedMastery);
	++progress.brews;
	result.xpGranted = g_wizardProgression().get().brewing.xpPerValidBrew;
	WizardBrewingMasterySystem::addXp(player, recipeId, result.xpGranted);
	result.code = WizardBrewResultCode::SUCCESS;
	return result;
}
