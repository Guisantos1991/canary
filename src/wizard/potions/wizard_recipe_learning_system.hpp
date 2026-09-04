#pragma once

#include <cstdint>
#include <memory>

class Player;

enum class WizardRecipeLearningResult : uint8_t {
	RECIPE_NOT_FOUND,
	ALREADY_LEARNED,
	INSUFFICIENT_RECIPE_KNOWLEDGE,
	INSUFFICIENT_MAGICAL_KNOWLEDGE,
	SOURCE_REQUIREMENT_NOT_MET,
	SUCCESS,
};

class WizardRecipeLearningSystem {
public:
	[[nodiscard]] static WizardRecipeLearningResult check(const std::shared_ptr<Player> &player, uint32_t recipeId);
	[[nodiscard]] static WizardRecipeLearningResult learn(const std::shared_ptr<Player> &player, uint32_t recipeId);
	[[nodiscard]] static bool isLearnable(const std::shared_ptr<Player> &player, uint32_t recipeId);
};
