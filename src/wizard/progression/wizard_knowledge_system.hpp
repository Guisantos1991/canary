#pragma once

#include "wizard/progression/wizard_knowledge_types.hpp"

#include <cstdint>
#include <memory>

class Player;

enum class WizardKnowledgeResult : uint8_t {
	SPELL_NOT_FOUND,
	SOURCE_NOT_ALLOWED,
	SUCCESS,
};

class WizardKnowledgeSystem {
public:
	[[nodiscard]] static WizardKnowledgeResult addKnowledge(const std::shared_ptr<Player> &player, uint32_t spellId, int32_t amount, WizardKnowledgeSource source);
	[[nodiscard]] static WizardKnowledgeResult setKnowledge(const std::shared_ptr<Player> &player, uint32_t spellId, int32_t value, WizardKnowledgeSource source);
	[[nodiscard]] static uint16_t getKnowledge(const std::shared_ptr<Player> &player, uint32_t spellId);
	[[nodiscard]] static bool isSourceAllowed(uint32_t spellId, WizardKnowledgeSource source);
};
