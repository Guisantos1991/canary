#include "wizard/skills/wizard_skill.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

TEST(WizardPersistenceTest, DefaultPlayerWizardValuesAreStable) {
	const WizardSkillSnapshot skills;
	EXPECT_EQ(skills.getMagicalPower(), 1);
	EXPECT_EQ(skills.getMagicalControl(), 1);
	EXPECT_EQ(skills.getMagicalKnowledge(), 1);
	EXPECT_EQ(skills.getSkillCombat(), 1);

	const WizardSpellProgress progress;
	EXPECT_EQ(progress.knowledge, 0);
	EXPECT_EQ(progress.mastery, 0);
	EXPECT_FALSE(progress.learned);
	EXPECT_EQ(progress.uses, 0);
}

TEST(WizardPersistenceTest, LoadedValuesClampToDomain) {
	WizardSkillSnapshot skills { 44, 55, 66, 77 };
	EXPECT_EQ(skills.getMagicalPower(), 44);
	EXPECT_EQ(skills.getMagicalControl(), 55);
	EXPECT_EQ(skills.getMagicalKnowledge(), 66);
	EXPECT_EQ(skills.getSkillCombat(), 77);
	WizardSpellProgress progress { 12, 34, true, 567 };
	EXPECT_EQ(progress.knowledge, 12);
	EXPECT_EQ(progress.mastery, 34);
	EXPECT_TRUE(progress.learned);
	EXPECT_EQ(progress.uses, 567);
}

TEST(WizardPersistenceTest, SkillLimitsComeFromProgressionConfiguration) {
	const auto productionPath = std::string(TESTS_SOURCE_DIR) + "/data/wizard/progression.json";
	std::ifstream input(productionPath);
	auto json = nlohmann::json::parse(input);
	json["skills"]["min"] = 10;
	json["skills"]["max"] = 50;
	const auto customPath = std::filesystem::temp_directory_path() / "wizard_progression_custom_limits.json";
	std::ofstream output(customPath);
	output << json.dump();
	output.close();

	auto &config = g_wizardProgression();
	std::string error;
	ASSERT_TRUE(config.load(customPath.string(), error)) << error;
	WizardSkillSnapshot skills;
	for (const auto skill : { WizardSkill::MAGICAL_POWER, WizardSkill::MAGICAL_CONTROL, WizardSkill::MAGICAL_KNOWLEDGE, WizardSkill::SKILL_COMBAT }) {
		skills.set(skill, 1);
		EXPECT_EQ(skills.get(skill), 10);
		skills.set(skill, 100);
		EXPECT_EQ(skills.get(skill), 50);
	}

	ASSERT_TRUE(config.load(productionPath, error)) << error;
}
