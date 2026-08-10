#pragma once

#include "wizard/spells/wizard_spell_definition.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>

class WizardSpellRegistry {
public:
	WizardSpellRegistry() = default;
	static WizardSpellRegistry &getInstance();

	bool load(const std::string &path, std::string &error);
	[[nodiscard]] const WizardSpellDefinition* getById(uint32_t id) const;
	[[nodiscard]] const WizardSpellDefinition* getByName(const std::string &name) const;
	[[nodiscard]] const WizardSpellDefinition* getByIncantation(const std::string &incantation) const;
	[[nodiscard]] size_t size() const;
	void clear();

private:
	std::unordered_map<uint32_t, WizardSpellDefinition> spellsById;
	std::unordered_map<std::string, uint32_t> idsByName;
	std::unordered_map<std::string, uint32_t> idsByIncantation;
};

constexpr auto g_wizardSpells = WizardSpellRegistry::getInstance;
