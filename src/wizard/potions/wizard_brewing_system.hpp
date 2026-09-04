#pragma once

#include "wizard/potions/wizard_potion_definition.hpp"

#include <cstdint>
#include <memory>
#include <vector>

class Player;

struct WizardIngredientInput {
	uint16_t itemId = 0;
	uint16_t amount = 0;
	uint16_t quality = 0;
};

struct WizardPotionEffectResult {
	double potency = 0;
	uint32_t durationMs = 0;
	uint16_t yield = 0;
	double stability = 0;
};

enum class WizardBrewResultCode : uint8_t {
	RECIPE_NOT_FOUND,
	RECIPE_NOT_LEARNED,
	INVALID_INGREDIENTS,
	SUCCESS,
};

struct WizardBrewResult {
	WizardBrewResultCode code = WizardBrewResultCode::RECIPE_NOT_FOUND;
	uint16_t quality = 0;
	WizardPotionEffectResult effects;
	uint64_t xpGranted = 0;
};

class WizardBrewingSystem {
public:
	[[nodiscard]] static uint16_t calculateQuality(const WizardPotionDefinition &recipe, const std::vector<WizardIngredientInput> &ingredients, uint16_t magicalControl, uint16_t brewingMastery);
	[[nodiscard]] static WizardPotionEffectResult calculateEffects(const WizardPotionDefinition &recipe, uint16_t quality, uint16_t magicalControl, uint16_t brewingMastery, uint16_t linkedSpellMastery = 0);
	[[nodiscard]] static bool validateIngredients(const WizardPotionDefinition &recipe, const std::vector<WizardIngredientInput> &ingredients);
	[[nodiscard]] static WizardBrewResult brew(const std::shared_ptr<Player> &player, uint32_t recipeId, const std::vector<WizardIngredientInput> &ingredients);
};
