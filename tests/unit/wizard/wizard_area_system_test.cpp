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

class WizardDirectionalAreaTest : public ::testing::TestWithParam<std::tuple<WizardAreaPattern, Direction, Position>> { };

TEST_P(WizardDirectionalAreaTest, RotatesNorthGeometryToCasterDirection) {
	const auto &[pattern, direction, expectedForward] = GetParam();
	const Position center { 100, 100, 7 };
	const WizardAreaDefinition area { pattern, 2, 2 };
	const auto positions = WizardAreaSystem::resolve(area, center, 100, direction);
	ASSERT_EQ(positions.size(), 2);
	EXPECT_EQ(positions.front(), center);
	EXPECT_EQ(positions[1], expectedForward);
}

INSTANTIATE_TEST_SUITE_P(
	LineConeWaveAllDirections,
	WizardDirectionalAreaTest,
	::testing::Values(
		std::tuple { WizardAreaPattern::LINE, DIRECTION_NORTH, Position { 100, 99, 7 } },
		std::tuple { WizardAreaPattern::LINE, DIRECTION_EAST, Position { 101, 100, 7 } },
		std::tuple { WizardAreaPattern::LINE, DIRECTION_SOUTH, Position { 100, 101, 7 } },
		std::tuple { WizardAreaPattern::LINE, DIRECTION_WEST, Position { 99, 100, 7 } },
		std::tuple { WizardAreaPattern::CONE, DIRECTION_NORTH, Position { 99, 99, 7 } },
		std::tuple { WizardAreaPattern::CONE, DIRECTION_EAST, Position { 101, 99, 7 } },
		std::tuple { WizardAreaPattern::CONE, DIRECTION_SOUTH, Position { 101, 101, 7 } },
		std::tuple { WizardAreaPattern::CONE, DIRECTION_WEST, Position { 99, 101, 7 } },
		std::tuple { WizardAreaPattern::WAVE, DIRECTION_NORTH, Position { 99, 99, 7 } },
		std::tuple { WizardAreaPattern::WAVE, DIRECTION_EAST, Position { 101, 99, 7 } },
		std::tuple { WizardAreaPattern::WAVE, DIRECTION_SOUTH, Position { 101, 101, 7 } },
		std::tuple { WizardAreaPattern::WAVE, DIRECTION_WEST, Position { 99, 101, 7 } }
	)
);

TEST(WizardAreaSystemTest, RingExcludesCenterAndIncludesEntirePerimeter) {
	const Position center { 100, 100, 7 };
	const WizardAreaDefinition ring { WizardAreaPattern::RING, 8, 8 };
	const auto positions = WizardAreaSystem::resolve(ring, center, 100);
	ASSERT_EQ(positions.size(), 8);
	EXPECT_EQ(std::ranges::find(positions, center), positions.end());
	for (int32_t y = -1; y <= 1; ++y) {
		for (int32_t x = -1; x <= 1; ++x) {
			if (x == 0 && y == 0) {
				continue;
			}
			EXPECT_NE(std::ranges::find(positions, Position { static_cast<uint16_t>(100 + x), static_cast<uint16_t>(100 + y), 7 }), positions.end());
		}
	}
}

TEST(WizardAreaSystemTest, CustomDoesNotSilentlyResolveAsCircle) {
	const WizardAreaDefinition custom { WizardAreaPattern::CUSTOM, 3, 12 };
	EXPECT_TRUE(WizardAreaSystem::resolve(custom, Position { 100, 100, 7 }, 100).empty());
}
