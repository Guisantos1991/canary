#include "creatures/players/player.hpp"
#include "wizard/potions/wizard_brewing_mastery_system.hpp"
#include "wizard/potions/wizard_brewing_system.hpp"
#include "wizard/potions/wizard_potion_registry.hpp"
#include "wizard/potions/wizard_recipe_knowledge_system.hpp"
#include "wizard/potions/wizard_recipe_learning_system.hpp"
#include "wizard/progression/wizard_progression_config.hpp"
#include "wizard/spells/wizard_spell_registry.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

namespace {
	void loadPotionFixtures() {
		std::string error;
		ASSERT_TRUE(g_wizardProgression().load(std::string(TESTS_SOURCE_DIR) + "/data/wizard/progression.json", error)) << error;
		ASSERT_TRUE(g_wizardSpells().load(std::string(TESTS_SOURCE_DIR) + "/data/wizard/spells.json", error)) << error;
		ASSERT_TRUE(g_wizardPotions().load(std::string(TESTS_SOURCE_DIR) + "/data/wizard/potions.json", error)) << error;
	}

	std::vector<WizardIngredientInput> ingredients(const uint16_t quality) {
		return { { 266, 1, quality }, { 268, 2, quality } };
	}
}

TEST(WizardRecipeKnowledgeTest, DefaultsAddsClampsAndValidatesSources) {
	loadPotionFixtures();
	auto player = std::make_shared<Player>();
	EXPECT_EQ(WizardRecipeKnowledgeSystem::getKnowledge(player, 7001), 0);
	EXPECT_EQ(WizardRecipeKnowledgeSystem::addKnowledge(player, 7001, 30, WizardKnowledgeSource::READING), WizardRecipeKnowledgeResult::SUCCESS);
	EXPECT_EQ(WizardRecipeKnowledgeSystem::addKnowledge(player, 7001, 30, WizardKnowledgeSource::EXPLORATION), WizardRecipeKnowledgeResult::SUCCESS);
	EXPECT_EQ(WizardRecipeKnowledgeSystem::addKnowledge(player, 7001, 1000, WizardKnowledgeSource::EXPERIMENTATION), WizardRecipeKnowledgeResult::SUCCESS);
	EXPECT_EQ(WizardRecipeKnowledgeSystem::getKnowledge(player, 7001), 100);
	EXPECT_EQ(WizardRecipeKnowledgeSystem::setKnowledge(player, 7001, -1, WizardKnowledgeSource::READING), WizardRecipeKnowledgeResult::SUCCESS);
	EXPECT_EQ(WizardRecipeKnowledgeSystem::getKnowledge(player, 7001), 0);
	EXPECT_EQ(WizardRecipeKnowledgeSystem::addKnowledge(player, 7001, 1, static_cast<WizardKnowledgeSource>(99)), WizardRecipeKnowledgeResult::SOURCE_NOT_ALLOWED);
}

TEST(WizardPotionRegistryTest, RejectsInvalidKnowledgeSource) {
	loadPotionFixtures();
	std::ifstream input(std::string(TESTS_SOURCE_DIR) + "/data/wizard/potions.json");
	auto json = nlohmann::json::parse(input);
	json["recipes"][0]["progression"]["allowedKnowledgeSources"] = { "READING", "COMBAT" };
	const auto path = std::filesystem::temp_directory_path() / "wizard_invalid_recipe_source.json";
	std::ofstream output(path);
	output << json.dump();
	output.close();
	WizardPotionRegistry registry;
	std::string error;
	EXPECT_FALSE(registry.load(path.string(), error));
	EXPECT_NE(error.find("invalid recipe knowledge source"), std::string::npos) << error;
}

TEST(WizardRecipeLearningTest, KnowledgeDoesNotAutoLearnAndRequirementsAreExplicit) {
	loadPotionFixtures();
	auto player = std::make_shared<Player>();
	EXPECT_EQ(WizardRecipeLearningSystem::learn(player, 9999), WizardRecipeLearningResult::RECIPE_NOT_FOUND);
	ASSERT_EQ(WizardRecipeKnowledgeSystem::setKnowledge(player, 7001, 100, WizardKnowledgeSource::READING), WizardRecipeKnowledgeResult::SUCCESS);
	EXPECT_FALSE(player->hasLearnedWizardRecipe(7001));
	EXPECT_EQ(WizardRecipeLearningSystem::learn(player, 7001), WizardRecipeLearningResult::INSUFFICIENT_MAGICAL_KNOWLEDGE);
	player->setWizardSkill(WizardSkill::MAGICAL_KNOWLEDGE, 10);
	EXPECT_EQ(WizardRecipeLearningSystem::learn(player, 7001), WizardRecipeLearningResult::SUCCESS);
	EXPECT_EQ(WizardRecipeLearningSystem::learn(player, 7001), WizardRecipeLearningResult::ALREADY_LEARNED);
}

