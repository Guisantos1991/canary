#include "wizard/discovery/wizard_discovery_registry.hpp"

#include "wizard/potions/wizard_potion_registry.hpp"
#include "wizard/spells/wizard_spell_registry.hpp"

#include <fstream>
#include <functional>
#include <limits>
#include <nlohmann/json.hpp>
#include <unordered_set>

namespace {
	constexpr uint16_t LOCATION_BUCKET_SIZE = 32;

	Position parsePosition(const nlohmann::json &value) {
		return {
			value.at("x").get<uint16_t>(),
			value.at("y").get<uint16_t>(),
			value.at("z").get<uint8_t>(),
		};
	}

	WizardDiscoveryType parseType(const std::string &value) {
		static const std::unordered_map<std::string, WizardDiscoveryType> values {
			{ "BOOK", WizardDiscoveryType::BOOK }, { "PLAQUE", WizardDiscoveryType::PLAQUE },
			{ "PAGE", WizardDiscoveryType::PAGE }, { "SCROLL", WizardDiscoveryType::SCROLL },
			{ "WORLD_OBJECT", WizardDiscoveryType::WORLD_OBJECT }, { "LOCATION", WizardDiscoveryType::LOCATION },
			{ "INGREDIENT", WizardDiscoveryType::INGREDIENT }, { "CREATURE", WizardDiscoveryType::CREATURE },
			{ "NPC", WizardDiscoveryType::NPC }, { "ITEM", WizardDiscoveryType::ITEM },
		};
		const auto found = values.find(value);
		if (found == values.end()) throw std::runtime_error("unknown discovery type: " + value);
		return found->second;
	}

	WizardDiscoveryRewardType parseRewardType(const std::string &value) {
		if (value == "SPELL_KNOWLEDGE") return WizardDiscoveryRewardType::SPELL_KNOWLEDGE;
		if (value == "RECIPE_KNOWLEDGE") return WizardDiscoveryRewardType::RECIPE_KNOWLEDGE;
		if (value == "DISCOVERY_UNLOCK" || value == "CLUE") return WizardDiscoveryRewardType::DISCOVERY_UNLOCK;
		throw std::runtime_error("unknown discovery reward type: " + value);
	}

	WizardDiscoveryLocationDefinition parseLocation(const nlohmann::json &json) {
		WizardDiscoveryLocationDefinition location;
		location.id = json.at("id").get<std::string>();
		if (json.contains("position")) {
			location.from = parsePosition(json.at("position"));
			location.to = location.from;
		} else if (json.contains("area")) {
			location.from = parsePosition(json.at("area").at("from"));
			location.to = parsePosition(json.at("area").at("to"));
		} else {
			throw std::runtime_error("location requires position or area");
		}
		if (location.id.empty() || location.from.x == 0 || location.from.y == 0 || location.from.z > 15
			|| location.to.x < location.from.x || location.to.y < location.from.y || location.to.z != location.from.z) {
			throw std::runtime_error("invalid location: " + location.id);
		}
		return location;
	}

	WizardDiscoveryDefinition parseDiscovery(const nlohmann::json &json) {
		WizardDiscoveryDefinition definition;
		definition.id = json.at("id").get<std::string>();
		definition.type = parseType(json.at("type").get<std::string>());
		if (!parseWizardKnowledgeSource(json.at("progressionSource").get<std::string>(), definition.progressionSource)
			|| (definition.progressionSource != WizardKnowledgeSource::READING && definition.progressionSource != WizardKnowledgeSource::EXPLORATION)) {
			throw std::runtime_error("unknown discovery progression source");
		}
		const auto repeatability = json.value("repeatability", "ONE_SHOT");
		if (repeatability != "ONE_SHOT") throw std::runtime_error("unsupported discovery repeatability: " + repeatability);
		const auto visibility = json.value("visibility", "SHARED");
		if (visibility == "SHARED") definition.visibility = WizardDiscoveryVisibility::SHARED;
		else if (visibility == "PERSONAL") definition.visibility = WizardDiscoveryVisibility::PERSONAL;
		else throw std::runtime_error("unknown discovery visibility: " + visibility);
		definition.personalObject = json.value("personalObject", false);
		definition.text = json.value("text", "");
		definition.developmentFixture = json.value("developmentFixture", false);
		if (json.contains("actionId")) definition.actionId = json.at("actionId").get<uint16_t>();
		if (json.contains("ingredientItemId")) definition.ingredientItemId = json.at("ingredientItemId").get<uint16_t>();

		const auto &placement = json.at("placement");
		const auto mode = placement.at("mode").get<std::string>();
		if (mode == "FIXED") definition.placement.mode = WizardDiscoveryPlacementMode::FIXED;
		else if (mode == "PLAYER_RANDOM") definition.placement.mode = WizardDiscoveryPlacementMode::PLAYER_RANDOM;
		else throw std::runtime_error("unknown discovery placement mode: " + mode);
		if (placement.contains("locationId")) definition.placement.locationIds.push_back(placement.at("locationId").get<std::string>());
		for (const auto &locationId : placement.value("locationPool", nlohmann::json::array())) definition.placement.locationIds.push_back(locationId.get<std::string>());

		if (json.contains("requirements")) {
			for (const auto &id : json.at("requirements").value("allOf", nlohmann::json::array())) definition.requiresDiscoveries.push_back(id.get<std::string>());
		}
		for (const auto &rewardJson : json.value("rewards", nlohmann::json::array())) {
			WizardDiscoveryRewardDefinition reward;
			reward.type = parseRewardType(rewardJson.at("type").get<std::string>());
			if (reward.type == WizardDiscoveryRewardType::SPELL_KNOWLEDGE) reward.numericId = rewardJson.at("spellId").get<uint32_t>();
			else if (reward.type == WizardDiscoveryRewardType::RECIPE_KNOWLEDGE) reward.numericId = rewardJson.at("recipeId").get<uint32_t>();
			else reward.discoveryId = rewardJson.at("discoveryId").get<std::string>();
			if (reward.type != WizardDiscoveryRewardType::DISCOVERY_UNLOCK) {
				const auto amount = rewardJson.at("amount").get<int64_t>();
				if (amount <= 0 || amount > std::numeric_limits<uint16_t>::max()) {
					throw std::runtime_error("discovery reward amount must be between 1 and 65535");
				}
				reward.amount = static_cast<uint16_t>(amount);
			}
			definition.rewards.push_back(std::move(reward));
		}
		return definition;
	}

