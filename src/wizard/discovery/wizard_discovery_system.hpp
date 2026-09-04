#pragma once

#include "wizard/discovery/wizard_discovery_types.hpp"

#include <functional>
#include <memory>
#include <string>

class Item;
class Player;

class WizardDiscoverySystem {
public:
	static bool hasDiscovery(const std::shared_ptr<Player> &player, const std::string &discoveryId);
	static const WizardDiscoveryState* getDiscoveryState(const std::shared_ptr<Player> &player, const std::string &discoveryId);
	static bool canDiscover(const std::shared_ptr<Player> &player, const std::string &discoveryId);
	static WizardDiscoveryResult assignDiscovery(const std::shared_ptr<Player> &player, const std::string &discoveryId, bool adminOverride = false);
	static const WizardDiscoveryLocationDefinition* getAssignedLocation(const std::shared_ptr<Player> &player, const std::string &discoveryId);
	static WizardDiscoveryResult discover(const std::shared_ptr<Player> &player, const std::string &discoveryId, const Position &position, bool adminOverride = false);
	static WizardDiscoveryResult interact(const std::shared_ptr<Player> &player, uint16_t actionId, const Position &position);
	static WizardDiscoveryResult examineIngredient(const std::shared_ptr<Player> &player, uint16_t itemId);
	static void processLocation(const std::shared_ptr<Player> &player, const Position &position);
	static bool isPersonalObjectVisible(const std::shared_ptr<Player> &player, const std::shared_ptr<Item> &item, const Position &position);
	static bool reset(const std::shared_ptr<Player> &player, const std::string &discoveryId, bool clearAssignment);

	static void setRandomIndexProviderForTests(std::function<size_t(size_t)> provider);
	static void resetRandomIndexProviderForTests();
};
