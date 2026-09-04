#include "creatures/players/player.hpp"
#include "items/item.hpp"
#include "items/tile.hpp"
#include "wizard/discovery/wizard_discovery_registry.hpp"
#include "wizard/discovery/wizard_discovery_system.hpp"
#include "wizard/potions/wizard_potion_registry.hpp"
#include "wizard/potions/wizard_recipe_knowledge_system.hpp"
#include "wizard/progression/wizard_knowledge_system.hpp"
#include "wizard/progression/wizard_progression_config.hpp"
#include "wizard/spells/wizard_spell_registry.hpp"
#include "test_items.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <thread>

namespace {
	class WizardDiscoverySystemTest : public ::testing::Test {
	protected:
		void SetUp() override {
			TestItems::init();
			std::string error;
			ASSERT_TRUE(g_wizardProgression().load(std::string(TESTS_SOURCE_DIR) + "/data/wizard/progression.json", error)) << error;
			ASSERT_TRUE(g_wizardSpells().load(std::string(TESTS_SOURCE_DIR) + "/data/wizard/spells.json", error)) << error;
			ASSERT_TRUE(g_wizardPotions().load(std::string(TESTS_SOURCE_DIR) + "/data/wizard/potions.json", error)) << error;
			std::ifstream input(std::string(TESTS_SOURCE_DIR) + "/data/wizard/discoveries.json");
			auto json = nlohmann::json::parse(input);
			for (auto &location : json["locations"]) {
				const auto id = location.at("id").get<std::string>();
				const auto x = (id == "dev_secret_a" || id == "dev_hidden_area_a") ? 2 : (id == "dev_secret_c" || id == "dev_hidden_area_c") ? 3 : 1;
				if (location.contains("position")) location["position"] = { { "x", x }, { "y", 1 }, { "z", 0 } };
				else location["area"] = { { "from", { { "x", x }, { "y", 1 }, { "z", 0 } } }, { "to", { { "x", x }, { "y", 1 }, { "z", 0 } } } };
			}
			const auto path = std::filesystem::temp_directory_path() / "wizard_discovery_system.json";
			std::ofstream output(path);
			output << json.dump();
			output.close();
			ASSERT_TRUE(g_wizardDiscoveries().load(path.string(), error)) << error;
			WizardDiscoverySystem::setRandomIndexProviderForTests([](size_t) { return 1; });
		}

		void TearDown() override {
			WizardDiscoverySystem::resetRandomIndexProviderForTests();
		}
	};
}

TEST_F(WizardDiscoverySystemTest, BookIsReadableOneShotAndUsesExistingKnowledgeRules) {
	auto player = std::make_shared<Player>();
	EXPECT_EQ(WizardDiscoverySystem::interact(player, 62001, { 1, 1, 0 }), WizardDiscoveryResult::SUCCESS);
	EXPECT_EQ(WizardKnowledgeSystem::getKnowledge(player, 9001), 15);
	EXPECT_EQ(WizardKnowledgeSystem::getKnowledge(player, 9002), 0);
	EXPECT_EQ(WizardRecipeKnowledgeSystem::getKnowledge(player, 7001), 5);
	EXPECT_TRUE(WizardDiscoverySystem::hasDiscovery(player, "dev_clue_sealed_fire_room"));
	EXPECT_EQ(WizardDiscoverySystem::interact(player, 62001, { 1, 1, 0 }), WizardDiscoveryResult::ALREADY_DISCOVERED);
	EXPECT_EQ(WizardKnowledgeSystem::getKnowledge(player, 9001), 15);
	EXPECT_EQ(WizardRecipeKnowledgeSystem::getKnowledge(player, 7001), 5);
}

TEST_F(WizardDiscoverySystemTest, PlaqueAndWorldObjectRemainOneShotAndExplorationOnlyWorks) {
	auto player = std::make_shared<Player>();
	EXPECT_EQ(WizardDiscoverySystem::interact(player, 62002, { 1, 1, 0 }), WizardDiscoveryResult::SUCCESS);
	EXPECT_EQ(WizardKnowledgeSystem::getKnowledge(player, 9001), 10);
	EXPECT_EQ(WizardDiscoverySystem::interact(player, 62004, { 1, 1, 0 }), WizardDiscoveryResult::SUCCESS);
	EXPECT_EQ(WizardKnowledgeSystem::getKnowledge(player, 9002), 15);
	EXPECT_EQ(WizardDiscoverySystem::interact(player, 62004, { 1, 1, 0 }), WizardDiscoveryResult::ALREADY_DISCOVERED);
	EXPECT_EQ(WizardKnowledgeSystem::getKnowledge(player, 9002), 15);
}

