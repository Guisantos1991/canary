function onUpdateDatabase()
	logger.info("Updating database to version 60 (Wizard Magic Core persistence)")

	if not db.query([[
		CREATE TABLE IF NOT EXISTS `player_wizard_skills` (
			`player_id` int(11) NOT NULL,
			`magical_power` tinyint(3) UNSIGNED NOT NULL DEFAULT '1',
			`magical_control` tinyint(3) UNSIGNED NOT NULL DEFAULT '1',
			`magical_knowledge` tinyint(3) UNSIGNED NOT NULL DEFAULT '1',
			`skill_combat` tinyint(3) UNSIGNED NOT NULL DEFAULT '1',
			CONSTRAINT `player_wizard_skills_pk` PRIMARY KEY (`player_id`),
			CONSTRAINT `player_wizard_skills_players_fk` FOREIGN KEY (`player_id`) REFERENCES `players` (`id`) ON DELETE CASCADE
		) ENGINE=InnoDB DEFAULT CHARSET=utf8;
	]]) then
		return false
	end

	return db.query([[
		CREATE TABLE IF NOT EXISTS `player_wizard_spells` (
			`player_id` int(11) NOT NULL,
			`spell_id` int(10) UNSIGNED NOT NULL,
			`knowledge` tinyint(3) UNSIGNED NOT NULL DEFAULT '0',
			`mastery` tinyint(3) UNSIGNED NOT NULL DEFAULT '0',
			`learned` tinyint(1) UNSIGNED NOT NULL DEFAULT '0',
			`uses` bigint(20) UNSIGNED NOT NULL DEFAULT '0',
			CONSTRAINT `player_wizard_spells_pk` PRIMARY KEY (`player_id`, `spell_id`),
			CONSTRAINT `player_wizard_spells_players_fk` FOREIGN KEY (`player_id`) REFERENCES `players` (`id`) ON DELETE CASCADE
		) ENGINE=InnoDB DEFAULT CHARSET=utf8;
	]])
end
