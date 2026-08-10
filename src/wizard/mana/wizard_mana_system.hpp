#pragma once

#include "wizard/skills/wizard_skill.hpp"
#include "wizard/progression/wizard_progression_config.hpp"

#include <cstdint>

class WizardManaSystem {
public:
	[[nodiscard]] static uint32_t calculateMaxMana(
		const WizardSkillSnapshot &skills,
		const WizardManaConfig &config = {}
	);

	[[nodiscard]] static uint32_t calculateSpellManaCost(
		uint32_t baseManaCost,
		uint16_t magicalControl,
		uint16_t mastery,
		const WizardManaConfig &config = {}
	);
};
