#pragma once

#include <cstdint>
#include <memory>

class Player;

class WizardBrewingMasterySystem {
public:
	static uint16_t addXp(const std::shared_ptr<Player> &player, uint32_t recipeId, uint64_t xp);
	static bool setMastery(const std::shared_ptr<Player> &player, uint32_t recipeId, int32_t level);
	[[nodiscard]] static uint16_t calculateLevel(uint64_t xp);
	[[nodiscard]] static uint64_t calculateThreshold(uint16_t level);
};
