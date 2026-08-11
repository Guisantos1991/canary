#include "wizard/exhaustion/wizard_exhaustion_system.hpp"

TEST(WizardExhaustionSystemTest, UsesDefaultAndAppliesReductions) {
	EXPECT_EQ(WizardExhaustionSystem::calculateRecovery(0, 1, 0), 1998);
	EXPECT_EQ(WizardExhaustionSystem::calculateRecovery(3000, 100, 0), 2640);
	EXPECT_EQ(WizardExhaustionSystem::calculateRecovery(3000, 0, 100), 2757);
	EXPECT_EQ(WizardExhaustionSystem::calculateRecovery(3000, 100, 100), 2400);
}

TEST(WizardExhaustionSystemTest, CapsReductionAndHonorsMinimum) {
	WizardRecoveryConfig config;
	config.controlMaxReduction = 0.90;
	config.masteryMaxReduction = 0.90;
	config.maxTotalReduction = 0.20;
	EXPECT_EQ(WizardExhaustionSystem::calculateRecovery(3000, 100, 100, config), 2400);
	EXPECT_EQ(WizardExhaustionSystem::calculateRecovery(1600, 100, 100), 1500);
}
