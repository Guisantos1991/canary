#include "wizard/potions/wizard_potion_registry.hpp"

#include "utils/tools.hpp"
#include "wizard/spells/wizard_spell_registry.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <unordered_set>

namespace {
	WizardPotionDefinition parseRecipe(const nlohmann::json &json) {
		WizardPotionDefinition recipe;
		recipe.id = json.at("id").get<uint32_t>();
		recipe.name = json.at("name").get<std::string>();
		recipe.developmentFixture = json.value("developmentFixture", false);
		recipe.baseQuality = json.at("baseQuality").get<double>();
		if (json.contains("linkedSpellId") && !json.at("linkedSpellId").is_null()) recipe.linkedSpellId = json.at("linkedSpellId").get<uint32_t>();

		const auto &progression = json.at("progression");
		recipe.progression.knowledgeRequired = progression.at("knowledgeRequired").get<uint16_t>();
		recipe.progression.magicalKnowledgeRequired = progression.at("magicalKnowledgeRequired").get<uint16_t>();
		if (!parseWizardAcquisitionProfile(progression.at("acquisitionProfile").get<std::string>(), recipe.progression.acquisitionProfile)) throw std::runtime_error("invalid recipe acquisition profile");
		for (const auto &sourceName : progression.at("allowedKnowledgeSources")) {
			WizardKnowledgeSource source;
			if (!parseWizardKnowledgeSource(sourceName.get<std::string>(), source)) throw std::runtime_error("invalid recipe knowledge source: " + sourceName.get<std::string>());
			const auto bit = wizardKnowledgeSourceBit(source);
			if ((recipe.progression.allowedKnowledgeSources & bit) != 0) throw std::runtime_error("duplicate recipe knowledge source");
			recipe.progression.allowedKnowledgeSources |= bit;
		}
		for (const auto &sourceName : progression.value("requiredKnowledgeSources", nlohmann::json::array())) {
			WizardKnowledgeSource source;
			if (!parseWizardKnowledgeSource(sourceName.get<std::string>(), source)) throw std::runtime_error("invalid required recipe knowledge source");
			recipe.progression.requiredKnowledgeSources |= wizardKnowledgeSourceBit(source);
		}

		for (const auto &ingredient : json.at("ingredients")) recipe.ingredients.push_back({ ingredient.at("itemId").get<uint16_t>(), ingredient.at("amount").get<uint16_t>() });
		const auto &effects = json.at("baseEffects");
		recipe.baseEffects.potency = effects.at("potency").get<double>();
		recipe.baseEffects.durationMs = effects.at("durationMs").get<uint32_t>();
		recipe.baseEffects.yield = effects.at("yield").get<uint16_t>();
		recipe.baseEffects.stability = effects.at("stability").get<double>();
		return recipe;
	}

	void validateRecipe(const WizardPotionDefinition &recipe) {
		if (recipe.id == 0 || recipe.name.empty()) throw std::runtime_error("recipe id and name are required");
		if (recipe.progression.knowledgeRequired > 100 || recipe.progression.magicalKnowledgeRequired > 100 || recipe.progression.allowedKnowledgeSources == 0) throw std::runtime_error("recipe progression is invalid");
		if ((recipe.progression.requiredKnowledgeSources & ~recipe.progression.allowedKnowledgeSources) != 0) throw std::runtime_error("required recipe sources must also be allowed");
		if (recipe.ingredients.empty()) throw std::runtime_error("recipe requires at least one ingredient");
		std::unordered_set<uint16_t> itemIds;
		for (const auto &ingredient : recipe.ingredients) {
			if (ingredient.itemId == 0 || ingredient.amount == 0 || !itemIds.emplace(ingredient.itemId).second) throw std::runtime_error("recipe ingredients must be positive and unique");
		}
		if (recipe.baseQuality < 0 || recipe.baseQuality > 100 || recipe.baseEffects.potency < 0 || recipe.baseEffects.stability < 0 || recipe.baseEffects.yield == 0) throw std::runtime_error("recipe base quality/effects are invalid");
		if (recipe.linkedSpellId && !g_wizardSpells().getById(*recipe.linkedSpellId)) throw std::runtime_error("linkedSpellId does not exist");
	}
}

WizardPotionRegistry &WizardPotionRegistry::getInstance() {
	static WizardPotionRegistry instance;
	return instance;
}

bool WizardPotionRegistry::load(const std::string &path, std::string &error) {
	try {
		std::ifstream input(path);
		if (!input) { error = "cannot open " + path; return false; }
		const auto root = nlohmann::json::parse(input);
		const auto &entries = root.at("recipes");
		if (!entries.is_array()) throw std::runtime_error("root 'recipes' must be an array");
		WizardPotionRegistry candidate;
		for (const auto &entry : entries) {
			auto recipe = parseRecipe(entry);
			validateRecipe(recipe);
			if (candidate.recipesById.contains(recipe.id)) throw std::runtime_error("duplicate recipe id");
			if (!candidate.idsByName.emplace(asLowerCaseString(recipe.name), recipe.id).second) throw std::runtime_error("duplicate recipe name");
			candidate.recipesById.emplace(recipe.id, std::move(recipe));
		}
		if (candidate.recipesById.empty()) throw std::runtime_error("registry contains no recipes");
		*this = std::move(candidate);
		return true;
	} catch (const std::exception &exception) {
		error = exception.what();
		return false;
	}
}

const WizardPotionDefinition* WizardPotionRegistry::getById(const uint32_t id) const {
	const auto found = recipesById.find(id);
	return found == recipesById.end() ? nullptr : &found->second;
}

const WizardPotionDefinition* WizardPotionRegistry::getByName(const std::string &name) const {
	const auto found = idsByName.find(asLowerCaseString(name));
	return found == idsByName.end() ? nullptr : getById(found->second);
}

size_t WizardPotionRegistry::size() const { return recipesById.size(); }
void WizardPotionRegistry::clear() { recipesById.clear(); idsByName.clear(); }
