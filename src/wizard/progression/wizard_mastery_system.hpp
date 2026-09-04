#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

class Player;

class WizardMasterySystem {
public:
	static uint16_t addXp(const std::shared_ptr<Player> &player, uint32_t spellId, uint64_t xp);
	static uint16_t grantMeaningfulUse(const std::shared_ptr<Player> &player, uint32_t spellId, std::size_t affectedTargets);
	static bool setMastery(const std::shared_ptr<Player> &player, uint32_t spellId, int32_t level);
	[[nodiscard]] static uint16_t calculateLevel(uint64_t xp);
	[[nodiscard]] static uint64_t calculateThreshold(uint16_t level);
	[[nodiscard]] static uint64_t calculateMeaningfulUseXp(std::size_t affectedTargets);
};
