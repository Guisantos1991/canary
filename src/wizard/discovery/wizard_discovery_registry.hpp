#pragma once

#include "wizard/discovery/wizard_discovery_types.hpp"

#include <string>
#include <unordered_map>

struct WizardLocationTrigger {
	std::string discoveryId;
	std::string locationId;
};

class WizardDiscoveryRegistry {
public:
	static WizardDiscoveryRegistry &getInstance();

	bool load(const std::string &path, std::string &error);
	void clear();

	[[nodiscard]] const WizardDiscoveryDefinition* getById(const std::string &id) const;
	[[nodiscard]] const WizardDiscoveryDefinition* getByActionId(uint16_t actionId) const;
	[[nodiscard]] const WizardDiscoveryDefinition* getByIngredientItemId(uint16_t itemId) const;
	[[nodiscard]] const WizardDiscoveryLocationDefinition* getLocation(const std::string &id) const;
	[[nodiscard]] const std::vector<WizardLocationTrigger> &getLocationTriggers(const Position &position) const;
	[[nodiscard]] const std::unordered_map<std::string, WizardDiscoveryDefinition> &getDefinitions() const;
	[[nodiscard]] size_t size() const;

private:
	static uint64_t locationBucketKey(const Position &position);
	void indexLocationTrigger(const WizardDiscoveryDefinition &definition, const WizardDiscoveryLocationDefinition &location);

	std::unordered_map<std::string, WizardDiscoveryDefinition> definitions;
	std::unordered_map<std::string, WizardDiscoveryLocationDefinition> locations;
	std::unordered_map<uint16_t, std::string> discoveriesByActionId;
	std::unordered_map<uint16_t, std::string> discoveriesByIngredientItemId;
	std::unordered_map<uint64_t, std::vector<WizardLocationTrigger>> locationTriggers;
};

constexpr auto g_wizardDiscoveries = WizardDiscoveryRegistry::getInstance;
