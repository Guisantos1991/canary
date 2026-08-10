#include "wizard/skills/wizard_skill.hpp"

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
