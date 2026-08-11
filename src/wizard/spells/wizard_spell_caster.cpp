#include "wizard/spells/wizard_spell_caster.hpp"

#include "creatures/combat/combat.hpp"
#include "creatures/players/player.hpp"
#include "game/game.hpp"
#include "game/scheduling/dispatcher.hpp"
#include "items/tile.hpp"
#include "utils/tools.hpp"
#include "wizard/combat/wizard_area_system.hpp"
#include "wizard/exhaustion/wizard_exhaustion_system.hpp"
#include "wizard/mana/wizard_mana_system.hpp"
#include "wizard/progression/wizard_progression_config.hpp"
#include "wizard/spells/wizard_spell_registry.hpp"
#include "wizard/spells/wizard_targeting_validator.hpp"

#include <nlohmann/json.hpp>

namespace {
	std::string targetError(const WizardTargetValidationResult result) {
		switch (result) {
			case WizardTargetValidationResult::POSITION_REQUIRED:
				return "Choose a tile for this spell.";
			case WizardTargetValidationResult::INVALID_TILE:
				return "That tile is invalid.";
			case WizardTargetValidationResult::DIFFERENT_FLOOR:
				return "The target must be on the same floor.";
			case WizardTargetValidationResult::OUT_OF_RANGE:
				return "That tile is out of range.";
			case WizardTargetValidationResult::LINE_OF_SIGHT_BLOCKED:
				return "The path to that tile is blocked.";
			case WizardTargetValidationResult::OK:
				return {};
		}
		return "Invalid target.";
	}

	CombatType_t combatTypeForElement(const WizardElement element) {
		switch (element) {
			case WizardElement::FIRE:
				return COMBAT_FIREDAMAGE;
			case WizardElement::ICE:
				return COMBAT_ICEDAMAGE;
			case WizardElement::EARTH:
				return COMBAT_EARTHDAMAGE;
			case WizardElement::DARK:
				return COMBAT_DEATHDAMAGE;
			case WizardElement::LIGHT:
				return COMBAT_HOLYDAMAGE;
			case WizardElement::AIR:
			case WizardElement::ARCANE:
				return COMBAT_ENERGYDAMAGE;
		}
		return COMBAT_ENERGYDAMAGE;
	}

	int32_t calculateDamage(const WizardSpellDefinition &spell, const uint16_t magicalPower, const uint16_t skillCombat, const uint16_t mastery) {
		const double powerScale = 0.5 + static_cast<double>(std::clamp<uint16_t>(magicalPower, 1, 100)) / 200.0;
		const int32_t maximum = std::max(1, static_cast<int32_t>(std::floor(static_cast<double>(spell.maxPower) * powerScale)));
		const int32_t rawMinimum = std::max(1, static_cast<int32_t>(std::floor(static_cast<double>(spell.minPower) * powerScale)));
		const double stability = (static_cast<double>(std::clamp<uint16_t>(skillCombat, 1, 100)) + static_cast<double>(std::clamp<uint16_t>(mastery, 0, 100))) / 400.0;
		const int32_t minimum = std::min(maximum, rawMinimum + static_cast<int32_t>(std::floor(static_cast<double>(maximum - rawMinimum) * stability)));
		return normal_random(minimum, maximum);
	}

	bool cannotCast(const std::shared_ptr<Player> &player) {
		return player->isRemoved() || player->getHealth() <= 0 || player->hasFlag(PlayerFlags_t::CannotUseSpells)
			|| player->hasCondition(CONDITION_FEARED) || player->hasCondition(CONDITION_PACIFIED);
	}

	WizardTargetValidationResult validateTarget(
		const std::shared_ptr<Player> &player,
		const WizardSpellDefinition &spell,
		const Position &resolvedTarget,
		const std::optional<Position> &requestedTarget
	) {
		const auto tile = g_game().map.getTile(resolvedTarget);
		const bool lineOfSightClear = !spell.requiresLineOfSight || g_game().canThrowObjectTo(
			player->getPosition(),
			resolvedTarget,
			SightLine_CheckSightLineAndFloor,
			spell.range,
			spell.range
		);
		return WizardTargetingValidator::validate(
			spell.targetType,
			player->getPosition(),
			requestedTarget,
			spell.range,
			tile != nullptr,
			spell.requiresLineOfSight,
			lineOfSightClear
		);
	}
}

