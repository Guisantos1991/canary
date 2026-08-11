#include "wizard/combat/wizard_area_system.hpp"

TEST(WizardAreaSystemTest, CalculatesRequiredProgressionWithoutUnlockingMaximumEarly) {
	const std::vector<std::pair<uint16_t, uint16_t>> cases {
		{ 1, 3 }, { 20, 4 }, { 40, 6 }, { 60, 8 }, { 80, 10 }, { 90, 11 }, { 99, 11 }, { 100, 12 }
	};
	for (const auto &[power, expected] : cases) {
		EXPECT_EQ(WizardAreaSystem::calculateEffectiveSquares(power, 3, 12), expected) << "power=" << power;
	}
	EXPECT_LE(WizardAreaSystem::calculateEffectiveSquares(500, 3, 12), 12);
}

TEST(WizardAreaSystemTest, TreatsMaximumAtOrBelowMinimumAsMinimum) {
	EXPECT_EQ(WizardAreaSystem::calculateEffectiveSquares(50, 8, 3), 8);
	EXPECT_EQ(WizardAreaSystem::calculateEffectiveSquares(50, 8, 8), 8);
}

TEST(WizardAreaSystemTest, ResolvesDeterministicCenterOutArea) {
	WizardAreaDefinition area { WizardAreaPattern::CIRCLE, 3, 12 };
	const Position center { 100, 100, 7 };
	const auto first = WizardAreaSystem::resolve(area, center, 100);
	const auto second = WizardAreaSystem::resolve(area, center, 100);
	ASSERT_EQ(first.size(), 12);
	EXPECT_EQ(first, second);
	EXPECT_EQ(first.front(), center);
}

TEST(WizardAreaSystemTest, StopsWhenPatternPointsOutsideMapBounds) {
	WizardAreaDefinition area { WizardAreaPattern::LINE, 3, 12 };
	const auto positions = WizardAreaSystem::resolve(area, Position { 100, 0, 7 }, 100, DIRECTION_NORTH);
	ASSERT_EQ(positions.size(), 1);
	EXPECT_EQ(positions.front(), Position(100, 0, 7));
}
