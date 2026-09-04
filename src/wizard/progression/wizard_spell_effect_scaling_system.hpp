#pragma once

#include "wizard/progression/wizard_progression_config.hpp"

#include <cstdint>

class WizardSpellEffectScalingSystem {
public:
	[[nodiscard]] static double calculatePotencyMultiplier(uint16_t magicalControl, uint16_t mastery, const WizardSpellEffectConfig &config = {});
	[[nodiscard]] static int32_t scalePotency(int32_t baseValue, uint16_t magicalControl, uint16_t mastery, const WizardSpellEffectConfig &config = {});
};