TEST_F(WizardDiscoverySystemTest, PlayerRandomAssignmentIsCuratedStableAndIndependent) {
	auto first = std::make_shared<Player>();
	auto second = std::make_shared<Player>();
	const auto* firstLocation = WizardDiscoverySystem::getAssignedLocation(first, "dev_secret_page_01");
	ASSERT_NE(firstLocation, nullptr);
	EXPECT_EQ(firstLocation->id, "dev_secret_b");
	EXPECT_EQ(WizardDiscoverySystem::getAssignedLocation(first, "dev_secret_page_01")->id, "dev_secret_b");
	EXPECT_EQ(WizardDiscoverySystem::getAssignedLocation(second, "dev_secret_page_01")->id, "dev_secret_b");
	EXPECT_NE(first->getWizardDiscoveryState("dev_secret_page_01"), second->getWizardDiscoveryState("dev_secret_page_01"));
	auto corrupt = std::make_shared<Player>();
	corrupt->getOrCreateWizardDiscoveryState("dev_secret_page_01").assignedLocationId = "removed_location";
	EXPECT_EQ(WizardDiscoverySystem::assignDiscovery(corrupt, "dev_secret_page_01"), WizardDiscoveryResult::CORRUPT_ASSIGNMENT);
	EXPECT_EQ(corrupt->getWizardDiscoveryState("dev_secret_page_01")->assignedLocationId, "removed_location");
}

TEST_F(WizardDiscoverySystemTest, FixedPlacementIsTheSameForEveryPlayer) {
	auto first = std::make_shared<Player>();
	auto second = std::make_shared<Player>();
	ASSERT_NE(WizardDiscoverySystem::getAssignedLocation(first, "dev_reading_book_01"), nullptr);
	ASSERT_NE(WizardDiscoverySystem::getAssignedLocation(second, "dev_reading_book_01"), nullptr);
	EXPECT_EQ(WizardDiscoverySystem::getAssignedLocation(first, "dev_reading_book_01")->id, "dev_book_position");
	EXPECT_EQ(WizardDiscoverySystem::getAssignedLocation(second, "dev_reading_book_01")->id, "dev_book_position");
}

TEST_F(WizardDiscoverySystemTest, PersonalSecretVisibilityDisappearsForOnlyDiscoveringPlayer) {
	auto first = std::make_shared<Player>();
	auto second = std::make_shared<Player>();
	auto page = std::make_shared<Item>(100);
	page->setAttribute(ItemAttribute_t::ACTIONID, static_cast<uint16_t>(62003));
	EXPECT_FALSE(page->canBeMoved());
	auto tile = std::make_shared<DynamicTile>(1, 1, 0);
	tile->internalAddThing(page);
	EXPECT_TRUE(WizardDiscoverySystem::isPersonalObjectVisible(first, page, { 1, 1, 0 }));
	EXPECT_TRUE(WizardDiscoverySystem::isPersonalObjectVisible(second, page, { 1, 1, 0 }));
	EXPECT_EQ(tile->getStackposOfItem(first, page), 0);
	EXPECT_EQ(tile->getUseItem(0, first), page);
	EXPECT_EQ(WizardDiscoverySystem::interact(first, 62003, { 1, 1, 0 }), WizardDiscoveryResult::SUCCESS);
	EXPECT_FALSE(WizardDiscoverySystem::isPersonalObjectVisible(first, page, { 1, 1, 0 }));
	EXPECT_TRUE(WizardDiscoverySystem::isPersonalObjectVisible(second, page, { 1, 1, 0 }));
	EXPECT_EQ(tile->getStackposOfItem(first, page), -1);
	EXPECT_EQ(tile->getUseItem(0, first), nullptr);
	EXPECT_EQ(tile->getStackposOfItem(second, page), 0);
	EXPECT_EQ(tile->getUseItem(0, second), page);
}

TEST_F(WizardDiscoverySystemTest, AdminResetDoesNotRollbackProgressionAndOnlyExplicitlyClearsAssignment) {
	auto player = std::make_shared<Player>();
	ASSERT_EQ(WizardDiscoverySystem::interact(player, 62002, { 1, 1, 0 }), WizardDiscoveryResult::SUCCESS);
	ASSERT_TRUE(WizardDiscoverySystem::reset(player, "dev_fire_plaque_01", false));
	ASSERT_NE(WizardDiscoverySystem::getDiscoveryState(player, "dev_fire_plaque_01"), nullptr);
	EXPECT_EQ(WizardDiscoverySystem::getDiscoveryState(player, "dev_fire_plaque_01")->state, WizardDiscoveryStateKind::ASSIGNED);
	EXPECT_EQ(WizardKnowledgeSystem::getKnowledge(player, 9001), 10);
	ASSERT_TRUE(WizardDiscoverySystem::reset(player, "dev_fire_plaque_01", true));
	EXPECT_EQ(WizardDiscoverySystem::getDiscoveryState(player, "dev_fire_plaque_01"), nullptr);
	EXPECT_EQ(WizardKnowledgeSystem::getKnowledge(player, 9001), 10);
}