	void validateCycles(const std::unordered_map<std::string, WizardDiscoveryDefinition> &definitions) {
		enum class Visit : uint8_t { NONE, VISITING, DONE };
		std::unordered_map<std::string, Visit> visits;
		std::function<void(const std::string &)> visit = [&](const std::string &id) {
			auto &state = visits[id];
			if (state == Visit::VISITING) throw std::runtime_error("discovery dependency cycle at: " + id);
			if (state == Visit::DONE) return;
			state = Visit::VISITING;
			for (const auto &required : definitions.at(id).requiresDiscoveries) visit(required);
			state = Visit::DONE;
		};
		for (const auto &[id, definition] : definitions) visit(id);
	}
}

WizardDiscoveryRegistry &WizardDiscoveryRegistry::getInstance() {
	static WizardDiscoveryRegistry instance;
	return instance;
}

bool WizardDiscoveryRegistry::load(const std::string &path, std::string &error) {
	try {
		std::ifstream input(path);
		if (!input) { error = "cannot open " + path; return false; }
		const auto root = nlohmann::json::parse(input);
		WizardDiscoveryRegistry candidate;
		for (const auto &entry : root.at("locations")) {
			auto location = parseLocation(entry);
			if (!candidate.locations.emplace(location.id, std::move(location)).second) throw std::runtime_error("duplicate location id");
		}
		for (const auto &entry : root.at("discoveries")) {
			auto definition = parseDiscovery(entry);
			if (definition.id.empty()) throw std::runtime_error("discovery id is required");
			if (!candidate.definitions.emplace(definition.id, std::move(definition)).second) throw std::runtime_error("duplicate discovery id");
		}

		for (const auto &[id, definition] : candidate.definitions) {
			if (definition.placement.locationIds.empty()) throw std::runtime_error("discovery placement is empty: " + id);
			if (definition.placement.mode == WizardDiscoveryPlacementMode::FIXED && definition.placement.locationIds.size() != 1) throw std::runtime_error("FIXED discovery requires exactly one location: " + id);
			std::unordered_set<std::string> uniqueLocations;
			for (const auto &locationId : definition.placement.locationIds) {
				const auto found = candidate.locations.find(locationId);
				if (found == candidate.locations.end()) throw std::runtime_error("unknown discovery location: " + locationId);
				if (!uniqueLocations.emplace(locationId).second) throw std::runtime_error("duplicate location in discovery: " + id);
			}
			for (const auto &required : definition.requiresDiscoveries) {
				if (required == id) throw std::runtime_error("self discovery dependency: " + id);
				if (!candidate.definitions.contains(required)) throw std::runtime_error("unknown required discovery: " + required);
			}
			for (const auto &reward : definition.rewards) {
				if (reward.type == WizardDiscoveryRewardType::SPELL_KNOWLEDGE && (!g_wizardSpells().getById(reward.numericId) || reward.amount == 0)) throw std::runtime_error("invalid spell knowledge reward: " + id);
				if (reward.type == WizardDiscoveryRewardType::RECIPE_KNOWLEDGE && (!g_wizardPotions().getById(reward.numericId) || reward.amount == 0)) throw std::runtime_error("invalid recipe knowledge reward: " + id);
				if (reward.type == WizardDiscoveryRewardType::DISCOVERY_UNLOCK) {
					const auto target = candidate.definitions.find(reward.discoveryId);
					if (target == candidate.definitions.end() || target->second.id == id) throw std::runtime_error("invalid discovery unlock reward: " + id);
					if (!target->second.rewards.empty()) throw std::runtime_error("DISCOVERY_UNLOCK target must not have rewards: " + reward.discoveryId);
					if (target->second.placement.mode != WizardDiscoveryPlacementMode::FIXED) throw std::runtime_error("DISCOVERY_UNLOCK target must use FIXED placement: " + reward.discoveryId);
				}
			}
			if (definition.actionId && (!candidate.discoveriesByActionId.emplace(*definition.actionId, id).second || *definition.actionId == 0)) throw std::runtime_error("duplicate or zero discovery actionId");
			if (definition.ingredientItemId && (!candidate.discoveriesByIngredientItemId.emplace(*definition.ingredientItemId, id).second || *definition.ingredientItemId == 0)) throw std::runtime_error("duplicate or zero ingredient item id");
			if (definition.personalObject && (definition.visibility != WizardDiscoveryVisibility::PERSONAL || !definition.actionId)) throw std::runtime_error("personalObject requires PERSONAL visibility and actionId: " + id);
			if (definition.type == WizardDiscoveryType::LOCATION) {
				for (const auto &locationId : definition.placement.locationIds) candidate.indexLocationTrigger(definition, candidate.locations.at(locationId));
			}
		}
		validateCycles(candidate.definitions);
		if (candidate.definitions.empty()) throw std::runtime_error("registry contains no discoveries");
		*this = std::move(candidate);
		return true;
	} catch (const std::exception &exception) {
		error = exception.what();
		return false;
	}
}

