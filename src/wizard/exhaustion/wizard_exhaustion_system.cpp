#include "wizard/exhaustion/wizard_exhaustion_system.hpp"

#include <algorithm>
#include <cmath>

uint32_t WizardExhaustionSystem::calculateRecovery(
	const uint32_t spellRecoveryMs,
	const uint16_t magicalControl,
	const uint16_t mastery,
	const WizardRecoveryConfig &config
) {
	const auto baseRecovery = spellRecoveryMs == 0 ? config.defaultMs : spellRecoveryMs;
	const auto control = std::clamp<uint16_t>(magicalControl, 1, 100);
	const auto spellMastery = std::clamp<uint16_t>(mastery, 0, 100);
	const double reduction = std::min(
		(static_cast<double>(control) / 100.0) * config.controlMaxReduction
			+ (static_cast<double>(spellMastery) / 100.0) * config.masteryMaxReduction,
		config.maxTotalReduction
	);
	const auto reduced = static_cast<uint32_t>(std::ceil(static_cast<double>(baseRecovery) * (1.0 - reduction)));
	return std::max(config.minimumMs, reduced);
}

uint32_t WizardExhaustionSystem::calculateCastTime(
	const uint32_t baseCastTimeMs,
	const uint16_t skillCombat,
	const uint16_t mastery,
	const WizardCastConfig &config
) {
	const auto combat = std::clamp<uint16_t>(skillCombat, 1, 100);
	const auto spellMastery = std::clamp<uint16_t>(mastery, 0, 100);
	const double reduction = (static_cast<double>(combat) / 100.0) * config.combatSkillMaxReduction
		+ (static_cast<double>(spellMastery) / 100.0) * config.masteryMaxReduction;
	return static_cast<uint32_t>(std::ceil(static_cast<double>(baseCastTimeMs) * (1.0 - std::min(0.95, reduction))));
}
