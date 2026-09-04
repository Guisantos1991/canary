#include "wizard/progression/wizard_progression_config.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

namespace {
	bool validReduction(double value) {
		return value >= 0.0 && value <= 1.0;
	}

	WizardValueLimits parseLimits(const nlohmann::json &json) {
		return { json.at("min").get<uint16_t>(), json.at("max").get<uint16_t>() };
	}

	uint64_t parsePositiveXp(const nlohmann::json &json, const char* field) {
		if (!json.is_number_integer()) {
			throw std::runtime_error(std::string(field) + " must be a positive integer");
		}
		if (json.is_number_unsigned()) {
			const auto value = json.get<uint64_t>();
			if (value == 0) throw std::runtime_error(std::string(field) + " must be positive");
			return value;
		}
		const auto value = json.get<int64_t>();
		if (value <= 0) throw std::runtime_error(std::string(field) + " must be positive");
		return static_cast<uint64_t>(value);
	}

	WizardMasteryCurveConfig parseCurve(const nlohmann::json &json) {
		WizardMasteryCurveConfig curve;
		curve.limits = parseLimits(json);
		for (const auto &band : json.at("bands")) {
			curve.bands.push_back({ band.at("throughLevel").get<uint16_t>(), parsePositiveXp(band.at("xpPerLevel"), "xpPerLevel") });
		}
		return curve;
	}

	bool validLimits(const WizardValueLimits &limits) {
		return limits.min <= limits.max && limits.max <= 100;
	}

	bool validCurve(const WizardMasteryCurveConfig &curve, std::string &error, const std::string &name) {
		if (!validLimits(curve.limits) || curve.limits.min != 0 || curve.limits.max == 0 || curve.bands.empty()) {
			error = name + " must cover a non-empty 0..max domain";
			return false;
		}
		uint16_t previous = 0;
		uint64_t previousXp = 0;
		for (const auto &band : curve.bands) {
			if (band.throughLevel <= previous || band.throughLevel > curve.limits.max || band.xpPerLevel == 0 || band.xpPerLevel < previousXp) {
				error = name + " bands must be ordered, non-overlapping, progressive, and use positive XP";
				return false;
			}
			previous = band.throughLevel;
			previousXp = band.xpPerLevel;
		}
		if (previous != curve.limits.max) {
			error = name + " bands must end at the configured maximum";
			return false;
		}
		return true;
	}
}

WizardProgressionConfig &WizardProgressionConfig::getInstance() {
	static WizardProgressionConfig instance;
	return instance;
}

