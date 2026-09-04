#include "wizard/discovery/wizard_discovery_system.hpp"

#include "creatures/players/player.hpp"
#include "database/database.hpp"
#include "items/item.hpp"
#include "lib/logging/logger.hpp"
#include "wizard/discovery/wizard_discovery_registry.hpp"
#include "wizard/potions/wizard_recipe_knowledge_system.hpp"
#include "wizard/progression/wizard_knowledge_system.hpp"

#include <chrono>
#include <mutex>
#include <random>

namespace {
	std::recursive_mutex discoveryMutex;
	std::function<size_t(size_t)> randomIndexProvider;

	uint64_t nowSeconds() {
		return static_cast<uint64_t>(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
	}

	size_t selectRandomIndex(const size_t size) {
		if (randomIndexProvider) return randomIndexProvider(size) % size;
		static std::mt19937 generator(std::random_device {}());
		std::uniform_int_distribution<size_t> distribution(0, size - 1);
		return distribution(generator);
	}

	bool isEligible(const std::shared_ptr<Player> &player, const WizardDiscoveryDefinition &definition) {
		for (const auto &required : definition.requiresDiscoveries) {
			if (!WizardDiscoverySystem::hasDiscovery(player, required)) return false;
		}
		return true;
	}

	bool persistSpellProgress(Database &db, const std::shared_ptr<Player> &player, const uint32_t spellId) {
		const auto* progress = player->getWizardSpellProgress(spellId);
		if (!progress) return true;
		return db.executeQuery(fmt::format(
			"INSERT INTO `player_wizard_spells` (`player_id`, `spell_id`, `knowledge`, `mastery`, `mastery_xp`, `knowledge_sources`, `learned`, `uses`) "
			"VALUES ({}, {}, {}, {}, {}, {}, {}, {}) ON DUPLICATE KEY UPDATE `knowledge` = VALUES(`knowledge`), `mastery` = VALUES(`mastery`), "
			"`mastery_xp` = VALUES(`mastery_xp`), `knowledge_sources` = VALUES(`knowledge_sources`), `learned` = VALUES(`learned`), `uses` = VALUES(`uses`)",
			player->getGUID(), spellId, progress->knowledge, progress->mastery, progress->masteryXp, progress->knowledgeSources, progress->learned ? 1 : 0, progress->uses
		));
	}

	bool persistRecipeProgress(Database &db, const std::shared_ptr<Player> &player, const uint32_t recipeId) {
		const auto* progress = player->getWizardRecipeProgress(recipeId);
		if (!progress) return true;
		return db.executeQuery(fmt::format(
			"INSERT INTO `player_wizard_recipes` (`player_id`, `recipe_id`, `knowledge`, `mastery`, `mastery_xp`, `knowledge_sources`, `learned`, `brews`) "
			"VALUES ({}, {}, {}, {}, {}, {}, {}, {}) ON DUPLICATE KEY UPDATE `knowledge` = VALUES(`knowledge`), `mastery` = VALUES(`mastery`), "
			"`mastery_xp` = VALUES(`mastery_xp`), `knowledge_sources` = VALUES(`knowledge_sources`), `learned` = VALUES(`learned`), `brews` = VALUES(`brews`)",
			player->getGUID(), recipeId, progress->knowledge, progress->mastery, progress->masteryXp, progress->knowledgeSources, progress->learned ? 1 : 0, progress->brews
		));
	}

	bool persistUnlock(Database &db, const std::shared_ptr<Player> &player, const std::string &discoveryId, const std::string &locationId) {
		return db.executeQuery(fmt::format(
			"INSERT INTO `player_wizard_discoveries` (`player_id`, `discovery_id`, `state`, `assigned_location_id`, `assigned_at`, `discovered_at`, `reward_applied_at`) "
			"VALUES ({}, {}, 'DISCOVERED', {}, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP) ON DUPLICATE KEY UPDATE `state` = 'DISCOVERED', "
			"`discovered_at` = COALESCE(`discovered_at`, CURRENT_TIMESTAMP), `reward_applied_at` = COALESCE(`reward_applied_at`, CURRENT_TIMESTAMP)",
			player->getGUID(), db.escapeString(discoveryId), db.escapeString(locationId)
		));

	}

	bool applyRewards(const std::shared_ptr<Player> &player, const WizardDiscoveryDefinition &definition, Database* db) {
		for (const auto &reward : definition.rewards) {
			switch (reward.type) {
				case WizardDiscoveryRewardType::SPELL_KNOWLEDGE: {
					const auto result = WizardKnowledgeSystem::addKnowledge(player, reward.numericId, reward.amount, definition.progressionSource);
					if (result == WizardKnowledgeResult::SUCCESS && db && !persistSpellProgress(*db, player, reward.numericId)) return false;
					break;
				}
				case WizardDiscoveryRewardType::RECIPE_KNOWLEDGE: {
					const auto result = WizardRecipeKnowledgeSystem::addKnowledge(player, reward.numericId, reward.amount, definition.progressionSource);
					if (result == WizardRecipeKnowledgeResult::SUCCESS && db && !persistRecipeProgress(*db, player, reward.numericId)) return false;
					break;
				}
				case WizardDiscoveryRewardType::DISCOVERY_UNLOCK: {
					const auto* target = g_wizardDiscoveries().getById(reward.discoveryId);
					if (!target || target->placement.locationIds.empty()) return false;
					auto &state = player->getOrCreateWizardDiscoveryState(reward.discoveryId);
					state.state = WizardDiscoveryStateKind::DISCOVERED;
					state.assignedLocationId = target->placement.locationIds.front();
					state.assignedAt = nowSeconds();
					state.discoveredAt = nowSeconds();
					state.rewardAppliedAt = state.discoveredAt;
					if (db && !persistUnlock(*db, player, reward.discoveryId, state.assignedLocationId)) return false;
					break;
				}
			}
		}
		return true;
	}

	bool validatePosition(const std::shared_ptr<Player> &player, const WizardDiscoveryDefinition &definition, const Position &position, WizardDiscoveryResult &failure) {
		if (definition.type == WizardDiscoveryType::INGREDIENT) return true;
		if (definition.type != WizardDiscoveryType::LOCATION && !Position::areInRange<1, 1, 0>(player->getPosition(), position)) {
			failure = WizardDiscoveryResult::TOO_FAR;
			return false;
		}
		const auto* state = player->getWizardDiscoveryState(definition.id);
		if (!state) {
			failure = WizardDiscoveryResult::NOT_ASSIGNED;
			return false;
		}
		const auto* location = g_wizardDiscoveries().getLocation(state->assignedLocationId);
		if (!location) {
			failure = WizardDiscoveryResult::CORRUPT_ASSIGNMENT;
			return false;
		}
		if (!location->contains(position)) {
			failure = WizardDiscoveryResult::WRONG_LOCATION;
			return false;
		}
		return true;
	}
}

bool WizardDiscoverySystem::hasDiscovery(const std::shared_ptr<Player> &player, const std::string &discoveryId) {
	const auto* state = player ? player->getWizardDiscoveryState(discoveryId) : nullptr;
	return state && state->state == WizardDiscoveryStateKind::DISCOVERED;
}

const WizardDiscoveryState* WizardDiscoverySystem::getDiscoveryState(const std::shared_ptr<Player> &player, const std::string &discoveryId) {
	return player ? player->getWizardDiscoveryState(discoveryId) : nullptr;
}

bool WizardDiscoverySystem::canDiscover(const std::shared_ptr<Player> &player, const std::string &discoveryId) {
	const auto* definition = g_wizardDiscoveries().getById(discoveryId);
	return player && definition && !hasDiscovery(player, discoveryId) && isEligible(player, *definition);
}

WizardDiscoveryResult WizardDiscoverySystem::assignDiscovery(const std::shared_ptr<Player> &player, const std::string &discoveryId, const bool adminOverride) {
	std::lock_guard lock(discoveryMutex);
	const auto* definition = g_wizardDiscoveries().getById(discoveryId);
	if (!player || !definition) return WizardDiscoveryResult::NOT_FOUND;
	if (const auto* existing = player->getWizardDiscoveryState(discoveryId)) {
		if (!g_wizardDiscoveries().getLocation(existing->assignedLocationId)) {
			g_logger().error("[WizardDiscoverySystem] Player {} has invalid persisted assignment '{}' for discovery '{}'; refusing reroll", player->getGUID(), existing->assignedLocationId, discoveryId);
			return WizardDiscoveryResult::CORRUPT_ASSIGNMENT;
		}
		return existing->state == WizardDiscoveryStateKind::DISCOVERED ? WizardDiscoveryResult::ALREADY_DISCOVERED : WizardDiscoveryResult::SUCCESS;
	}
	if (!adminOverride && !isEligible(player, *definition)) return WizardDiscoveryResult::NOT_ELIGIBLE;
	const auto index = definition->placement.mode == WizardDiscoveryPlacementMode::PLAYER_RANDOM ? selectRandomIndex(definition->placement.locationIds.size()) : 0;
	const auto &locationId = definition->placement.locationIds.at(index);
	WizardDiscoveryState assigned;
	assigned.state = WizardDiscoveryStateKind::ASSIGNED;
	assigned.assignedLocationId = locationId;
	assigned.assignedAt = nowSeconds();

	if (player->getGUID() != 0) {
		auto &db = g_database();
		if (!db.executeQuery(fmt::format(
			"INSERT IGNORE INTO `player_wizard_discoveries` (`player_id`, `discovery_id`, `state`, `assigned_location_id`, `assigned_at`) VALUES ({}, {}, 'ASSIGNED', {}, CURRENT_TIMESTAMP)",
			player->getGUID(), db.escapeString(discoveryId), db.escapeString(locationId)
		))) return WizardDiscoveryResult::PERSISTENCE_ERROR;
		const auto result = db.storeQuery(fmt::format(
			"SELECT `state`, `assigned_location_id`, UNIX_TIMESTAMP(`assigned_at`) AS `assigned_at`, UNIX_TIMESTAMP(`discovered_at`) AS `discovered_at`, UNIX_TIMESTAMP(`reward_applied_at`) AS `reward_applied_at` "
			"FROM `player_wizard_discoveries` WHERE `player_id` = {} AND `discovery_id` = {}",
			player->getGUID(), db.escapeString(discoveryId)
		));
		if (!result) return WizardDiscoveryResult::PERSISTENCE_ERROR;
		assigned.state = result->getString("state") == "DISCOVERED" ? WizardDiscoveryStateKind::DISCOVERED : WizardDiscoveryStateKind::ASSIGNED;
		assigned.assignedLocationId = result->getString("assigned_location_id");
		assigned.assignedAt = result->getNumber<uint64_t>("assigned_at");
		assigned.discoveredAt = result->getNumber<uint64_t>("discovered_at");
		assigned.rewardAppliedAt = result->getNumber<uint64_t>("reward_applied_at");
		if (!g_wizardDiscoveries().getLocation(assigned.assignedLocationId)) {
			g_logger().error("[WizardDiscoverySystem] Database returned invalid assignment '{}' for player {} discovery '{}'; refusing reroll", assigned.assignedLocationId, player->getGUID(), discoveryId);
			return WizardDiscoveryResult::CORRUPT_ASSIGNMENT;
		}
	}
	player->getOrCreateWizardDiscoveryState(discoveryId) = std::move(assigned);
	return hasDiscovery(player, discoveryId) ? WizardDiscoveryResult::ALREADY_DISCOVERED : WizardDiscoveryResult::SUCCESS;
}

const WizardDiscoveryLocationDefinition* WizardDiscoverySystem::getAssignedLocation(const std::shared_ptr<Player> &player, const std::string &discoveryId) {
	const auto assigned = assignDiscovery(player, discoveryId);
	if (assigned != WizardDiscoveryResult::SUCCESS && assigned != WizardDiscoveryResult::ALREADY_DISCOVERED) return nullptr;
	const auto* state = getDiscoveryState(player, discoveryId);
	return state ? g_wizardDiscoveries().getLocation(state->assignedLocationId) : nullptr;
}

WizardDiscoveryResult WizardDiscoverySystem::discover(const std::shared_ptr<Player> &player, const std::string &discoveryId, const Position &position, const bool adminOverride) {
	std::lock_guard lock(discoveryMutex);
	const auto* definition = g_wizardDiscoveries().getById(discoveryId);
	if (!player || !definition) return WizardDiscoveryResult::NOT_FOUND;
	if (hasDiscovery(player, discoveryId)) return WizardDiscoveryResult::ALREADY_DISCOVERED;
	if (!adminOverride && !isEligible(player, *definition)) return WizardDiscoveryResult::NOT_ELIGIBLE;
	const auto assigned = assignDiscovery(player, discoveryId, adminOverride);
	if (assigned != WizardDiscoveryResult::SUCCESS) return assigned;
	WizardDiscoveryResult positionFailure;
	if (!adminOverride && !validatePosition(player, *definition, position, positionFailure)) return positionFailure;

	const auto spellBackup = player->getWizardSpellProgressMap();
	const auto recipeBackup = player->getWizardRecipeProgressMap();
	const auto discoveryBackup = player->getWizardDiscoveryStateMap();
	auto applyInMemory = [&]() {
		if (!applyRewards(player, *definition, nullptr)) return false;
		auto &state = player->getOrCreateWizardDiscoveryState(discoveryId);
		state.state = WizardDiscoveryStateKind::DISCOVERED;
		state.discoveredAt = nowSeconds();
		state.rewardAppliedAt = state.discoveredAt;
		return true;
	};

	if (player->getGUID() == 0) return applyInMemory() ? WizardDiscoveryResult::SUCCESS : WizardDiscoveryResult::PERSISTENCE_ERROR;
	auto &db = g_database();
	bool alreadyDiscovered = false;
	const bool committed = DBTransaction::executeWithinTransactionRollbackOnFailure([&]() {
		const auto persisted = db.storeQuery(fmt::format(
			"SELECT `state` FROM `player_wizard_discoveries` WHERE `player_id` = {} AND `discovery_id` = {} FOR UPDATE",
			player->getGUID(), db.escapeString(discoveryId)
		));
		if (!persisted) return false;
		if (persisted->getString("state") == "DISCOVERED") { alreadyDiscovered = true; return true; }
		if (!applyRewards(player, *definition, &db)) return false;
		if (!db.executeQuery(fmt::format(
			"UPDATE `player_wizard_discoveries` SET `state` = 'DISCOVERED', `discovered_at` = CURRENT_TIMESTAMP, `reward_applied_at` = CURRENT_TIMESTAMP "
			"WHERE `player_id` = {} AND `discovery_id` = {} AND `state` = 'ASSIGNED'",
			player->getGUID(), db.escapeString(discoveryId)
		))) return false;
		auto &state = player->getOrCreateWizardDiscoveryState(discoveryId);
		state.state = WizardDiscoveryStateKind::DISCOVERED;
		state.discoveredAt = nowSeconds();
		state.rewardAppliedAt = state.discoveredAt;
		return true;
	});
	if (!committed || alreadyDiscovered) {
		player->replaceWizardSpellProgress(spellBackup);
		player->replaceWizardRecipeProgress(recipeBackup);
		player->replaceWizardDiscoveryStates(discoveryBackup);
		return alreadyDiscovered ? WizardDiscoveryResult::ALREADY_DISCOVERED : WizardDiscoveryResult::PERSISTENCE_ERROR;
	}
	return WizardDiscoveryResult::SUCCESS;
}

WizardDiscoveryResult WizardDiscoverySystem::interact(const std::shared_ptr<Player> &player, const uint16_t actionId, const Position &position) {
	const auto* definition = g_wizardDiscoveries().getByActionId(actionId);
	if (!definition || !definition->actionId || *definition->actionId != actionId || definition->type == WizardDiscoveryType::LOCATION || definition->type == WizardDiscoveryType::INGREDIENT) return WizardDiscoveryResult::INVALID_TRIGGER;
	return discover(player, definition->id, position);
}

WizardDiscoveryResult WizardDiscoverySystem::examineIngredient(const std::shared_ptr<Player> &player, const uint16_t itemId) {
	const auto* definition = g_wizardDiscoveries().getByIngredientItemId(itemId);
	if (!definition || definition->type != WizardDiscoveryType::INGREDIENT) return WizardDiscoveryResult::INVALID_TRIGGER;
	return discover(player, definition->id, player->getPosition());
}

void WizardDiscoverySystem::processLocation(const std::shared_ptr<Player> &player, const Position &position) {
	for (const auto &trigger : g_wizardDiscoveries().getLocationTriggers(position)) {
		const auto* location = g_wizardDiscoveries().getLocation(trigger.locationId);
		if (!location || !location->contains(position) || !canDiscover(player, trigger.discoveryId)) continue;
		const auto* assigned = getAssignedLocation(player, trigger.discoveryId);
		if (assigned && assigned->id == trigger.locationId) (void)discover(player, trigger.discoveryId, position);
	}
}

bool WizardDiscoverySystem::isPersonalObjectVisible(const std::shared_ptr<Player> &player, const std::shared_ptr<Item> &item, const Position &position) {
	if (!player || !item || !item->hasAttribute(ItemAttribute_t::ACTIONID)) return true;
	const auto* definition = g_wizardDiscoveries().getByActionId(item->getAttribute<uint16_t>(ItemAttribute_t::ACTIONID));
	if (!definition || !definition->personalObject) return true;
	if (!canDiscover(player, definition->id)) return false;
	const auto* assigned = getAssignedLocation(player, definition->id);
	return assigned && assigned->contains(position);
}

bool WizardDiscoverySystem::reset(const std::shared_ptr<Player> &player, const std::string &discoveryId, const bool clearAssignment) {
	std::lock_guard lock(discoveryMutex);
	if (!player || !g_wizardDiscoveries().getById(discoveryId)) return false;
	const auto* state = player->getWizardDiscoveryState(discoveryId);
	if (!state) return true;
	if (player->getGUID() != 0) {
		auto &db = g_database();
		const auto query = clearAssignment
			? fmt::format("DELETE FROM `player_wizard_discoveries` WHERE `player_id` = {} AND `discovery_id` = {}", player->getGUID(), db.escapeString(discoveryId))
			: fmt::format("UPDATE `player_wizard_discoveries` SET `state` = 'ASSIGNED', `discovered_at` = NULL, `reward_applied_at` = NULL WHERE `player_id` = {} AND `discovery_id` = {}", player->getGUID(), db.escapeString(discoveryId));
		if (!db.executeQuery(query)) return false;
	}
	if (clearAssignment) player->eraseWizardDiscoveryState(discoveryId);
	else {
		auto &mutableState = player->getOrCreateWizardDiscoveryState(discoveryId);
		mutableState.state = WizardDiscoveryStateKind::ASSIGNED;
		mutableState.discoveredAt = 0;
		mutableState.rewardAppliedAt = 0;
	}
	return true;
}

void WizardDiscoverySystem::setRandomIndexProviderForTests(std::function<size_t(size_t)> provider) { randomIndexProvider = std::move(provider); }
void WizardDiscoverySystem::resetRandomIndexProviderForTests() { randomIndexProvider = {}; }
