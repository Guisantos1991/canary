#pragma once

#include <algorithm>
#include <cstdint>

#include "wizard/progression/wizard_progression_config.hpp"

enum class WizardSkill : uint8_t {
	MAGICAL_POWER,
	MAGICAL_CONTROL,
	MAGICAL_KNOWLEDGE,
	SKILL_COMBAT
};

struct WizardSkillSnapshot {
	uint16_t magicalPower = 1;
	uint16_t magicalControl = 1;
	uint16_t magicalKnowledge = 1;
	uint16_t skillCombat = 1;

	[[nodiscard]] static uint16_t normalize(const int32_t value) {
		const auto &limits = g_wizardProgression().get().skills;
		return static_cast<uint16_t>(std::clamp<int32_t>(value, limits.min, limits.max));
	}

	[[nodiscard]] uint16_t getMagicalPower() const {
		return normalize(magicalPower);
	}

	[[nodiscard]] uint16_t getMagicalControl() const {
		return normalize(magicalControl);
	}

	[[nodiscard]] uint16_t getMagicalKnowledge() const {
		return normalize(magicalKnowledge);
	}

	[[nodiscard]] uint16_t getSkillCombat() const {
		return normalize(skillCombat);
	}

	[[nodiscard]] uint16_t get(WizardSkill skill) const {
		switch (skill) {
			case WizardSkill::MAGICAL_POWER:
				return getMagicalPower();
			case WizardSkill::MAGICAL_CONTROL:
				return getMagicalControl();
			case WizardSkill::MAGICAL_KNOWLEDGE:
				return getMagicalKnowledge();
			case WizardSkill::SKILL_COMBAT:
				return getSkillCombat();
		}
		return 1;
	}

	void set(WizardSkill skill, int32_t value) {
		const auto normalized = normalize(value);
		switch (skill) {
			case WizardSkill::MAGICAL_POWER:
				magicalPower = normalized;
				break;
			case WizardSkill::MAGICAL_CONTROL:
				magicalControl = normalized;
				break;
			case WizardSkill::MAGICAL_KNOWLEDGE:
				magicalKnowledge = normalized;
				break;
			case WizardSkill::SKILL_COMBAT:
				skillCombat = normalized;
				break;
		}
	}
};

struct WizardSpellProgress {
	uint16_t knowledge = 0;
	uint16_t mastery = 0;
	bool learned = false;
	uint64_t uses = 0;
};
