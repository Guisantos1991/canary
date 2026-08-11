#include "wizard/mana/wizard_mana_system.hpp"

TEST(WizardManaSystemTest, CalculatesMaximumManaFromPowerAndControl) {
	WizardSkillSnapshot skills { 50, 25, 1, 1 };
	EXPECT_EQ(WizardManaSystem::calculateMaxMana(skills), 550);
}

TEST(WizardManaSystemTest, AppliesControlMasteryAndTotalCaps) {
	EXPECT_EQ(WizardManaSystem::calculateSpellManaCost(100, 100, 0), 85);
	EXPECT_EQ(WizardManaSystem::calculateSpellManaCost(100, 1, 100), 90);
	EXPECT_EQ(WizardManaSystem::calculateSpellManaCost(100, 100, 100), 75);

	WizardManaConfig config;
	config.controlMaxReduction = 0.90;
	config.masteryMaxReduction = 0.90;
	config.maxTotalReduction = 0.25;
	EXPECT_EQ(WizardManaSystem::calculateSpellManaCost(100, 100, 100, config), 75);
}

TEST(WizardManaSystemTest, NeverReturnsFreeManaCost) {
	EXPECT_EQ(WizardManaSystem::calculateSpellManaCost(1, 100, 100), 1);
	EXPECT_EQ(WizardManaSystem::calculateSpellManaCost(0, 100, 100), 1);
}
