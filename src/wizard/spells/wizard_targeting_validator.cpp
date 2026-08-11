#include "wizard/spells/wizard_targeting_validator.hpp"

WizardTargetValidationResult WizardTargetingValidator::validate(
	const WizardTargetType targetType,
	const Position &casterPosition,
	const std::optional<Position> &targetPosition,
	const uint16_t range,
	const bool tileExists,
	const bool lineOfSightRequired,
	const bool lineOfSightClear
) {
	if ((targetType == WizardTargetType::TILE || targetType == WizardTargetType::AREA) && !targetPosition) {
		return WizardTargetValidationResult::POSITION_REQUIRED;
	}
	if (targetType == WizardTargetType::SELF || targetType == WizardTargetType::DIRECTION) {
		return WizardTargetValidationResult::OK;
	}
	if (!tileExists) {
		return WizardTargetValidationResult::INVALID_TILE;
	}
	if (casterPosition.z != targetPosition->z) {
		return WizardTargetValidationResult::DIFFERENT_FLOOR;
	}
	if (Position::getDiagonalDistance(casterPosition, *targetPosition) > range) {
		return WizardTargetValidationResult::OUT_OF_RANGE;
	}
	if (lineOfSightRequired && !lineOfSightClear) {
		return WizardTargetValidationResult::LINE_OF_SIGHT_BLOCKED;
	}
	return WizardTargetValidationResult::OK;
}