TEST(WizardBrewingMasteryTest, CurveIsProgressiveAndDebugSetSynchronizesXp) {
	loadPotionFixtures();
	EXPECT_EQ(WizardBrewingMasterySystem::calculateThreshold(20), 800);
	EXPECT_EQ(WizardBrewingMasterySystem::calculateThreshold(50), 3800);
	EXPECT_EQ(WizardBrewingMasterySystem::calculateThreshold(100), 27550);
	auto player = std::make_shared<Player>();
	player->getOrCreateWizardRecipeProgress(7001).learned = true;
	ASSERT_TRUE(WizardBrewingMasterySystem::setMastery(player, 7001, 95));
	EXPECT_EQ(player->getWizardRecipeProgress(7001)->masteryXp, WizardBrewingMasterySystem::calculateThreshold(95));
	ASSERT_TRUE(WizardBrewingMasterySystem::setMastery(player, 7001, 1000));
	EXPECT_EQ(player->getWizardRecipeProgress(7001)->mastery, 100);
}

TEST(WizardBrewingSystemTest, ValidBrewIncrementsBrewsAndXpInvalidBrewDoesNot) {
	loadPotionFixtures();
	auto player = std::make_shared<Player>();
	EXPECT_EQ(WizardBrewingSystem::brew(player, 7001, ingredients(50)).code, WizardBrewResultCode::RECIPE_NOT_LEARNED);
	EXPECT_EQ(player->getOrCreateWizardRecipeProgress(7001).brews, 0);
	player->getOrCreateWizardRecipeProgress(7001).learned = true;
	EXPECT_EQ(WizardBrewingSystem::brew(player, 7001, { { 266, 1, 50 } }).code, WizardBrewResultCode::INVALID_INGREDIENTS);
	EXPECT_EQ(player->getWizardRecipeProgress(7001)->masteryXp, 0);
	const auto result = WizardBrewingSystem::brew(player, 7001, ingredients(50));
	EXPECT_EQ(result.code, WizardBrewResultCode::SUCCESS);
	EXPECT_EQ(result.xpGranted, 10);
	EXPECT_EQ(player->getWizardRecipeProgress(7001)->brews, 1);
	EXPECT_EQ(player->getWizardRecipeProgress(7001)->masteryXp, 10);
}

TEST(WizardPotionQualityTest, IsDeterministicMonotonicAndCapped) {
	loadPotionFixtures();
	const auto* recipe = g_wizardPotions().getById(7001);
	ASSERT_NE(recipe, nullptr);
	const auto low = WizardBrewingSystem::calculateQuality(*recipe, ingredients(10), 1, 0);
	const auto repeated = WizardBrewingSystem::calculateQuality(*recipe, ingredients(10), 1, 0);
	const auto betterIngredients = WizardBrewingSystem::calculateQuality(*recipe, ingredients(90), 1, 0);
	const auto master = WizardBrewingSystem::calculateQuality(*recipe, ingredients(90), 100, 100);
	EXPECT_EQ(low, repeated);
	EXPECT_LT(low, betterIngredients);
	EXPECT_LT(betterIngredients, master);
	EXPECT_LE(master, 100);
	EXPECT_EQ(WizardBrewingSystem::calculateQuality(*recipe, { { 266, 1, 101 }, { 268, 2, 100 } }, 100, 100), 0);
}

TEST(WizardPotionEffectTest, QualityControlAndMasteryImproveWithinCombinedCap) {
	loadPotionFixtures();
	const auto* recipe = g_wizardPotions().getById(7001);
	ASSERT_NE(recipe, nullptr);
	const auto base = WizardBrewingSystem::calculateEffects(*recipe, 0, 1, 0);
	const auto quality = WizardBrewingSystem::calculateEffects(*recipe, 100, 1, 0);
	const auto control = WizardBrewingSystem::calculateEffects(*recipe, 0, 100, 0);
	const auto mastery = WizardBrewingSystem::calculateEffects(*recipe, 0, 1, 100);
	const auto combined = WizardBrewingSystem::calculateEffects(*recipe, 100, 100, 100);
	EXPECT_LT(base.potency, quality.potency);
	EXPECT_LT(base.potency, control.potency);
	EXPECT_LT(base.potency, mastery.potency);
	EXPECT_LE(combined.potency, recipe->baseEffects.potency * 1.25);
	EXPECT_GE(combined.durationMs, base.durationMs);
	EXPECT_LE(combined.stability, 100);
}

TEST(WizardPotionEffectTest, SpellMasteryOnlyAppliesToExplicitLinkedSpell) {
	loadPotionFixtures();
	auto common = *g_wizardPotions().getById(7001);
	const auto ignored = WizardBrewingSystem::calculateEffects(common, 0, 1, 0, 100);
	const auto commonBase = WizardBrewingSystem::calculateEffects(common, 0, 1, 0, 0);
	EXPECT_DOUBLE_EQ(ignored.potency, commonBase.potency);
	common.linkedSpellId = 9001;
	const auto linked = WizardBrewingSystem::calculateEffects(common, 0, 1, 0, 100);
	EXPECT_GT(linked.potency, commonBase.potency);
}
