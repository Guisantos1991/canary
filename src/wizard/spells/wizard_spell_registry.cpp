#include "wizard/spells/wizard_spell_registry.hpp"

#include "utils/tools.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

namespace {
	template <typename Enum>
	Enum parseEnum(const std::string &value, const std::unordered_map<std::string, Enum> &values, const std::string &field) {
		const auto normalized = asUpperCaseString(value);
		const auto found = values.find(normalized);
		if (found == values.end()) {
			throw std::runtime_error("invalid " + field + ": " + value);
		}
		return found->second;
	}

	WizardSpellDefinition parseSpell(const nlohmann::json &json) {
		WizardSpellDefinition spell;
		spell.id = json.at("id").get<uint32_t>();
		spell.name = json.at("name").get<std::string>();
		spell.incantation = json.at("incantation").get<std::string>();
		spell.element = parseEnum<WizardElement>(json.at("element").get<std::string>(), {
			{ "ARCANE", WizardElement::ARCANE }, { "FIRE", WizardElement::FIRE }, { "ICE", WizardElement::ICE },
			{ "EARTH", WizardElement::EARTH }, { "AIR", WizardElement::AIR }, { "LIGHT", WizardElement::LIGHT }, { "DARK", WizardElement::DARK }
		}, "element");
		spell.category = parseEnum<WizardSpellCategory>(json.at("category").get<std::string>(), {
			{ "OFFENSIVE", WizardSpellCategory::OFFENSIVE }, { "DEFENSIVE", WizardSpellCategory::DEFENSIVE },
			{ "UTILITY", WizardSpellCategory::UTILITY }, { "CONTROL", WizardSpellCategory::CONTROL }
		}, "category");
		spell.targetType = parseEnum<WizardTargetType>(json.at("targetType").get<std::string>(), {
			{ "TILE", WizardTargetType::TILE }, { "AREA", WizardTargetType::AREA }, { "SELF", WizardTargetType::SELF }, { "DIRECTION", WizardTargetType::DIRECTION }
		}, "targetType");

		spell.manaCost = json.at("manaCost").get<uint32_t>();
		spell.castTimeMs = json.at("castTimeMs").get<uint32_t>();
		spell.recoveryTimeMs = json.at("recoveryTimeMs").get<uint32_t>();
		spell.cooldownMs = json.at("cooldownMs").get<uint32_t>();
		spell.range = json.at("range").get<uint16_t>();
		spell.minPower = json.at("minPower").get<int32_t>();
		spell.maxPower = json.at("maxPower").get<int32_t>();
		spell.impactEffect = json.value("impactEffect", 0);
		spell.requiresLineOfSight = json.at("requiresLineOfSight").get<bool>();
		spell.learnable = json.at("learnable").get<bool>();
		spell.darkMagic = json.at("darkMagic").get<bool>();
		spell.difficulty = json.value("difficulty", 0);
		spell.requiredKnowledge = json.value("requiredKnowledge", spell.difficulty);
		spell.specialScript = json.value("specialScript", "");

		const auto &area = json.at("area");
		spell.area.pattern = parseEnum<WizardAreaPattern>(area.at("pattern").get<std::string>(), {
			{ "NONE", WizardAreaPattern::NONE }, { "CIRCLE", WizardAreaPattern::CIRCLE }, { "CROSS", WizardAreaPattern::CROSS },
			{ "CONE", WizardAreaPattern::CONE }, { "LINE", WizardAreaPattern::LINE }, { "RING", WizardAreaPattern::RING },
			{ "WAVE", WizardAreaPattern::WAVE }, { "CUSTOM", WizardAreaPattern::CUSTOM }
		}, "area.pattern");
		spell.area.minSquares = area.at("minSquares").get<uint16_t>();
		spell.area.maxSquares = area.at("maxSquares").get<uint16_t>();

		if (const auto projectile = json.find("projectile"); projectile != json.end()) {
			spell.projectile.visualEffect = projectile->value("visualEffect", 0);
			spell.projectile.travelTimeMs = projectile->value("travelTimeMs", 0);
		}
		return spell;
	}

