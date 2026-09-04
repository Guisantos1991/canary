function onUpdateDatabase()
	logger.info("Updating database to version 62 (Wizard personal discoveries)")

	return db.query([[
		CREATE TABLE IF NOT EXISTS `player_wizard_discoveries` (
			`player_id` int(11) NOT NULL,
			`discovery_id` varchar(128) NOT NULL,
			`state` enum('ASSIGNED','DISCOVERED') NOT NULL DEFAULT 'ASSIGNED',
			`assigned_location_id` varchar(128) DEFAULT NULL,
			`assigned_at` timestamp NULL DEFAULT NULL,
			`discovered_at` timestamp NULL DEFAULT NULL,
			`reward_applied_at` timestamp NULL DEFAULT NULL,
			CONSTRAINT `player_wizard_discoveries_pk` PRIMARY KEY (`player_id`, `discovery_id`),
			INDEX `player_wizard_discoveries_discovery_idx` (`discovery_id`),
			CONSTRAINT `player_wizard_discoveries_players_fk` FOREIGN KEY (`player_id`) REFERENCES `players` (`id`) ON DELETE CASCADE
		) ENGINE=InnoDB DEFAULT CHARSET=utf8;
	]])
end
