#include "creatures/players/player.hpp"
#include "io/functions/iologindata_load_player.hpp"
#include "io/functions/iologindata_save_player.hpp"
#include "test_env.hpp"
#include "wizard/discovery/wizard_discovery_registry.hpp"
#include "wizard/discovery/wizard_discovery_system.hpp"
#include "wizard/potions/wizard_potion_registry.hpp"
#include "wizard/potions/wizard_recipe_knowledge_system.hpp"
#include "wizard/progression/wizard_knowledge_system.hpp"
#include "wizard/progression/wizard_progression_config.hpp"
#include "wizard/spells/wizard_spell_registry.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

namespace {
	struct WizardTestIds {
		uint32_t accountId;
		uint32_t playerId;
		uint32_t defaultPlayerId;
	};

	WizardTestIds nextWizardTestIds() {
		static std::atomic<uint32_t> counter { 1 };
		static const uint32_t base = (static_cast<uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()) & 0x3FFFFFFF) + 20000000;
		const auto index = counter.fetch_add(3);
		return { base + index, base + index + 1, base + index + 2 };
	}

	void createWizardPlayers(Database &db, const WizardTestIds &ids) {
		ASSERT_TRUE(db.executeQuery(fmt::format(
			"INSERT INTO `accounts` (`id`, `name`, `password`, `email`) VALUES ({}, 'wizard_acc_{}', '', 'wizard@test.com')",
			ids.accountId,
			ids.accountId
		)));
		ASSERT_TRUE(db.executeQuery(fmt::format(
			"INSERT INTO `players` (`id`, `name`, `account_id`, `conditions`) VALUES "
			"({}, 'wizard_player_{}', {}, ''), ({}, 'wizard_default_{}', {}, '')",
			ids.playerId,
			ids.playerId,
			ids.accountId,
			ids.defaultPlayerId,
			ids.defaultPlayerId,
			ids.accountId
		)));
	}

	class WizardPersistenceDBTest : public ::testing::Test { };
}

TEST_F(WizardPersistenceDBTest, SkillsAndSpellProgressRoundTripThroughDatabase) {
	auto &db = g_database();
	databaseTest(db, [&db] {
		const auto ids = nextWizardTestIds();
		createWizardPlayers(db, ids);

		auto saved = std::make_shared<Player>();
		saved->setGUID(ids.playerId);
		saved->setWizardSkill(WizardSkill::MAGICAL_POWER, 73);
		saved->setWizardSkill(WizardSkill::MAGICAL_CONTROL, 54);
		saved->setWizardSkill(WizardSkill::MAGICAL_KNOWLEDGE, 81);
		saved->setWizardSkill(WizardSkill::SKILL_COMBAT, 67);
		auto &ignis = saved->getOrCreateWizardSpellProgress(9001);
		ignis.knowledge = 42;
		ignis.mastery = 37;
		ignis.learned = true;
		ignis.uses = 9876;
		ignis.masteryXp = 4321;
		ignis.knowledgeSources = wizardKnowledgeSourceBit(WizardKnowledgeSource::READING) | wizardKnowledgeSourceBit(WizardKnowledgeSource::EXPLORATION);
		auto &recipe = saved->getOrCreateWizardRecipeProgress(7001);
		recipe.knowledge = 88;
		recipe.mastery = 45;
		recipe.masteryXp = 3456;
		recipe.knowledgeSources = wizardKnowledgeSourceBit(WizardKnowledgeSource::EXPLORATION);
		recipe.learned = true;
		recipe.brews = 789;
		ASSERT_TRUE(IOLoginDataSave::savePlayerWizardData(saved));

		auto loaded = std::make_shared<Player>();
		loaded->setGUID(ids.playerId);
		IOLoginDataLoad::loadPlayerWizardData(loaded);
		EXPECT_EQ(loaded->getWizardSkills().getMagicalPower(), 73);
		EXPECT_EQ(loaded->getWizardSkills().getMagicalControl(), 54);
		EXPECT_EQ(loaded->getWizardSkills().getMagicalKnowledge(), 81);
		EXPECT_EQ(loaded->getWizardSkills().getSkillCombat(), 67);
		const auto* loadedIgnis = loaded->getWizardSpellProgress(9001);
		ASSERT_NE(loadedIgnis, nullptr);
		EXPECT_EQ(loadedIgnis->knowledge, 42);
		EXPECT_EQ(loadedIgnis->mastery, 37);
		EXPECT_TRUE(loadedIgnis->learned);
		EXPECT_EQ(loadedIgnis->uses, 9876);
		EXPECT_EQ(loadedIgnis->masteryXp, 4321);
		EXPECT_EQ(loadedIgnis->knowledgeSources, ignis.knowledgeSources);
		const auto* loadedRecipe = loaded->getWizardRecipeProgress(7001);
		ASSERT_NE(loadedRecipe, nullptr);
		EXPECT_EQ(loadedRecipe->knowledge, 88);
		EXPECT_EQ(loadedRecipe->mastery, 45);
		EXPECT_EQ(loadedRecipe->masteryXp, 3456);
		EXPECT_EQ(loadedRecipe->knowledgeSources, recipe.knowledgeSources);
		EXPECT_TRUE(loadedRecipe->learned);
		EXPECT_EQ(loadedRecipe->brews, 789);

		auto fresh = std::make_shared<Player>();
		fresh->setGUID(ids.defaultPlayerId);
		IOLoginDataLoad::loadPlayerWizardData(fresh);
		EXPECT_EQ(fresh->getWizardSkills().getMagicalPower(), 1);
		EXPECT_EQ(fresh->getWizardSkills().getMagicalControl(), 1);
		EXPECT_EQ(fresh->getWizardSkills().getMagicalKnowledge(), 1);
		EXPECT_EQ(fresh->getWizardSkills().getSkillCombat(), 1);
		EXPECT_TRUE(fresh->getWizardSpellProgressMap().empty());
		EXPECT_TRUE(fresh->getWizardRecipeProgressMap().empty());
	})();
}

