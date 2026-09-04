#include "creatures/players/player.hpp"
#include "wizard/combat/wizard_area_system.hpp"
#include "wizard/progression/wizard_mastery_system.hpp"
#include "wizard/progression/wizard_progression_config.hpp"
#include "wizard/progression/wizard_spell_effect_scaling_system.hpp"
#include "wizard/spells/wizard_spell_registry.hpp"

namespace {
	void loadMasteryFixtures() {
		std::string error;
		ASSERT_TRUE(g_wizardProgression().load(std::string(TESTS_SOURCE_DIR) + "/data/wizard/progression.json", error)) << error;
		ASSERT_TRUE(g_wizardSpells().load(std::string(TESTS_SOURCE_DIR) + "/data/wizard/spells.json", error)) << error;
	}
}

TEST(WizardMasterySystemTest, ProgressiveCurveMatchesConfiguredBands) {
	loadMasteryFixtures();
	const std::vector<uint16_t> levels { 0, 20, 50, 80, 95, 99, 100 };
	uint64_t previousThreshold = 0;
	for (const auto level : levels) {
		const auto threshold = WizardMasterySystem::calculateThreshold(level);
		EXPECT_GE(threshold, previousThreshold);
		EXPECT_EQ(WizardMasterySystem::calculateLevel(threshold), level);
		previousThreshold = threshold;
	}
	EXPECT_EQ(WizardMasterySystem::calculateThreshold(20), 1000);
	EXPECT_EQ(WizardMasterySystem::calculateThreshold(50), 4750);
	EXPECT_EQ(WizardMasterySystem::calculateThreshold(80), 13750);
	EXPECT_EQ(WizardMasterySystem::calculateThreshold(95), 25000);
	EXPECT_EQ(WizardMasterySystem::calculateThreshold(100), 32500);
}

TEST(WizardMasterySystemTest, DebugSetSynchronizesXpAndNeverExceedsMaximum) {
	loadMasteryFixtures();
	auto player = std::make_shared<Player>();
	ASSERT_TRUE(player->learnWizardSpell(9001));
	ASSERT_TRUE(WizardMasterySystem::setMastery(player, 9001, 99));
	EXPECT_EQ(player->getWizardSpellProgress(9001)->masteryXp, WizardMasterySystem::calculateThreshold(99));
	ASSERT_TRUE(WizardMasterySystem::setMastery(player, 9001, 999));
	EXPECT_EQ(player->getWizardSpellProgress(9001)->mastery, 100);
	WizardMasterySystem::addXp(player, 9001, std::numeric_limits<uint64_t>::max());
	EXPECT_EQ(player->getWizardSpellProgress(9001)->mastery, 100);
	EXPECT_EQ(player->getWizardSpellProgress(9001)->masteryXp, WizardMasterySystem::calculateThreshold(100));
}

TEST(WizardMasterySystemTest, MeaningfulUseIsOncePerCastCappedAndHasNoTimer) {
	loadMasteryFixtures();
	EXPECT_EQ(WizardMasterySystem::calculateMeaningfulUseXp(0), 0);
	EXPECT_EQ(WizardMasterySystem::calculateMeaningfulUseXp(1), 10);
	EXPECT_EQ(WizardMasterySystem::calculateMeaningfulUseXp(2), 12);
	EXPECT_EQ(WizardMasterySystem::calculateMeaningfulUseXp(4), 16);
	EXPECT_EQ(WizardMasterySystem::calculateMeaningfulUseXp(100), 16);
	auto player = std::make_shared<Player>();
	ASSERT_TRUE(player->learnWizardSpell(9001));
	for (int cast = 0; cast < 200; ++cast) WizardMasterySystem::grantMeaningfulUse(player, 9001, 1);
	EXPECT_EQ(player->getWizardSpellProgress(9001)->masteryXp, 2000);
}

TEST(WizardSpellEffectScalingTest, MasteryAndControlImprovePotencyWithinCombinedCap) {
	loadMasteryFixtures();
	const auto &config = g_wizardProgression().get().spellEffect;
	const auto low = WizardSpellEffectScalingSystem::scalePotency(100, 1, 0, config);
	const auto mastery = WizardSpellEffectScalingSystem::scalePotency(100, 1, 100, config);
	const auto control = WizardSpellEffectScalingSystem::scalePotency(100, 100, 0, config);
	const auto combined = WizardSpellEffectScalingSystem::scalePotency(100, 100, 100, config);
	EXPECT_LT(low, mastery);
	EXPECT_LT(low, control);
	EXPECT_LE(combined, 120);
}

TEST(WizardSpellEffectScalingTest, MasteryDoesNotChangeAreaAndPowerStillOwnsMaximum) {
	loadMasteryFixtures();
	const auto* spell = g_wizardSpells().getById(9001);
	ASSERT_NE(spell, nullptr);
	const auto power99 = WizardAreaSystem::calculateEffectiveSquares(99, spell->area.minSquares, spell->area.maxSquares);
	const auto power100 = WizardAreaSystem::calculateEffectiveSquares(100, spell->area.minSquares, spell->area.maxSquares);
	EXPECT_LT(power99, spell->area.maxSquares);
	EXPECT_EQ(power100, spell->area.maxSquares);
	EXPECT_EQ(WizardAreaSystem::resolve(spell->area, Position { 100, 100, 7 }, 99, DIRECTION_NORTH).size(), power99);
}
