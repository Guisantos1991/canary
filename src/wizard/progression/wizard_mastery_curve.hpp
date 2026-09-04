#pragma once

#include "wizard/progression/wizard_progression_config.hpp"

#include <cstdint>

class WizardMasteryCurve {
public:
	[[nodiscard]] static uint64_t calculateThreshold(uint16_t level, const WizardMasteryCurveConfig &config);
	[[nodiscard]] static uint16_t calculateLevel(uint64_t xp, const WizardMasteryCurveConfig &config);
	[[nodiscard]] static uint64_t clampXp(uint64_t xp, const WizardMasteryCurveConfig &config);
};
