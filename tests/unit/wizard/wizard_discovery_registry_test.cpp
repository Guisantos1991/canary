#include "wizard/discovery/wizard_discovery_registry.hpp"
#include "wizard/potions/wizard_potion_registry.hpp"
#include "wizard/spells/wizard_spell_registry.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

namespace {
	nlohmann::json validDiscoveries() {
		std::ifstream input(std::string(TESTS_SOURCE_DIR) + "/data/wizard/discoveries.json");
		return nlohmann::json::parse(input);
	}

	std::filesystem::path writeDiscoveries(const nlohmann::json &json, const std::string &name) {
		const auto path = std::filesystem::temp_directory_path() / name;
		std::ofstream output(path);
		output << json.dump();
		return path;
	}

	void loadDependencies() {
		std::string error;
		ASSERT_TRUE(g_wizardSpells().load(std::string(TESTS_SOURCE_DIR) + "/data/wizard/spells.json", error)) << error;
		ASSERT_TRUE(g_wizardPotions().load(std::string(TESTS_SOURCE_DIR) + "/data/wizard/potions.json", error)) << error;
	}

	void expectInvalid(const nlohmann::json &json, const std::string &name) {
		WizardDiscoveryRegistry registry;
		std::string error;
		EXPECT_FALSE(registry.load(writeDiscoveries(json, "wizard_discovery_" + name + ".json").string(), error)) << name;
		EXPECT_FALSE(error.empty()) << name;
	}
}

TEST(WizardDiscoveryRegistryTest, LoadsValidDefinitionsAndIndexes) {
	loadDependencies();
	WizardDiscoveryRegistry registry;
	std::string error;
	ASSERT_TRUE(registry.load(std::string(TESTS_SOURCE_DIR) + "/data/wizard/discoveries.json", error)) << error;
	EXPECT_NE(registry.getById("dev_reading_book_01"), nullptr);
	EXPECT_EQ(registry.getByActionId(62001)->type, WizardDiscoveryType::BOOK);
	EXPECT_EQ(registry.getByIngredientItemId(266)->type, WizardDiscoveryType::INGREDIENT);
	EXPECT_TRUE(registry.getLocation("dev_hidden_area_a")->contains({ 32368, 32246, 7 }));
	EXPECT_FALSE(registry.getLocationTriggers({ 32368, 32246, 7 }).empty());
}

TEST(WizardDiscoveryRegistryTest, RejectsDuplicateAndUnknownDefinitionFields) {
	loadDependencies();
	auto duplicateId = validDiscoveries();
	duplicateId["discoveries"].push_back(duplicateId["discoveries"][0]);
	expectInvalid(duplicateId, "duplicate_id");
	auto duplicateLocation = validDiscoveries();
	duplicateLocation["locations"].push_back(duplicateLocation["locations"][0]);
	expectInvalid(duplicateLocation, "duplicate_location");
	auto unknownType = validDiscoveries();
	unknownType["discoveries"][1]["type"] = "QUEST";
	expectInvalid(unknownType, "unknown_type");
	auto unknownSource = validDiscoveries();
	unknownSource["discoveries"][1]["progressionSource"] = "COMBAT";
	expectInvalid(unknownSource, "unknown_source");
	auto unknownReward = validDiscoveries();
	unknownReward["discoveries"][1]["rewards"][0]["type"] = "GOLD";
	expectInvalid(unknownReward, "unknown_reward");
}

TEST(WizardDiscoveryRegistryTest, RejectsInvalidRewardReferencesAndAmounts) {
	loadDependencies();
	auto unknownSpell = validDiscoveries();
	unknownSpell["discoveries"][1]["rewards"][0]["spellId"] = 999999;
	expectInvalid(unknownSpell, "unknown_spell");
	auto negativeReward = validDiscoveries();
	negativeReward["discoveries"][1]["rewards"][0]["amount"] = -1;
	expectInvalid(negativeReward, "negative_reward");
	auto unknownRecipe = validDiscoveries();
	unknownRecipe["discoveries"][1]["rewards"][2]["recipeId"] = 999999;
	expectInvalid(unknownRecipe, "unknown_recipe");
}

TEST(WizardDiscoveryRegistryTest, RejectsInvalidRequirementsAndCycles) {
	loadDependencies();
	auto unknown = validDiscoveries();
	unknown["discoveries"][9]["requirements"]["allOf"] = { "missing" };
	expectInvalid(unknown, "unknown_requirement");
	auto self = validDiscoveries();
	self["discoveries"][9]["requirements"]["allOf"] = { "dev_chain_c" };
	expectInvalid(self, "self_dependency");
	auto cycle = validDiscoveries();
	cycle["discoveries"][7]["requirements"] = { { "allOf", { "dev_chain_c" } } };
	expectInvalid(cycle, "dependency_cycle");
}

TEST(WizardDiscoveryRegistryTest, RejectsInvalidLocationsAndRandomPools) {
	loadDependencies();
	auto invalidPosition = validDiscoveries();
	invalidPosition["locations"][0]["position"]["x"] = 0;
	expectInvalid(invalidPosition, "invalid_position");
	auto emptyPool = validDiscoveries();
	emptyPool["discoveries"][3]["placement"]["locationPool"] = nlohmann::json::array();
	expectInvalid(emptyPool, "empty_pool");
	auto unknownPoolLocation = validDiscoveries();
	unknownPoolLocation["discoveries"][3]["placement"]["locationPool"] = { "missing" };
	expectInvalid(unknownPoolLocation, "unknown_pool_location");
}