bool WizardSpellCaster::handleExtendedOpcode(const std::shared_ptr<Player> &player, const std::string &buffer) {
	try {
		const auto request = nlohmann::json::parse(buffer);
		const auto spellId = request.at("spellId").get<uint32_t>();
		const Position target {
			request.at("x").get<uint16_t>(),
			request.at("y").get<uint16_t>(),
			request.at("z").get<uint8_t>()
		};
		return cast(player, spellId, target);
	} catch (const std::exception &exception) {
		g_logger().warn("[WizardMagic] Invalid cast request from player {}: {}", player ? player->getName() : "unknown", exception.what());
		if (player) {
			sendFailure(player, "Invalid wizard cast request.");
		}
		return false;
	}
}

bool WizardSpellCaster::cast(const std::shared_ptr<Player> &player, const uint32_t spellId, const std::optional<Position> &targetPosition) {
	if (!player) {
		return false;
	}
	const auto* spell = g_wizardSpells().getById(spellId);
	if (!spell) {
		sendFailure(player, "Unknown wizard spell.");
		return false;
	}
	if (!player->hasLearnedWizardSpell(spellId)) {
		sendFailure(player, "You have not learned this spell.");
		return false;
	}
	const auto* progress = player->getWizardSpellProgress(spellId);
	if (player->getWizardSkills().getMagicalKnowledge() < spell->requiredKnowledge || !progress || progress->knowledge < spell->difficulty) {
		sendFailure(player, "You do not understand this spell yet.");
		return false;
	}

	const Position resolvedTarget = spell->targetType == WizardTargetType::SELF ? player->getPosition() : targetPosition.value_or(Position {});
	const auto tile = g_game().map.getTile(resolvedTarget);
	const auto targetResult = validateTarget(player, *spell, resolvedTarget, targetPosition);
	if (targetResult != WizardTargetValidationResult::OK) {
		sendFailure(player, targetError(targetResult));
		return false;
	}
	if (cannotCast(player)) {
		sendFailure(player, "You cannot cast right now.");
		return false;
	}

	const auto now = OTSYS_TIME();
	if (player->getWizardRecoveryUntil() > now) {
		sendFailure(player, "You are still recovering from the previous spell.");
		return false;
	}
	if (player->getWizardCooldownUntil(spellId) > now) {
		sendFailure(player, "This spell is still on cooldown.");
		return false;
	}
	const auto &config = g_wizardProgression().get();
	const uint32_t manaCost = WizardManaSystem::calculateSpellManaCost(spell->manaCost, player->getWizardSkills().getMagicalControl(), progress->mastery, config.mana);
	if (player->getMana() < manaCost) {
		sendFailure(player, "You do not have enough mana.");
		return false;
	}
	if (Combat::canDoCombat(player, tile, spell->category == WizardSpellCategory::OFFENSIVE) != RETURNVALUE_NOERROR) {
		sendFailure(player, "You cannot use that spell on this tile.");
		return false;
	}

	const uint32_t castTime = WizardExhaustionSystem::calculateCastTime(spell->castTimeMs, player->getWizardSkills().getSkillCombat(), progress->mastery, config.cast);
	player->setWizardRecoveryUntil(now + castTime);
	if (castTime == 0) {
		executeCast(player->getID(), spellId, resolvedTarget);
	} else {
		g_dispatcher().scheduleEvent(castTime, [playerId = player->getID(), spellId, resolvedTarget] {
			WizardSpellCaster::executeCast(playerId, spellId, resolvedTarget);
		}, "WizardSpellCaster::executeCast");
	}
	return true;
}