TEST_F(WizardDiscoverySystemTest, LocationRequiresPersonalAssignedAreaAndRunsOnce) {
	auto player = std::make_shared<Player>();
	const auto* assigned = WizardDiscoverySystem::getAssignedLocation(player, "dev_found_hidden_area");
	ASSERT_NE(assigned, nullptr);
	EXPECT_EQ(assigned->id, "dev_hidden_area_b");
	WizardDiscoverySystem::processLocation(player, { 2, 1, 0 });
	EXPECT_FALSE(WizardDiscoverySystem::hasDiscovery(player, "dev_found_hidden_area"));
	WizardDiscoverySystem::processLocation(player, { 1, 1, 0 });
	EXPECT_TRUE(WizardDiscoverySystem::hasDiscovery(player, "dev_found_hidden_area"));
	const auto knowledge = WizardKnowledgeSystem::getKnowledge(player, 9001);
	WizardDiscoverySystem::processLocation(player, { 1, 1, 0 });
	EXPECT_EQ(WizardKnowledgeSystem::getKnowledge(player, 9001), knowledge);
}

TEST_F(WizardDiscoverySystemTest, ChainRequiresAllDiscoveries) {
	auto player = std::make_shared<Player>();
	EXPECT_EQ(WizardDiscoverySystem::interact(player, 62007, { 1, 1, 0 }), WizardDiscoveryResult::NOT_ELIGIBLE);
	EXPECT_EQ(WizardDiscoverySystem::interact(player, 62005, { 1, 1, 0 }), WizardDiscoveryResult::SUCCESS);
	EXPECT_EQ(WizardDiscoverySystem::interact(player, 62007, { 1, 1, 0 }), WizardDiscoveryResult::NOT_ELIGIBLE);
	EXPECT_EQ(WizardDiscoverySystem::interact(player, 62006, { 1, 1, 0 }), WizardDiscoveryResult::SUCCESS);
	EXPECT_EQ(WizardDiscoverySystem::interact(player, 62007, { 1, 1, 0 }), WizardDiscoveryResult::SUCCESS);
	EXPECT_EQ(WizardKnowledgeSystem::getKnowledge(player, 9001), 5);
}

TEST_F(WizardDiscoverySystemTest, IngredientExaminationIsOneShot) {
	auto player = std::make_shared<Player>();
	EXPECT_EQ(WizardDiscoverySystem::examineIngredient(player, 266), WizardDiscoveryResult::SUCCESS);
	EXPECT_EQ(WizardRecipeKnowledgeSystem::getKnowledge(player, 7001), 5);
	EXPECT_EQ(WizardDiscoverySystem::examineIngredient(player, 266), WizardDiscoveryResult::ALREADY_DISCOVERED);
	EXPECT_EQ(WizardRecipeKnowledgeSystem::getKnowledge(player, 7001), 5);
	EXPECT_EQ(WizardDiscoverySystem::examineIngredient(player, 9999), WizardDiscoveryResult::INVALID_TRIGGER);
}

TEST_F(WizardDiscoverySystemTest, RejectsForgedWrongRemoteAndUnassignedClaims) {
	auto player = std::make_shared<Player>();
	EXPECT_EQ(WizardDiscoverySystem::interact(player, 61999, { 1, 1, 0 }), WizardDiscoveryResult::INVALID_TRIGGER);
	EXPECT_EQ(WizardDiscoverySystem::interact(player, 62002, { 100, 100, 0 }), WizardDiscoveryResult::TOO_FAR);
	EXPECT_EQ(WizardDiscoverySystem::discover(player, "missing", { 1, 1, 0 }), WizardDiscoveryResult::NOT_FOUND);
	WizardDiscoverySystem::setRandomIndexProviderForTests([](size_t) { return 0; });
	EXPECT_EQ(WizardDiscoverySystem::interact(player, 62003, { 1, 1, 0 }), WizardDiscoveryResult::WRONG_LOCATION);
}

TEST_F(WizardDiscoverySystemTest, ConcurrentDuplicateAttemptRewardsOnce) {
	auto player = std::make_shared<Player>();
	WizardDiscoveryResult first = WizardDiscoveryResult::NOT_FOUND;
	WizardDiscoveryResult second = WizardDiscoveryResult::NOT_FOUND;
	std::thread a([&] { first = WizardDiscoverySystem::interact(player, 62002, { 1, 1, 0 }); });
	std::thread b([&] { second = WizardDiscoverySystem::interact(player, 62002, { 1, 1, 0 }); });
	a.join();
	b.join();
	EXPECT_TRUE((first == WizardDiscoveryResult::SUCCESS && second == WizardDiscoveryResult::ALREADY_DISCOVERED)
		|| (second == WizardDiscoveryResult::SUCCESS && first == WizardDiscoveryResult::ALREADY_DISCOVERED));
	EXPECT_EQ(WizardKnowledgeSystem::getKnowledge(player, 9001), 10);
}
