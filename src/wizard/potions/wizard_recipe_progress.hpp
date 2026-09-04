#pragma once

#include "wizard/progression/wizard_knowledge_types.hpp"

#include <cstdint>

struct WizardRecipeProgress {
	uint16_t knowledge = 0;
	uint16_t mastery = 0;
	bool learned = false;
	uint64_t masteryXp = 0;
	uint64_t brews = 0;
	WizardKnowledgeSourceMask knowledgeSources = 0;
};