void WizardSpellCaster::executeCast(const uint32_t playerId, const uint32_t spellId, const Position targetPosition) {
	const auto player = g_game().getPlayerByID(playerId);
	const auto* spell = g_wizardSpells().getById(spellId);
	if (!player || !spell || player->isRemoved() || player->getHealth() <= 0) {
		return;
	}
	const auto abortCast = [&player](const std::string &message) {
		sendFailure(player, message);
		player->setWizardRecoveryUntil(0);
	};
	if (cannotCast(player)) {
		abortCast("You cannot cast right now.");
		return;
	}

	const Position resolvedTarget = spell->targetType == WizardTargetType::SELF ? player->getPosition() : targetPosition;
	const auto requestedTarget = std::optional<Position> { resolvedTarget };
	const auto targetResult = validateTarget(player, *spell, resolvedTarget, requestedTarget);
	if (targetResult != WizardTargetValidationResult::OK) {
		abortCast(targetError(targetResult));
		return;
	}
	const auto tile = g_game().map.getTile(resolvedTarget);
	if (Combat::canDoCombat(player, tile, spell->category == WizardSpellCategory::OFFENSIVE) != RETURNVALUE_NOERROR) {
		abortCast("You cannot use that spell on this tile.");
		return;
	}

	auto &progress = player->getOrCreateWizardSpellProgress(spellId);
	const auto &config = g_wizardProgression().get();
	const uint32_t manaCost = WizardManaSystem::calculateSpellManaCost(spell->manaCost, player->getWizardSkills().getMagicalControl(), progress.mastery, config.mana);
	if (player->getMana() < manaCost) {
		abortCast("The spell failed because your mana changed during casting.");
		return;
	}

	player->changeMana(-static_cast<int32_t>(manaCost));
	const auto now = OTSYS_TIME();
	player->setWizardRecoveryUntil(now + WizardExhaustionSystem::calculateRecovery(spell->recoveryTimeMs, player->getWizardSkills().getMagicalControl(), progress.mastery, config.recovery));
	player->setWizardCooldownUntil(spellId, now + spell->cooldownMs);
	++progress.uses;
	progress.mastery = std::min<uint16_t>(100, std::max<uint16_t>(progress.mastery, static_cast<uint16_t>(progress.uses / 10)));

	if (spell->projectile.visualEffect != 0) {
		g_game().addDistanceEffect(player->getPosition(), resolvedTarget, spell->projectile.visualEffect, player);
	}
	const auto power = player->getWizardSkills().getMagicalPower();
	const auto combat = player->getWizardSkills().getSkillCombat();
	const auto mastery = progress.mastery;
	if (spell->projectile.travelTimeMs == 0) {
		impact(playerId, spellId, resolvedTarget, power, combat, mastery);
	} else {
		g_dispatcher().scheduleEvent(spell->projectile.travelTimeMs, [playerId, spellId, resolvedTarget, power, combat, mastery] {
			WizardSpellCaster::impact(playerId, spellId, resolvedTarget, power, combat, mastery);
		}, "WizardSpellCaster::impact");
	}
}

void WizardSpellCaster::impact(const uint32_t playerId, const uint32_t spellId, const Position targetPosition, const uint16_t magicalPower, const uint16_t skillCombat, const uint16_t mastery) {
	const auto player = g_game().getPlayerByID(playerId);
	const auto* spell = g_wizardSpells().getById(spellId);
	if (!player || !spell) {
		return;
	}
	CombatParams combatParams;
	combatParams.combatType = combatTypeForElement(spell->element);
	combatParams.origin = ORIGIN_SPELL;
	combatParams.aggressive = spell->category == WizardSpellCategory::OFFENSIVE;

	for (const auto &position : WizardAreaSystem::resolve(spell->area, targetPosition, magicalPower)) {
		const auto tile = g_game().map.getTile(position);
		if (!tile || Combat::canDoCombat(player, tile, spell->category == WizardSpellCategory::OFFENSIVE) != RETURNVALUE_NOERROR) {
			continue;
		}
		if (spell->impactEffect != 0) {
			g_game().addMagicEffect(position, spell->impactEffect, player);
		}
		const auto* creatures = tile->getCreatures();
		if (!creatures) {
			continue;
		}
		const CreatureVector occupants = *creatures;
		for (const auto &target : occupants) {
			if (!target || target == player) {
				continue;
			}
			CombatDamage damage;
			damage.origin = ORIGIN_SPELL;
			damage.primary.type = combatParams.combatType;
			damage.primary.value = -calculateDamage(*spell, magicalPower, skillCombat, mastery);
			damage.instantSpellName = spell->name;
			Combat::doCombatHealth(player, target, damage, combatParams);
		}
	}
}

void WizardSpellCaster::sendFailure(const std::shared_ptr<Player> &player, const std::string &message) {
	player->sendCancelMessage(message);
}
