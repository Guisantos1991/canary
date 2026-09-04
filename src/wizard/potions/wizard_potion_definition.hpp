#pragma once

#include "wizard/progression/wizard_knowledge_types.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct WizardPotionIngredientDefinition {
	uint16_t itemId = 0;
	uint16_t amount = 0;
};

struct WizardPotionBaseEffects {
	double potency = 0;
	uint32_t durationMs = 0;
	uint16_t yield = 1;
	double stability = 0;
};

struct WizardRecipeProgressionDefinition {
	uint16_t knowledgeRequired = 0;
	uint16_t magicalKnowledgeRequired = 0;
	WizardAcquisitionProfile acquisitionProfile = WizardAcquisitionProfile::ACADEMIC;
	WizardKnowledgeSourceMask allowedKnowledgeSources = 0;
	WizardKnowledgeSourceMask requiredKnowledgeSources = 0;
};

struct WizardPotionDefinition {
	uint32_t id = 0;
	std::string name;
	bool developmentFixture = false;
	WizardRecipeProgressionDefinition progression;
	std::vector<WizardPotionIngredientDefinition> ingredients;
	WizardPotionBaseEffects baseEffects;
	double baseQuality = 0;
	std::optional<uint32_t> linkedSpellId;
};
