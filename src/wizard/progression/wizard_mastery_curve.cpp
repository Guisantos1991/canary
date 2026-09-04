#include "wizard/progression/wizard_mastery_curve.hpp"

#include <algorithm>
#include <limits>

uint64_t WizardMasteryCurve::calculateThreshold(const uint16_t level, const WizardMasteryCurveConfig &config) {
	const auto cappedLevel = std::clamp<uint16_t>(level, config.limits.min, config.limits.max);
	uint16_t previousLevel = 0;
	uint64_t threshold = 0;
	for (const auto &band : config.bands) {
		const auto bandEnd = std::min(cappedLevel, band.throughLevel);
		if (bandEnd > previousLevel) {
			const auto levels = static_cast<uint64_t>(bandEnd - previousLevel);
			if (levels > (std::numeric_limits<uint64_t>::max() - threshold) / band.xpPerLevel) {
				return std::numeric_limits<uint64_t>::max();
			}
			threshold += levels * band.xpPerLevel;
		}
		previousLevel = band.throughLevel;
		if (bandEnd == cappedLevel) break;
	}
	return threshold;
}

uint16_t WizardMasteryCurve::calculateLevel(const uint64_t xp, const WizardMasteryCurveConfig &config) {
	for (uint16_t level = config.limits.max; level > config.limits.min; --level) {
		if (xp >= calculateThreshold(level, config)) return level;
	}
	return config.limits.min;
}

uint64_t WizardMasteryCurve::clampXp(const uint64_t xp, const WizardMasteryCurveConfig &config) {
	return std::min(xp, calculateThreshold(config.limits.max, config));
}
