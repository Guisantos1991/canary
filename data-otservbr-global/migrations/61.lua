local function columnExists(tableName, columnName)
	local result = db.storeQuery(string.format("SHOW COLUMNS FROM `%s` LIKE '%s';", tableName, columnName))
	if not result then
		return false
	end
	Result.free(result)
	return true
end

function onUpdateDatabase()
	logger.info("Updating database to version 61 (Wizard progression and potion foundation)")

	if not columnExists("player_wizard_spells", "mastery_xp") and not db.query([[
		ALTER TABLE `player_wizard_spells`
		ADD COLUMN `mastery_xp` bigint(20) UNSIGNED NOT NULL DEFAULT '0' AFTER `mastery`;
	]]) then
		return false
	end

	if not columnExists("player_wizard_spells", "knowledge_sources") and not db.query([[
		ALTER TABLE `player_wizard_spells`
		ADD COLUMN `knowledge_sources` smallint(5) UNSIGNED NOT NULL DEFAULT '0' AFTER `mastery_xp`;
	]]) then
		return false
	end

	-- Backfill the cumulative threshold for the initial data-driven curve. This
	-- preserves every existing mastery level while making mastery_xp consistent.
	if not db.query([[
		UPDATE `player_wizard_spells`
		SET `mastery_xp` = CASE
			WHEN `mastery` <= 20 THEN `mastery` * 50
			WHEN `mastery` <= 50 THEN 1000 + (`mastery` - 20) * 125
			WHEN `mastery` <= 80 THEN 4750 + (`mastery` - 50) * 300
			WHEN `mastery` <= 95 THEN 13750 + (`mastery` - 80) * 750
			ELSE 25000 + (`mastery` - 95) * 1500
		END
		WHERE `mastery` > 0 AND `mastery_xp` = 0;
	]]) then
		return false
	end

	return db.query([[
		CREATE TABLE IF NOT EXISTS `player_wizard_recipes` (
			`player_id` int(11) NOT NULL,
			`recipe_id` int(10) UNSIGNED NOT NULL,
			`knowledge` tinyint(3) UNSIGNED NOT NULL DEFAULT '0',
			`mastery` tinyint(3) UNSIGNED NOT NULL DEFAULT '0',
			`mastery_xp` bigint(20) UNSIGNED NOT NULL DEFAULT '0',
			`knowledge_sources` smallint(5) UNSIGNED NOT NULL DEFAULT '0',
			`learned` tinyint(1) UNSIGNED NOT NULL DEFAULT '0',
			`brews` bigint(20) UNSIGNED NOT NULL DEFAULT '0',
			CONSTRAINT `player_wizard_recipes_pk` PRIMARY KEY (`player_id`, `recipe_id`),
			CONSTRAINT `player_wizard_recipes_players_fk` FOREIGN KEY (`player_id`) REFERENCES `players` (`id`) ON DELETE CASCADE
		) ENGINE=InnoDB DEFAULT CHARSET=utf8;
	]])
end
