#include "creatures/players/player.hpp"
#include "wizard/progression/wizard_knowledge_system.hpp"
#include "wizard/progression/wizard_learning_system.hpp"
#include "wizard/progression/wizard_progression_config.hpp"
#include "wizard/spells/wizard_spell_registry.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

namespace {
	void loadSpellProgressionFixtures() {
		std::string error;
		ASSERT_TRUE(g_wizardProgression().load(std::string(TESTS_SOURCE_DIR) + "/data/wizard/progression.json", error)) << error;
		ASSERT_TRUE(g_wizardSpells().load(std::string(TESTS_SOURCE_DIR) + "/data/wizard/spells.json", error)) << error;
	}

	std::shared_ptr<Player> progressionPlayer() {
		auto player = std::make_shared<Player>();
		player->setWizardSkill(WizardSkill::MAGICAL_KNOWLEDGE, 100);
		return player;
	}
}

TEST(WizardKnowledgeSystemTest, DefaultIncrementAndClampAreStable) {
	loadSpellProgressionFixtures();
	auto player = progressionPlayer();
	EXPECT_EQ(WizardKnowledgeSystem::getKnowledge(player, 9001), 0);
	EXPECT_EQ(WizardKnowledgeSystem::addKnowledge(player, 9001, 25, WizardKnowledgeSource::READING), WizardKnowledgeResult::SUCCESS);
	EXPECT_EQ(WizardKnowledgeSystem::getKnowledge(player, 9001), 25);
	EXPECT_EQ(WizardKnowledgeSystem::addKnowledge(player, 9001, -1000, WizardKnowledgeSource::READING), WizardKnowledgeResult::SUCCESS);
	EXPECT_EQ(WizardKnowledgeSystem::getKnowledge(player, 9001), 0);
	EXPECT_EQ(WizardKnowledgeSystem::setKnowledge(player, 9001, 1000, WizardKnowledgeSource::EXPLORATION), WizardKnowledgeResult::SUCCESS);
	EXPECT_EQ(WizardKnowledgeSystem::getKnowledge(player, 9001), 100);
}

TEST(WizardKnowledgeSystemTest, AllowedAndForbiddenSourcesAreDataDriven) {
	loadSpellProgressionFixtures();
	auto player = progressionPlayer();
	EXPECT_EQ(WizardKnowledgeSystem::addKnowledge(player, 9001, 10, WizardKnowledgeSource::READING), WizardKnowledgeResult::SUCCESS);
	EXPECT_EQ(WizardKnowledgeSystem::addKnowledge(player, 9001, 10, WizardKnowledgeSource::EXPLORATION), WizardKnowledgeResult::SUCCESS);
	EXPECT_EQ(WizardKnowledgeSystem::addKnowledge(player, 9001, 10, WizardKnowledgeSource::EXPERIMENTATION), WizardKnowledgeResult::SOURCE_NOT_ALLOWED);
	EXPECT_EQ(WizardKnowledgeSystem::getKnowledge(player, 9001), 20);
}

TEST(WizardKnowledgeSystemTest, KnowledgeNeverChangesMasteryUsesOrLearnedState) {
	loadSpellProgressionFixtures();
	auto player = progressionPlayer();
	auto &progress = player->getOrCreateWizardSpellProgress(9001);
	progress.mastery = 42;
	progress.masteryXp = 1234;
	progress.uses = 99;
	ASSERT_EQ(WizardKnowledgeSystem::setKnowledge(player, 9001, 100, WizardKnowledgeSource::READING), WizardKnowledgeResult::SUCCESS);
	EXPECT_FALSE(progress.learned);
	EXPECT_EQ(progress.mastery, 42);
	EXPECT_EQ(progress.masteryXp, 1234);
	EXPECT_EQ(progress.uses, 99);
}