const WizardDiscoveryDefinition* WizardDiscoveryRegistry::getById(const std::string &id) const {
	const auto found = definitions.find(id);
	return found == definitions.end() ? nullptr : &found->second;
}

const WizardDiscoveryDefinition* WizardDiscoveryRegistry::getByActionId(const uint16_t actionId) const {
	const auto found = discoveriesByActionId.find(actionId);
	return found == discoveriesByActionId.end() ? nullptr : getById(found->second);
}

const WizardDiscoveryDefinition* WizardDiscoveryRegistry::getByIngredientItemId(const uint16_t itemId) const {
	const auto found = discoveriesByIngredientItemId.find(itemId);
	return found == discoveriesByIngredientItemId.end() ? nullptr : getById(found->second);
}

const WizardDiscoveryLocationDefinition* WizardDiscoveryRegistry::getLocation(const std::string &id) const {
	const auto found = locations.find(id);
	return found == locations.end() ? nullptr : &found->second;
}

uint64_t WizardDiscoveryRegistry::locationBucketKey(const Position &position) {
	return (static_cast<uint64_t>(position.z) << 48U) | (static_cast<uint64_t>(position.x / LOCATION_BUCKET_SIZE) << 24U) | (position.y / LOCATION_BUCKET_SIZE);
}

void WizardDiscoveryRegistry::indexLocationTrigger(const WizardDiscoveryDefinition &definition, const WizardDiscoveryLocationDefinition &location) {
	for (uint32_t x = location.from.x / LOCATION_BUCKET_SIZE; x <= location.to.x / LOCATION_BUCKET_SIZE; ++x) {
		for (uint32_t y = location.from.y / LOCATION_BUCKET_SIZE; y <= location.to.y / LOCATION_BUCKET_SIZE; ++y) {
			const Position bucketPosition { static_cast<uint16_t>(x * LOCATION_BUCKET_SIZE), static_cast<uint16_t>(y * LOCATION_BUCKET_SIZE), location.from.z };
			locationTriggers[locationBucketKey(bucketPosition)].push_back({ definition.id, location.id });
		}
	}
}

const std::vector<WizardLocationTrigger> &WizardDiscoveryRegistry::getLocationTriggers(const Position &position) const {
	static const std::vector<WizardLocationTrigger> empty;
	const auto found = locationTriggers.find(locationBucketKey(position));
	return found == locationTriggers.end() ? empty : found->second;
}

const std::unordered_map<std::string, WizardDiscoveryDefinition> &WizardDiscoveryRegistry::getDefinitions() const { return definitions; }
size_t WizardDiscoveryRegistry::size() const { return definitions.size(); }
void WizardDiscoveryRegistry::clear() { definitions.clear(); locations.clear(); discoveriesByActionId.clear(); discoveriesByIngredientItemId.clear(); locationTriggers.clear(); }
