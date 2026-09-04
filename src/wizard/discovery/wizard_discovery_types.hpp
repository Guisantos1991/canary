#pragma once

#include "game/movement/position.hpp"
#include "wizard/progression/wizard_knowledge_types.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

enum class WizardDiscoveryType : uint8_t {
	BOOK,
	PLAQUE,
	PAGE,
	SCROLL,
	WORLD_OBJECT,
	LOCATION,
	INGREDIENT,
	CREATURE,
	NPC,
	ITEM,
};

enum class WizardDiscoveryVisibility : uint8_t {
	SHARED,
	PERSONAL,
};

enum class WizardDiscoveryPlacementMode : uint8_t {
	FIXED,
	PLAYER_RANDOM,
};

enum class WizardDiscoveryRewardType : uint8_t {
	SPELL_KNOWLEDGE,
	RECIPE_KNOWLEDGE,
	DISCOVERY_UNLOCK,
};

enum class WizardDiscoveryStateKind : uint8_t {
	ASSIGNED,
	DISCOVERED,
};

struct WizardDiscoveryLocationDefinition {
	std::string id;
	Position from;
	Position to;

	[[nodiscard]] bool contains(const Position &position) const {
		return position.z == from.z && position.z == to.z
			&& position.x >= from.x && position.x <= to.x
			&& position.y >= from.y && position.y <= to.y;
	}

	[[nodiscard]] bool isPosition() const {
		return from == to;
	}
};

struct WizardDiscoveryRewardDefinition {
	WizardDiscoveryRewardType type = WizardDiscoveryRewardType::DISCOVERY_UNLOCK;
	uint32_t numericId = 0;
	std::string discoveryId;
	uint16_t amount = 0;
};

struct WizardDiscoveryPlacementDefinition {
	WizardDiscoveryPlacementMode mode = WizardDiscoveryPlacementMode::FIXED;
	std::vector<std::string> locationIds;
};

struct WizardDiscoveryDefinition {
	std::string id;
	WizardDiscoveryType type = WizardDiscoveryType::ITEM;
	WizardKnowledgeSource progressionSource = WizardKnowledgeSource::EXPLORATION;
	WizardDiscoveryVisibility visibility = WizardDiscoveryVisibility::SHARED;
	WizardDiscoveryPlacementDefinition placement;
	std::vector<std::string> requiresDiscoveries;
	std::vector<WizardDiscoveryRewardDefinition> rewards;
	std::optional<uint16_t> actionId;
	std::optional<uint16_t> ingredientItemId;
	std::string text;
	bool developmentFixture = false;
	bool personalObject = false;
};

struct WizardDiscoveryState {
	WizardDiscoveryStateKind state = WizardDiscoveryStateKind::ASSIGNED;
	std::string assignedLocationId;
	uint64_t assignedAt = 0;
	uint64_t discoveredAt = 0;
	uint64_t rewardAppliedAt = 0;
};

enum class WizardDiscoveryResult : uint8_t {
	SUCCESS,
	ALREADY_DISCOVERED,
	NOT_FOUND,
	NOT_ELIGIBLE,
	NOT_ASSIGNED,
	WRONG_LOCATION,
	TOO_FAR,
	INVALID_TRIGGER,
	CORRUPT_ASSIGNMENT,
	PERSISTENCE_ERROR,
};
