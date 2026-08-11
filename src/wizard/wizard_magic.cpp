#include "wizard/wizard_magic.hpp"

#include "lib/logging/logger.hpp"
#include "wizard/progression/wizard_progression_config.hpp"
#include "wizard/spells/wizard_spell_registry.hpp"

bool WizardMagic::load(const std::string &coreDirectory, std::string &error) {
	const std::string wizardDirectory = coreDirectory + "/wizard/";
	if (!g_wizardProgression().load(wizardDirectory + "progression.json", error)) {
		error = "progression.json: " + error;
		return false;
	}
	g_logger().info("[WizardMagic] Loaded progression configuration");

	if (!g_wizardSpells().load(wizardDirectory + "spells.json", error)) {
		error = "spells.json: " + error;
		return false;
	}
	g_logger().info("[WizardMagic] Loaded {} spells", g_wizardSpells().size());
	return true;
}
