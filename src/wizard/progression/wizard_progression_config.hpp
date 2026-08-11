#pragma once

#include <cstdint>
#include <string>

struct WizardSkillLimits {
	uint16_t min = 1;
	uint16_t max = 100;
};

struct WizardManaConfig {
	uint32_t base = 100;
	uint32_t powerMultiplier = 8;
	uint32_t controlMultiplier = 2;
	double controlMaxReduction = 0.15;
	double masteryMaxReduction = 0.10;
	double maxTotalReduction = 0.25;
};

struct WizardRecoveryConfig {
	uint32_t defaultMs = 2000;
	uint32_t minimumMs = 1500;
	double controlMaxReduction = 0.12;
	double masteryMaxReduction = 0.08;
	double maxTotalReduction = 0.20;
};

struct WizardCastConfig {
	double combatSkillMaxReduction = 0.10;
	double masteryMaxReduction = 0.05;
};

struct WizardProgressionConfigData {
	WizardSkillLimits skills;
	WizardManaConfig mana;
	WizardRecoveryConfig recovery;
	WizardCastConfig cast;
};

class WizardProgressionConfig {
public:
	static WizardProgressionConfig &getInstance();

	bool load(const std::string &path, std::string &error);
	[[nodiscard]] const WizardProgressionConfigData &get() const;
	[[nodiscard]] bool isLoaded() const;

private:
	WizardProgressionConfigData data;
	bool loaded = false;
};

constexpr auto g_wizardProgression = WizardProgressionConfig::getInstance;
