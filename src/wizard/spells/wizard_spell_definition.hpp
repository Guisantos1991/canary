#pragma once

#include <cstdint>
#include <string>

enum class WizardTargetType {
	TILE,
	AREA,
	SELF,
	DIRECTION
};

enum class WizardSpellCategory {
	OFFENSIVE,
	DEFENSIVE,
	UTILITY,
	CONTROL
};

enum class WizardAreaPattern {
	NONE,
	CIRCLE,
	CROSS,
	CONE,
	LINE,
	RING,
	WAVE,
	CUSTOM
};

enum class WizardElement {
	ARCANE,
	FIRE,
	ICE,
	EARTH,
	AIR,
	LIGHT,
	DARK
};

struct WizardAreaDefinition {
	WizardAreaPattern pattern = WizardAreaPattern::NONE;

	uint16_t minSquares = 1;
	uint16_t maxSquares = 1;
};

struct WizardProjectileDefinition {
	uint16_t visualEffect = 0;
	uint32_t travelTimeMs = 0;
};

struct WizardSpellDefinition {
	uint32_t id = 0;

	std::string name;
	std::string incantation;

	WizardElement element = WizardElement::ARCANE;
	WizardSpellCategory category = WizardSpellCategory::UTILITY;
	WizardTargetType targetType = WizardTargetType::TILE;

	uint32_t manaCost = 0;

	uint32_t castTimeMs = 0;

	// Tempo após conjurar antes de poder executar outra magia.
	uint32_t recoveryTimeMs = 1500;

	// Cooldown exclusivo desta magia.
	uint32_t cooldownMs = 2000;

	uint16_t range = 1;

	int32_t minPower = 0;
	int32_t maxPower = 0;
	uint16_t impactEffect = 0;
	uint16_t difficulty = 0;
	uint16_t requiredKnowledge = 0;

	WizardAreaDefinition area;
	WizardProjectileDefinition projectile;
	std::string specialScript;

	bool requiresLineOfSight = true;
	bool learnable = true;
	bool darkMagic = false;
};
