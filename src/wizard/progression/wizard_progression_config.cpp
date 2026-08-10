#include "wizard/progression/wizard_progression_config.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

namespace {
	bool validReduction(double value) {
		return value >= 0.0 && value <= 1.0;
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
