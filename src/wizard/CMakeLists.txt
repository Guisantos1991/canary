target_sources(
    ${CORE_TARGET_NAME}
    PRIVATE combat/wizard_area_system.cpp
            exhaustion/wizard_exhaustion_system.cpp
            mana/wizard_mana_system.cpp
            progression/wizard_progression_config.cpp
            spells/wizard_spell_caster.cpp
            spells/wizard_spell_registry.cpp
            spells/wizard_targeting_validator.cpp
            wizard_magic.cpp
)
