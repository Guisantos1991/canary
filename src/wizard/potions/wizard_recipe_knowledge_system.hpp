#pragma once

#include "wizard/progression/wizard_knowledge_types.hpp"

#include <cstdint>
#include <memory>

class Player;

enum class WizardRecipeKnowledgeResult : uint8_t {
	RECIPE_NOT_FOUND,
	SOURCE_NOT_ALLOWED,
	SUCCESS,
};

class WizardRecipeKnowledgeSystem {
public:
	[[nodiscard]] static WizardRecipeKnowledgeResult addKnowledge(const std::shared_ptr<Player> &player, uint32_t recipeId, int32_t amount, WizardKnowledgeSource source);
	[[nodiscard]] static WizardRecipeKnowledgeResult setKnowledge(const std::shared_ptr<Player> &player, uint32_t recipeId, int32_t value, WizardKnowledgeSource source);
	[[nodiscard]] static uint16_t getKnowledge(const std::shared_ptr<Player> &player, uint32_t recipeId);
	[[nodiscard]] static bool isSourceAllowed(uint32_t recipeId, WizardKnowledgeSource source);
};
