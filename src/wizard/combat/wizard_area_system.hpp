#pragma once

#include <cstdint>

#include "game/movement/position.hpp"
#include "wizard/spells/wizard_spell_definition.hpp"

class WizardAreaSystem {
public:
	[[nodiscard]] static uint16_t calculateEffectiveSquares(
		uint16_t magicalPower,
		uint16_t minSquares,
		uint16_t maxSquares
	);

	[[nodiscard]] static std::vector<Position> resolve(
		const WizardAreaDefinition &area,
		const Position &center,
		uint16_t magicalPower,
		Direction direction = DIRECTION_NORTH
	);
};