TEST_F(WizardPersistenceDBTest, DiscoveryAssignmentClaimAndReconnectArePersistentAndIdempotent) {
	auto &db = g_database();
	const auto ids = nextWizardTestIds();
	createWizardPlayers(db, ids);
	std::string error;
	ASSERT_TRUE(g_wizardProgression().load(std::string(TESTS_SOURCE_DIR) + "/data/wizard/progression.json", error)) << error;
	ASSERT_TRUE(g_wizardSpells().load(std::string(TESTS_SOURCE_DIR) + "/data/wizard/spells.json", error)) << error;
	ASSERT_TRUE(g_wizardPotions().load(std::string(TESTS_SOURCE_DIR) + "/data/wizard/potions.json", error)) << error;
	std::ifstream input(std::string(TESTS_SOURCE_DIR) + "/data/wizard/discoveries.json");
	auto json = nlohmann::json::parse(input);
	for (auto &location : json["locations"]) {
		if (location.contains("position")) location["position"] = { { "x", 1 }, { "y", 1 }, { "z", 0 } };
		else location["area"] = { { "from", { { "x", 1 }, { "y", 1 }, { "z", 0 } } }, { "to", { { "x", 1 }, { "y", 1 }, { "z", 0 } } } };
	}
	const auto path = std::filesystem::temp_directory_path() / "wizard_discovery_db_it.json";
	std::ofstream output(path);
	output << json.dump();
	output.close();
	ASSERT_TRUE(g_wizardDiscoveries().load(path.string(), error)) << error;
	WizardDiscoverySystem::setRandomIndexProviderForTests([](size_t) { return 2; });

	auto player = std::make_shared<Player>();
	player->setGUID(ids.playerId);
	ASSERT_EQ(WizardDiscoverySystem::assignDiscovery(player, "dev_secret_page_01"), WizardDiscoveryResult::SUCCESS);
	ASSERT_EQ(WizardDiscoverySystem::getAssignedLocation(player, "dev_secret_page_01")->id, "dev_secret_c");
	ASSERT_EQ(WizardDiscoverySystem::interact(player, 62003, { 1, 1, 0 }), WizardDiscoveryResult::SUCCESS);
	EXPECT_EQ(WizardKnowledgeSystem::getKnowledge(player, 9002), 20);
	ASSERT_EQ(WizardDiscoverySystem::interact(player, 62001, { 1, 1, 0 }), WizardDiscoveryResult::SUCCESS);
	EXPECT_EQ(WizardKnowledgeSystem::getKnowledge(player, 9001), 15);
	EXPECT_EQ(WizardRecipeKnowledgeSystem::getKnowledge(player, 7001), 5);

	const auto persisted = db.storeQuery(fmt::format(
		"SELECT `state`, `assigned_location_id`, `assigned_at`, `discovered_at`, `reward_applied_at` FROM `player_wizard_discoveries` "
		"WHERE `player_id` = {} AND `discovery_id` = 'dev_secret_page_01'",
		ids.playerId
	));
	ASSERT_NE(persisted, nullptr);
	EXPECT_EQ(persisted->getString("state"), "DISCOVERED");
	EXPECT_EQ(persisted->getString("assigned_location_id"), "dev_secret_c");
	EXPECT_FALSE(persisted->getString("assigned_at").empty());
	EXPECT_FALSE(persisted->getString("discovered_at").empty());
	EXPECT_FALSE(persisted->getString("reward_applied_at").empty());

	auto reconnected = std::make_shared<Player>();
	reconnected->setGUID(ids.playerId);
	IOLoginDataLoad::loadPlayerWizardData(reconnected);
	ASSERT_EQ(WizardDiscoverySystem::getAssignedLocation(reconnected, "dev_secret_page_01")->id, "dev_secret_c");
	EXPECT_TRUE(WizardDiscoverySystem::hasDiscovery(reconnected, "dev_secret_page_01"));
	EXPECT_EQ(WizardKnowledgeSystem::getKnowledge(reconnected, 9002), 20);
	EXPECT_EQ(WizardDiscoverySystem::interact(reconnected, 62003, { 1, 1, 0 }), WizardDiscoveryResult::ALREADY_DISCOVERED);
	EXPECT_EQ(WizardKnowledgeSystem::getKnowledge(reconnected, 9002), 20);
	EXPECT_EQ(WizardKnowledgeSystem::getKnowledge(reconnected, 9001), 15);
	EXPECT_EQ(WizardRecipeKnowledgeSystem::getKnowledge(reconnected, 7001), 5);
	EXPECT_EQ(WizardDiscoverySystem::interact(reconnected, 62001, { 1, 1, 0 }), WizardDiscoveryResult::ALREADY_DISCOVERED);
	EXPECT_EQ(WizardKnowledgeSystem::getKnowledge(reconnected, 9001), 15);
	EXPECT_EQ(WizardRecipeKnowledgeSystem::getKnowledge(reconnected, 7001), 5);

	WizardDiscoverySystem::resetRandomIndexProviderForTests();
	ASSERT_TRUE(db.executeQuery(fmt::format("DELETE FROM `accounts` WHERE `id` = {}", ids.accountId)));
}
