#pragma once

#include "wizard/progression/wizard_progression_config.hpp"

#include <cstdint>

class WizardExhaustionSystem {
public:
	[[nodiscard]] static uint32_t calculateRecovery(
		uint32_t spellRecoveryMs,
		uint16_t magicalControl,
		uint16_t mastery,
		const WizardRecoveryConfig &config = {}
	);

	[[nodiscard]] static uint32_t calculateCastTime(
		uint32_t baseCastTimeMs,
		uint16_t skillCombat,
		uint16_t mastery,
		const WizardCastConfig &config = {}
	);
};
