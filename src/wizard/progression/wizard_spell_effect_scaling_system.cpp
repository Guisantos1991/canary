#include "wizard/progression/wizard_spell_effect_scaling_system.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

double WizardSpellEffectScalingSystem::calculatePotencyMultiplier(const uint16_t magicalControl, const uint16_t mastery, const WizardSpellEffectConfig &config) {
	const auto control = std::clamp<uint16_t>(magicalControl, 1, 100);
	const auto spellMastery = std::clamp<uint16_t>(mastery, 0, 100);
	const double bonus = std::min(
		(static_cast<double>(control) / 100.0) * config.controlMaxPotencyBonus
			+ (static_cast<double>(spellMastery) / 100.0) * config.masteryMaxPotencyBonus,
		config.maxCombinedPotencyBonus
	);
	return 1.0 + std::max(0.0, bonus);
}

int32_t WizardSpellEffectScalingSystem::scalePotency(const int32_t baseValue, const uint16_t magicalControl, const uint16_t mastery, const WizardSpellEffectConfig &config) {
	const double scaled = static_cast<double>(baseValue) * calculatePotencyMultiplier(magicalControl, mastery, config);
	return static_cast<int32_t>(std::clamp<double>(std::floor(scaled), std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max()));
}