TEST(WizardLearningSystemTest, ReportsEveryRequirementAndLearnsExplicitly) {
	loadSpellProgressionFixtures();
	auto player = std::make_shared<Player>();
	EXPECT_EQ(WizardLearningSystem::learn(player, 999999), WizardLearningResult::SPELL_NOT_FOUND);
	EXPECT_EQ(WizardLearningSystem::learn(player, 9001), WizardLearningResult::INSUFFICIENT_SPELL_KNOWLEDGE);
	ASSERT_EQ(WizardKnowledgeSystem::setKnowledge(player, 9001, 100, WizardKnowledgeSource::READING), WizardKnowledgeResult::SUCCESS);
	EXPECT_EQ(WizardLearningSystem::learn(player, 9001), WizardLearningResult::INSUFFICIENT_MAGICAL_KNOWLEDGE);
	player->setWizardSkill(WizardSkill::MAGICAL_KNOWLEDGE, 20);
	EXPECT_TRUE(WizardLearningSystem::isLearnable(player, 9001));
	EXPECT_EQ(WizardLearningSystem::learn(player, 9001), WizardLearningResult::SUCCESS);
	EXPECT_TRUE(player->hasLearnedWizardSpell(9001));
	EXPECT_EQ(WizardLearningSystem::learn(player, 9001), WizardLearningResult::ALREADY_LEARNED);
}

TEST(WizardLearningSystemTest, RequiredSourceHistoryIsEnforcedAndPersistable) {
	loadSpellProgressionFixtures();
	std::ifstream input(std::string(TESTS_SOURCE_DIR) + "/data/wizard/spells.json");
	auto json = nlohmann::json::parse(input);
	json["spells"][0]["progression"]["requiredKnowledgeSources"] = { "EXPLORATION" };
	const auto path = std::filesystem::temp_directory_path() / "wizard_required_source.json";
	std::ofstream output(path);
	output << json.dump();
	output.close();
	std::string error;
	ASSERT_TRUE(g_wizardSpells().load(path.string(), error)) << error;
	auto player = progressionPlayer();
	ASSERT_EQ(WizardKnowledgeSystem::setKnowledge(player, 9001, 100, WizardKnowledgeSource::READING), WizardKnowledgeResult::SUCCESS);
	EXPECT_EQ(WizardLearningSystem::learn(player, 9001), WizardLearningResult::SOURCE_REQUIREMENT_NOT_MET);
	ASSERT_EQ(WizardKnowledgeSystem::addKnowledge(player, 9001, 1, WizardKnowledgeSource::EXPLORATION), WizardKnowledgeResult::SUCCESS);
	EXPECT_EQ(WizardLearningSystem::learn(player, 9001), WizardLearningResult::SUCCESS);
	ASSERT_TRUE(g_wizardSpells().load(std::string(TESTS_SOURCE_DIR) + "/data/wizard/spells.json", error)) << error;
}

TEST(WizardSpellRegistryTest, SupportsReadingOnlyExplorationOnlyAndHybridDefinitions) {
	std::ifstream input(std::string(TESTS_SOURCE_DIR) + "/data/wizard/spells.json");
	auto json = nlohmann::json::parse(input);
	auto reading = json["spells"][0];
	reading["id"] = 99002; reading["name"] = "Reading Dev"; reading["incantation"] = "reading dev";
	reading["progression"]["acquisitionProfile"] = "ACADEMIC";
	reading["progression"]["allowedKnowledgeSources"] = { "READING", "ADMIN", "SYSTEM" };
	auto exploration = json["spells"][0];
	exploration["id"] = 99003; exploration["name"] = "Exploration Dev"; exploration["incantation"] = "exploration dev";
	exploration["progression"]["acquisitionProfile"] = "EXPLORATION";
	exploration["progression"]["allowedKnowledgeSources"] = { "EXPLORATION", "ADMIN", "SYSTEM" };
	json["spells"].push_back(reading);
	json["spells"].push_back(exploration);
	const auto path = std::filesystem::temp_directory_path() / "wizard_profiles.json";
	std::ofstream output(path); output << json.dump(); output.close();
	WizardSpellRegistry registry;
	std::string error;
	ASSERT_TRUE(registry.load(path.string(), error)) << error;
	EXPECT_TRUE(wizardHasKnowledgeSource(registry.getById(99002)->progression.allowedKnowledgeSources, WizardKnowledgeSource::READING));
	EXPECT_FALSE(wizardHasKnowledgeSource(registry.getById(99002)->progression.allowedKnowledgeSources, WizardKnowledgeSource::EXPLORATION));
	EXPECT_TRUE(wizardHasKnowledgeSource(registry.getById(99003)->progression.allowedKnowledgeSources, WizardKnowledgeSource::EXPLORATION));
	EXPECT_FALSE(wizardHasKnowledgeSource(registry.getById(99003)->progression.allowedKnowledgeSources, WizardKnowledgeSource::READING));
	EXPECT_EQ(registry.getById(9001)->progression.acquisitionProfile, WizardAcquisitionProfile::HYBRID);
}
