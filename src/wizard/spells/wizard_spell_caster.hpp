#pragma once

#include "game/movement/position.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

class Player;
struct WizardSpellDefinition;

class WizardSpellCaster {
public:
	static constexpr uint8_t CAST_EXTENDED_OPCODE = 90;

	static bool handleExtendedOpcode(const std::shared_ptr<Player> &player, const std::string &buffer);
	static bool cast(const std::shared_ptr<Player> &player, uint32_t spellId, const std::optional<Position> &targetPosition);

private:
	static void executeCast(uint32_t playerId, uint32_t spellId, Position targetPosition);
	static void impact(uint32_t playerId, uint32_t spellId, Position targetPosition, uint16_t magicalPower, uint16_t skillCombat, uint16_t mastery);
	static void sendFailure(const std::shared_ptr<Player> &player, const std::string &message);
};
