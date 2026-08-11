#pragma once

#include <algorithm>
#include <cstdint>

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

	[[nodiscard]] uint16_t getMagicalPower() const {
		return std::clamp<uint16_t>(magicalPower, 1, 100);
	}

	[[nodiscard]] uint16_t getMagicalControl() const {
		return std::clamp<uint16_t>(magicalControl, 1, 100);
	}

	[[nodiscard]] uint16_t getMagicalKnowledge() const {
		return std::clamp<uint16_t>(magicalKnowledge, 1, 100);
	}

	[[nodiscard]] uint16_t getSkillCombat() const {
		return std::clamp<uint16_t>(skillCombat, 1, 100);
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

	void set(WizardSkill skill, uint16_t value) {
		value = std::clamp<uint16_t>(value, 1, 100);
		switch (skill) {
			case WizardSkill::MAGICAL_POWER:
				magicalPower = value;
				break;
			case WizardSkill::MAGICAL_CONTROL:
				magicalControl = value;
				break;
			case WizardSkill::MAGICAL_KNOWLEDGE:
				magicalKnowledge = value;
				break;
			case WizardSkill::SKILL_COMBAT:
				skillCombat = value;
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
