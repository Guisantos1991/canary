#pragma once

#include <cstdint>
#include <memory>

class Player;

enum class WizardLearningResult : uint8_t {
	SPELL_NOT_FOUND,
	ALREADY_LEARNED,
	INSUFFICIENT_SPELL_KNOWLEDGE,
	INSUFFICIENT_MAGICAL_KNOWLEDGE,
	SOURCE_REQUIREMENT_NOT_MET,
	NOT_LEARNABLE,
	SUCCESS,
};

class WizardLearningSystem {
public:
	[[nodiscard]] static WizardLearningResult check(const std::shared_ptr<Player> &player, uint32_t spellId);
	[[nodiscard]] static WizardLearningResult learn(const std::shared_ptr<Player> &player, uint32_t spellId);
	[[nodiscard]] static bool isLearnable(const std::shared_ptr<Player> &player, uint32_t spellId);
};
