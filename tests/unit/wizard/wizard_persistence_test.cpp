#include "wizard/skills/wizard_skill.hpp"
#include "wizard/potions/wizard_recipe_progress.hpp"

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
	EXPECT_EQ(progress.masteryXp, 0);
	EXPECT_EQ(progress.knowledgeSources, 0);

	const WizardRecipeProgress recipe;
	EXPECT_EQ(recipe.knowledge, 0);
	EXPECT_EQ(recipe.mastery, 0);
	EXPECT_EQ(recipe.masteryXp, 0);
	EXPECT_EQ(recipe.brews, 0);
	EXPECT_FALSE(recipe.learned);
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

TEST(WizardProgressionConfigTest, RejectsInvalidLimitsCurvesXpAndCaps) {
	const auto productionPath = std::string(TESTS_SOURCE_DIR) + "/data/wizard/progression.json";
	std::ifstream input(productionPath);
	const auto valid = nlohmann::json::parse(input);
	auto expectInvalid = [&](nlohmann::json json, const std::string &name) {
		const auto path = std::filesystem::temp_directory_path() / ("wizard_invalid_" + name + ".json");
		std::ofstream output(path);
		output << json.dump();
		output.close();
		WizardProgressionConfig config;
		std::string error;
		EXPECT_FALSE(config.load(path.string(), error)) << name;
		EXPECT_FALSE(error.empty()) << name;
	};

	auto invalid = valid;
	invalid["knowledge"]["min"] = 80;
	invalid["knowledge"]["max"] = 20;
	expectInvalid(invalid, "limits");
	invalid = valid;
	invalid["spellMastery"]["bands"][0]["xpPerLevel"] = -1;
	expectInvalid(invalid, "negative_xp");
	invalid = valid;
	invalid["spellMastery"]["bands"][1]["throughLevel"] = invalid["spellMastery"]["bands"][0]["throughLevel"];
	expectInvalid(invalid, "overlap");
	invalid = valid;
	invalid["brewingMastery"]["bands"] = nlohmann::json::array();
	expectInvalid(invalid, "missing_bands");
	invalid = valid;
	invalid["spellEffect"]["maxCombinedPotencyBonus"] = -0.01;
	expectInvalid(invalid, "negative_cap");
	invalid = valid;
	invalid["potionEffect"]["maxCombinedBonus"] = 1.01;
	expectInvalid(invalid, "oversized_cap");
}
