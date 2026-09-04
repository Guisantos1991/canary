#include "wizard/spells/wizard_spell_registry.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

namespace {
	nlohmann::json validRegistry() {
		std::ifstream input(std::string(TESTS_SOURCE_DIR) + "/data/wizard/spells.json");
		return nlohmann::json::parse(input);
	}

	std::filesystem::path writeRegistry(const nlohmann::json &json, const std::string &name) {
		const auto path = std::filesystem::temp_directory_path() / name;
		std::ofstream output(path);
		output << json.dump();
		return path;
	}
}

TEST(WizardSpellRegistryTest, LoadsValidJsonAndFindsAllIndexes) {
	WizardSpellRegistry registry;
	std::string error;
	ASSERT_TRUE(registry.load(std::string(TESTS_SOURCE_DIR) + "/data/wizard/spells.json", error)) << error;
	ASSERT_NE(registry.getById(9001), nullptr);
	EXPECT_EQ(registry.getByName("IGNIS")->id, 9001);
	EXPECT_EQ(registry.getByIncantation("IgNiS")->id, 9001);
	EXPECT_EQ(registry.getById(999999), nullptr);
}

TEST(WizardSpellRegistryTest, RejectsDuplicateIndexesAndInvalidArea) {
	for (const std::string field : { "id", "name", "incantation" }) {
		auto json = validRegistry();
		auto duplicate = json["spells"][0];
		if (field != "id") {
			duplicate["id"] = 9002;
		}
		if (field != "name") {
			duplicate["name"] = "Ignis Two";
		}
		if (field != "incantation") {
			duplicate["incantation"] = "ignis two";
		}
		json["spells"].push_back(duplicate);
		WizardSpellRegistry registry;
		std::string error;
		EXPECT_FALSE(registry.load(writeRegistry(json, "wizard_duplicate_" + field + ".json").string(), error));
		EXPECT_NE(error.find("duplicate"), std::string::npos);
	}

	auto json = validRegistry();
	json["spells"][0]["area"]["minSquares"] = 13;
	json["spells"][0]["area"]["maxSquares"] = 12;
	WizardSpellRegistry registry;
	std::string error;
	EXPECT_FALSE(registry.load(writeRegistry(json, "wizard_invalid_area.json").string(), error));
	EXPECT_NE(error.find("area"), std::string::npos);
}

TEST(WizardSpellRegistryTest, RejectsDamageForUnsupportedCategories) {
	for (const std::string category : { "DEFENSIVE", "UTILITY", "CONTROL" }) {
		auto json = validRegistry();
		json["spells"][0]["category"] = category;
		WizardSpellRegistry registry;
		std::string error;
		EXPECT_FALSE(registry.load(writeRegistry(json, "wizard_damaging_" + category + ".json").string(), error));
		EXPECT_NE(error.find("only OFFENSIVE"), std::string::npos) << error;
	}
}

TEST(WizardSpellRegistryTest, AcceptsNonDamagingNonOffensiveDefinition) {
	auto json = validRegistry();
	json["spells"][0]["category"] = "UTILITY";
	json["spells"][0]["minPower"] = 0;
	json["spells"][0]["maxPower"] = 0;
	WizardSpellRegistry registry;
	std::string error;
	EXPECT_TRUE(registry.load(writeRegistry(json, "wizard_utility_no_damage.json").string(), error)) << error;
}

TEST(WizardSpellRegistryTest, RejectsUnsupportedCustomArea) {
	auto json = validRegistry();
	json["spells"][0]["area"]["pattern"] = "CUSTOM";
	WizardSpellRegistry registry;
	std::string error;
	EXPECT_FALSE(registry.load(writeRegistry(json, "wizard_custom_area.json").string(), error));
	EXPECT_NE(error.find("CUSTOM is unsupported"), std::string::npos) << error;
}

TEST(WizardSpellRegistryTest, RejectsInvalidKnowledgeSource) {
	auto json = validRegistry();
	json["spells"][0]["progression"]["allowedKnowledgeSources"] = { "READING", "COMBAT" };
	WizardSpellRegistry registry;
	std::string error;
	EXPECT_FALSE(registry.load(writeRegistry(json, "wizard_invalid_knowledge_source.json").string(), error));
	EXPECT_NE(error.find("invalid spell knowledge source"), std::string::npos) << error;
}
