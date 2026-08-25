#pragma once

#include "creatures/creatures_definitions.hpp"
#include "game/movement/position.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

class Player;
class Creature;
struct WizardSpellDefinition;

class WizardSpellCaster {
public:
	static constexpr uint8_t CAST_EXTENDED_OPCODE = 90;

	static bool handleExtendedOpcode(const std::shared_ptr<Player> &player, const std::string &buffer);
	static bool cast(const std::shared_ptr<Player> &player, uint32_t spellId, const std::optional<Position> &targetPosition);

private:
	static void executeCast(uint32_t playerId, uint32_t spellId, Position targetPosition);

#ifdef BUILD_TESTS
public:
#endif
	static void impact(uint32_t playerId, uint32_t spellId, Position targetPosition, Direction direction, uint16_t magicalPower, uint16_t skillCombat, uint16_t mastery);
	static void setAffectedCount(CombatDamage &damage, std::size_t affected);
	static std::vector<std::shared_ptr<Creature>> resolveImpactOccupants(const WizardSpellDefinition &spell, Position targetPosition, Direction direction, uint16_t magicalPower);
	static std::vector<std::shared_ptr<Creature>> collectImpactTargets(const std::shared_ptr<Player> &player, const WizardSpellDefinition &spell, Position targetPosition, Direction direction, uint16_t magicalPower);

private:
	static void sendFailure(const std::shared_ptr<Player> &player, const std::string &message);
};
