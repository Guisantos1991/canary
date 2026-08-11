#pragma once

#include "game/movement/position.hpp"
#include "wizard/spells/wizard_spell_definition.hpp"

#include <optional>

enum class WizardTargetValidationResult : uint8_t {
	OK,
	POSITION_REQUIRED,
	INVALID_TILE,
	DIFFERENT_FLOOR,
	OUT_OF_RANGE,
	LINE_OF_SIGHT_BLOCKED
};

class WizardTargetingValidator {
public:
	[[nodiscard]] static WizardTargetValidationResult validate(
		WizardTargetType targetType,
		const Position &casterPosition,
		const std::optional<Position> &targetPosition,
		uint16_t range,
		bool tileExists,
		bool lineOfSightRequired,
		bool lineOfSightClear
	);
};
