#pragma once

#include "wizard/potions/wizard_potion_definition.hpp"

#include <string>
#include <unordered_map>

class WizardPotionRegistry {
public:
	static WizardPotionRegistry &getInstance();
	bool load(const std::string &path, std::string &error);
	[[nodiscard]] const WizardPotionDefinition* getById(uint32_t id) const;
	[[nodiscard]] const WizardPotionDefinition* getByName(const std::string &name) const;
	[[nodiscard]] size_t size() const;
	void clear();

private:
	std::unordered_map<uint32_t, WizardPotionDefinition> recipesById;
	std::unordered_map<std::string, uint32_t> idsByName;
};

constexpr auto g_wizardPotions = WizardPotionRegistry::getInstance;
