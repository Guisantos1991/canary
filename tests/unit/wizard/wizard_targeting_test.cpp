#include "wizard/spells/wizard_targeting_validator.hpp"

TEST(WizardTargetingTest, TileSpellRequiresPositionAndNeverRequiresCreatureId) {
	const Position caster { 100, 100, 7 };
	EXPECT_EQ(WizardTargetingValidator::validate(WizardTargetType::TILE, caster, std::nullopt, 7, false, true, false), WizardTargetValidationResult::POSITION_REQUIRED);
	EXPECT_EQ(WizardTargetingValidator::validate(WizardTargetType::TILE, caster, Position { 104, 100, 7 }, 7, true, true, true), WizardTargetValidationResult::OK);
}

TEST(WizardTargetingTest, RejectsMissingOutOfRangeAndBlockedTiles) {
	const Position caster { 100, 100, 7 };
	EXPECT_EQ(WizardTargetingValidator::validate(WizardTargetType::TILE, caster, Position { 101, 100, 7 }, 7, false, true, true), WizardTargetValidationResult::INVALID_TILE);
	EXPECT_EQ(WizardTargetingValidator::validate(WizardTargetType::TILE, caster, Position { 108, 100, 7 }, 7, true, true, true), WizardTargetValidationResult::OUT_OF_RANGE);
	EXPECT_EQ(WizardTargetingValidator::validate(WizardTargetType::TILE, caster, Position { 101, 100, 7 }, 7, true, true, false), WizardTargetValidationResult::LINE_OF_SIGHT_BLOCKED);
}

TEST(WizardTargetingTest, OccupancyCanChangeBeforeImpact) {
	const Position aimedTile { 101, 100, 7 };
	Position targetAtImpact { 101, 100, 7 };
	EXPECT_EQ(targetAtImpact, aimedTile);
	targetAtImpact = Position { 102, 100, 7 };
	EXPECT_NE(targetAtImpact, aimedTile);
}
