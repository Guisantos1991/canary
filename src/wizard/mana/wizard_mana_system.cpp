#include "wizard/mana/wizard_mana_system.hpp"

#include <algorithm>
#include <cmath>

uint32_t WizardManaSystem::calculateMaxMana(
	const WizardSkillSnapshot &skills,
	const WizardManaConfig &config
) {
	const auto power = skills.getMagicalPower();
	const auto control = skills.getMagicalControl();

	return config.base
		+ (static_cast<uint32_t>(power) * config.powerMultiplier)
		+ (static_cast<uint32_t>(control) * config.controlMultiplier);
}

uint32_t WizardManaSystem::calculateSpellManaCost(
	const uint32_t baseManaCost,
	const uint16_t magicalControl,
	const uint16_t mastery,
	const WizardManaConfig &config
) {
	const auto control =
		std::clamp<uint16_t>(magicalControl, 1, 100);

	const auto spellMastery =
		std::clamp<uint16_t>(mastery, 0, 100);

	// Controle pode economizar até 15%.
	const double controlReduction =
		(static_cast<double>(control) / 100.0) * config.controlMaxReduction;

	// Mastery pode economizar até 10%.
	const double masteryReduction =
		(static_cast<double>(spellMastery) / 100.0) * config.masteryMaxReduction;

	// Nunca reduzimos mais que 25%.
	const double totalReduction =
		std::min(
			controlReduction + masteryReduction,
			config.maxTotalReduction
		);

	const double finalCost =
		static_cast<double>(baseManaCost)
		* (1.0 - totalReduction);

	return std::max<uint32_t>(
		1,
		static_cast<uint32_t>(std::ceil(finalCost))
	);
}
