#include "wizard/progression/wizard_mastery_system.hpp"

#include "creatures/players/player.hpp"
#include "wizard/progression/wizard_mastery_curve.hpp"
#include "wizard/progression/wizard_progression_config.hpp"
#include "wizard/spells/wizard_spell_registry.hpp"

#include <algorithm>

uint64_t WizardMasterySystem::calculateThreshold(const uint16_t level) {
	return WizardMasteryCurve::calculateThreshold(level, g_wizardProgression().get().spellMastery);
}

uint16_t WizardMasterySystem::calculateLevel(const uint64_t xp) {
	return WizardMasteryCurve::calculateLevel(xp, g_wizardProgression().get().spellMastery);
}

uint64_t WizardMasterySystem::calculateMeaningfulUseXp(const std::size_t affectedTargets) {
	if (affectedTargets == 0) return 0;
	const auto &config = g_wizardProgression().get().meaningfulUse;
	const auto bonusTargets = std::min<std::size_t>(affectedTargets - 1, config.bonusTargetCap);
	return config.baseXp + bonusTargets * config.additionalTargetBonus;
}

uint16_t WizardMasterySystem::addXp(const std::shared_ptr<Player> &player, const uint32_t spellId, const uint64_t xp) {
	if (!player || !g_wizardSpells().getById(spellId) || !player->hasLearnedWizardSpell(spellId) || xp == 0) return 0;
	auto &progress = player->getOrCreateWizardSpellProgress(spellId);
	const auto maximum = calculateThreshold(g_wizardProgression().get().spellMastery.limits.max);
	progress.masteryXp = xp > maximum - std::min(progress.masteryXp, maximum) ? maximum : progress.masteryXp + xp;
	progress.mastery = calculateLevel(progress.masteryXp);
	return progress.mastery;
}

uint16_t WizardMasterySystem::grantMeaningfulUse(const std::shared_ptr<Player> &player, const uint32_t spellId, const std::size_t affectedTargets) {
	return addXp(player, spellId, calculateMeaningfulUseXp(affectedTargets));
}

bool WizardMasterySystem::setMastery(const std::shared_ptr<Player> &player, const uint32_t spellId, const int32_t level) {
	if (!player || !g_wizardSpells().getById(spellId)) return false;
	const auto &limits = g_wizardProgression().get().spellMastery.limits;
	auto &progress = player->getOrCreateWizardSpellProgress(spellId);
	progress.mastery = static_cast<uint16_t>(std::clamp<int32_t>(level, limits.min, limits.max));
	progress.masteryXp = calculateThreshold(progress.mastery);
	return true;
}