	void validateSpell(const WizardSpellDefinition &spell) {
		if (spell.id < 9000) {
			throw std::runtime_error("wizard spell id must be in the reserved range starting at 9000");
		}
		if (spell.name.empty() || spell.incantation.empty()) {
			throw std::runtime_error("name and incantation cannot be empty");
		}
		if (spell.range == 0 || spell.range > 50) {
			throw std::runtime_error("range must be between 1 and 50");
		}
		if (spell.manaCost == 0) {
			throw std::runtime_error("manaCost must be greater than zero");
		}
		if (spell.recoveryTimeMs == 0 || spell.cooldownMs == 0) {
			throw std::runtime_error("recoveryTimeMs and cooldownMs must be greater than zero");
		}
		if (spell.minPower < 0 || spell.maxPower < spell.minPower) {
			throw std::runtime_error("minPower/maxPower damage range is invalid");
		}
		if (spell.category == WizardSpellCategory::OFFENSIVE) {
			if (spell.minPower <= 0 || spell.maxPower <= 0) {
				throw std::runtime_error("OFFENSIVE spells require a positive minPower/maxPower damage range");
			}
		} else if (spell.minPower != 0 || spell.maxPower != 0) {
			throw std::runtime_error("only OFFENSIVE spells may define minPower/maxPower damage");
		}
		if (spell.area.minSquares == 0 || spell.area.maxSquares == 0 || spell.area.minSquares > spell.area.maxSquares) {
			throw std::runtime_error("area requires 1 <= minSquares <= maxSquares");
		}
		if (spell.area.pattern == WizardAreaPattern::CUSTOM) {
			throw std::runtime_error("area.pattern CUSTOM is unsupported until a custom pattern is defined");
		}
		if (spell.requiredKnowledge > 100 || spell.difficulty > 100) {
			throw std::runtime_error("difficulty and requiredKnowledge must be in 0..100");
		}
	}
}

WizardSpellRegistry &WizardSpellRegistry::getInstance() {
	static WizardSpellRegistry instance;
	return instance;
}

bool WizardSpellRegistry::load(const std::string &path, std::string &error) {
	try {
		std::ifstream input(path);
		if (!input) {
			error = "cannot open " + path;
			return false;
		}
		const auto root = nlohmann::json::parse(input);
		const auto &entries = root.is_array() ? root : root.at("spells");
		if (!entries.is_array()) {
			throw std::runtime_error("root 'spells' must be an array");
		}

		WizardSpellRegistry candidate;
		for (const auto &entry : entries) {
			auto spell = parseSpell(entry);
			validateSpell(spell);
			const auto normalizedName = asLowerCaseString(spell.name);
			const auto normalizedIncantation = asLowerCaseString(spell.incantation);
			if (candidate.spellsById.contains(spell.id)) {
				throw std::runtime_error("duplicate spell id " + std::to_string(spell.id));
			}
			if (!candidate.idsByName.emplace(normalizedName, spell.id).second) {
				throw std::runtime_error("duplicate spell name " + spell.name);
			}
			if (!candidate.idsByIncantation.emplace(normalizedIncantation, spell.id).second) {
				throw std::runtime_error("duplicate spell incantation " + spell.incantation);
			}
			candidate.spellsById.emplace(spell.id, std::move(spell));
		}
		if (candidate.spellsById.empty()) {
			throw std::runtime_error("registry contains no spells");
		}
		*this = std::move(candidate);
		return true;
	} catch (const std::exception &exception) {
		error = exception.what();
		return false;
	}
}

const WizardSpellDefinition* WizardSpellRegistry::getById(const uint32_t id) const {
	const auto found = spellsById.find(id);
	return found == spellsById.end() ? nullptr : &found->second;
}

const WizardSpellDefinition* WizardSpellRegistry::getByName(const std::string &name) const {
	const auto found = idsByName.find(asLowerCaseString(name));
	return found == idsByName.end() ? nullptr : getById(found->second);
}

const WizardSpellDefinition* WizardSpellRegistry::getByIncantation(const std::string &incantation) const {
	const auto found = idsByIncantation.find(asLowerCaseString(incantation));
	return found == idsByIncantation.end() ? nullptr : getById(found->second);
}

size_t WizardSpellRegistry::size() const {
	return spellsById.size();
}

void WizardSpellRegistry::clear() {
	spellsById.clear();
	idsByName.clear();
	idsByIncantation.clear();
}
