#include "creatures/players/player.hpp"
#include "io/functions/iologindata_load_player.hpp"
#include "io/functions/iologindata_save_player.hpp"
#include "test_env.hpp"

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

		auto fresh = std::make_shared<Player>();
		fresh->setGUID(ids.defaultPlayerId);
		IOLoginDataLoad::loadPlayerWizardData(fresh);
		EXPECT_EQ(fresh->getWizardSkills().getMagicalPower(), 1);
		EXPECT_EQ(fresh->getWizardSkills().getMagicalControl(), 1);
		EXPECT_EQ(fresh->getWizardSkills().getMagicalKnowledge(), 1);
		EXPECT_EQ(fresh->getWizardSkills().getSkillCombat(), 1);
		EXPECT_TRUE(fresh->getWizardSpellProgressMap().empty());
	})();
}
