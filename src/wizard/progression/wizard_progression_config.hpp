#pragma once

#include <cstdint>
#include <string>
#include <vector>

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

struct WizardValueLimits {
	uint16_t min = 0;
	uint16_t max = 100;
};

struct WizardXpBand {
	uint16_t throughLevel = 0;
	uint64_t xpPerLevel = 0;
};

struct WizardMasteryCurveConfig {
	WizardValueLimits limits;
	std::vector<WizardXpBand> bands;
};

struct WizardMeaningfulUseConfig {
	uint64_t baseXp = 10;
	uint64_t additionalTargetBonus = 2;
	uint16_t bonusTargetCap = 3;
};

struct WizardSpellEffectConfig {
	double masteryMaxPotencyBonus = 0.15;
	double controlMaxPotencyBonus = 0.10;
	double maxCombinedPotencyBonus = 0.20;
};

struct WizardBrewingConfig {
	uint16_t defaultIngredientQuality = 50;
	uint64_t xpPerValidBrew = 10;
};

struct WizardPotionQualityConfig {
	double ingredientWeight = 0.50;
	double controlMaxBonus = 15.0;
	double masteryMaxBonus = 15.0;
	uint16_t min = 0;
	uint16_t max = 100;
};

struct WizardPotionEffectConfig {
	double qualityMaxBonus = 0.15;
	double controlMaxBonus = 0.05;
	double masteryMaxBonus = 0.10;
	double linkedSpellMaxBonus = 0.05;
	double maxCombinedBonus = 0.25;
};

struct WizardProgressionConfigData {
	WizardSkillLimits skills;
	WizardManaConfig mana;
	WizardRecoveryConfig recovery;
	WizardCastConfig cast;
	WizardValueLimits knowledge;
	WizardMasteryCurveConfig spellMastery;
	WizardMeaningfulUseConfig meaningfulUse;
	WizardSpellEffectConfig spellEffect;
	WizardValueLimits recipeKnowledge;
	WizardMasteryCurveConfig brewingMastery;
	WizardBrewingConfig brewing;
	WizardPotionQualityConfig potionQuality;
	WizardPotionEffectConfig potionEffect;
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