bool WizardProgressionConfig::load(const std::string &path, std::string &error) {
	try {
		std::ifstream input(path);
		if (!input) {
			error = "cannot open " + path;
			return false;
		}

		const auto json = nlohmann::json::parse(input);
		WizardProgressionConfigData candidate;
		const auto &skills = json.at("skills");
		candidate.skills.min = skills.at("min").get<uint16_t>();
		candidate.skills.max = skills.at("max").get<uint16_t>();

		const auto &mana = json.at("mana");
		candidate.mana.base = mana.at("base").get<uint32_t>();
		candidate.mana.powerMultiplier = mana.at("powerMultiplier").get<uint32_t>();
		candidate.mana.controlMultiplier = mana.at("controlMultiplier").get<uint32_t>();
		candidate.mana.controlMaxReduction = mana.at("controlMaxReduction").get<double>();
		candidate.mana.masteryMaxReduction = mana.at("masteryMaxReduction").get<double>();
		candidate.mana.maxTotalReduction = mana.at("maxTotalReduction").get<double>();

		const auto &recovery = json.at("recovery");
		candidate.recovery.defaultMs = recovery.at("defaultMs").get<uint32_t>();
		candidate.recovery.minimumMs = recovery.at("minimumMs").get<uint32_t>();
		candidate.recovery.controlMaxReduction = recovery.at("controlMaxReduction").get<double>();
		candidate.recovery.masteryMaxReduction = recovery.at("masteryMaxReduction").get<double>();
		candidate.recovery.maxTotalReduction = recovery.at("maxTotalReduction").get<double>();

		const auto &cast = json.at("cast");
		candidate.cast.combatSkillMaxReduction = cast.at("combatSkillMaxReduction").get<double>();
		candidate.cast.masteryMaxReduction = cast.value("masteryMaxReduction", 0.05);

		candidate.knowledge = parseLimits(json.at("knowledge"));
		candidate.spellMastery = parseCurve(json.at("spellMastery"));
		const auto &meaningfulUse = json.at("meaningfulCombatUse");
		candidate.meaningfulUse.baseXp = parsePositiveXp(meaningfulUse.at("baseXp"), "meaningfulCombatUse.baseXp");
		candidate.meaningfulUse.additionalTargetBonus = meaningfulUse.at("additionalTargetBonus").get<uint64_t>();
		candidate.meaningfulUse.bonusTargetCap = meaningfulUse.at("bonusTargetCap").get<uint16_t>();
		const auto &spellEffect = json.at("spellEffect");
		candidate.spellEffect.masteryMaxPotencyBonus = spellEffect.at("masteryMaxPotencyBonus").get<double>();
		candidate.spellEffect.controlMaxPotencyBonus = spellEffect.at("controlMaxPotencyBonus").get<double>();
		candidate.spellEffect.maxCombinedPotencyBonus = spellEffect.at("maxCombinedPotencyBonus").get<double>();

		candidate.recipeKnowledge = parseLimits(json.at("recipeKnowledge"));
		candidate.brewingMastery = parseCurve(json.at("brewingMastery"));
		const auto &brewing = json.at("brewing");
		candidate.brewing.defaultIngredientQuality = brewing.at("defaultIngredientQuality").get<uint16_t>();
		candidate.brewing.xpPerValidBrew = parsePositiveXp(brewing.at("xpPerValidBrew"), "brewing.xpPerValidBrew");
		const auto &quality = json.at("potionQuality");
		candidate.potionQuality.ingredientWeight = quality.at("ingredientWeight").get<double>();
		candidate.potionQuality.controlMaxBonus = quality.at("controlMaxBonus").get<double>();
		candidate.potionQuality.masteryMaxBonus = quality.at("masteryMaxBonus").get<double>();
		candidate.potionQuality.min = quality.at("min").get<uint16_t>();
		candidate.potionQuality.max = quality.at("max").get<uint16_t>();
		const auto &potionEffect = json.at("potionEffect");
		candidate.potionEffect.qualityMaxBonus = potionEffect.at("qualityMaxBonus").get<double>();
		candidate.potionEffect.controlMaxBonus = potionEffect.at("controlMaxBonus").get<double>();
		candidate.potionEffect.masteryMaxBonus = potionEffect.at("masteryMaxBonus").get<double>();
		candidate.potionEffect.linkedSpellMaxBonus = potionEffect.at("linkedSpellMaxBonus").get<double>();
		candidate.potionEffect.maxCombinedBonus = potionEffect.at("maxCombinedBonus").get<double>();

		if (candidate.skills.min < 1 || candidate.skills.max > 100 || candidate.skills.min > candidate.skills.max) {
			error = "skills must define a valid range inside 1..100";
			return false;
		}
		if (candidate.mana.base == 0 || !validReduction(candidate.mana.controlMaxReduction) || !validReduction(candidate.mana.masteryMaxReduction) || !validReduction(candidate.mana.maxTotalReduction) || candidate.mana.controlMaxReduction + candidate.mana.masteryMaxReduction > candidate.mana.maxTotalReduction + 0.000001) {
			error = "mana reductions are invalid or exceed maxTotalReduction";
			return false;
		}
		if (candidate.recovery.defaultMs == 0 || candidate.recovery.minimumMs == 0 || candidate.recovery.minimumMs > candidate.recovery.defaultMs || !validReduction(candidate.recovery.controlMaxReduction) || !validReduction(candidate.recovery.masteryMaxReduction) || !validReduction(candidate.recovery.maxTotalReduction) || candidate.recovery.controlMaxReduction + candidate.recovery.masteryMaxReduction > candidate.recovery.maxTotalReduction + 0.000001) {
			error = "recovery configuration is invalid";
			return false;
		}
		if (!validReduction(candidate.cast.combatSkillMaxReduction) || !validReduction(candidate.cast.masteryMaxReduction)) {
			error = "cast reductions must be between 0 and 1";
			return false;
		}
		if (!validLimits(candidate.knowledge) || !validLimits(candidate.recipeKnowledge)) {
			error = "knowledge limits must define valid ranges inside 0..100";
			return false;
		}
		if (!validCurve(candidate.spellMastery, error, "spellMastery") || !validCurve(candidate.brewingMastery, error, "brewingMastery")) {
			return false;
		}
		if (candidate.meaningfulUse.baseXp == 0 || candidate.meaningfulUse.bonusTargetCap == 0) {
			error = "meaningful combat XP and target cap must be positive";
			return false;
		}
		if (!validReduction(candidate.spellEffect.masteryMaxPotencyBonus) || !validReduction(candidate.spellEffect.controlMaxPotencyBonus) || !validReduction(candidate.spellEffect.maxCombinedPotencyBonus)) {
			error = "spell effect caps must be between 0 and 1";
			return false;
		}
		if (candidate.brewing.defaultIngredientQuality > 100 || candidate.brewing.xpPerValidBrew == 0 || candidate.potionQuality.min > candidate.potionQuality.max || candidate.potionQuality.max > 100 || !validReduction(candidate.potionQuality.ingredientWeight) || candidate.potionQuality.controlMaxBonus < 0 || candidate.potionQuality.controlMaxBonus > 100 || candidate.potionQuality.masteryMaxBonus < 0 || candidate.potionQuality.masteryMaxBonus > 100) {
			error = "brewing or potion quality configuration is invalid";
			return false;
		}
		if (!validReduction(candidate.potionEffect.qualityMaxBonus) || !validReduction(candidate.potionEffect.controlMaxBonus) || !validReduction(candidate.potionEffect.masteryMaxBonus) || !validReduction(candidate.potionEffect.linkedSpellMaxBonus) || !validReduction(candidate.potionEffect.maxCombinedBonus)) {
			error = "potion effect caps must be between 0 and 1";
			return false;
		}

		data = candidate;
		loaded = true;
		return true;
	} catch (const std::exception &exception) {
		error = exception.what();
		return false;
	}
}

const WizardProgressionConfigData &WizardProgressionConfig::get() const {
	return data;
}

bool WizardProgressionConfig::isLoaded() const {
	return loaded;
}
