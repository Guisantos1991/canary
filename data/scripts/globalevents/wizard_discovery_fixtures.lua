-- Sprint 4 development fixtures only. Coordinates are curated and mirror
-- data/wizard/discoveries.json; this is intentionally not a content catalogue.
local fixtures = {
	{ itemId = 401, actionId = 62001, position = Position(32369, 32242, 7) },
	{ itemId = 2012, actionId = 62002, position = Position(32370, 32242, 7) },
	{ itemId = 203, actionId = 62004, position = Position(32371, 32242, 7) },
	{ itemId = 173, actionId = 62005, position = Position(32370, 32242, 7) },
	{ itemId = 174, actionId = 62006, position = Position(32371, 32242, 7) },
	{ itemId = 175, actionId = 62007, position = Position(32372, 32242, 7) },
	{ itemId = 635, actionId = 62003, position = Position(32369, 32244, 7) },
	{ itemId = 635, actionId = 62003, position = Position(32370, 32244, 7) },
	{ itemId = 635, actionId = 62003, position = Position(32371, 32244, 7) },
}

local function hasFixture(tile, actionId)
	for _, item in ipairs(tile:getItems() or {}) do
		if item:getActionId() == actionId then return true end
	end
	return false
end

local event = GlobalEvent("WizardDiscoveryDevelopmentFixtures")

function event.onStartup()
	for _, fixture in ipairs(fixtures) do
		local tile = Tile(fixture.position)
		if not tile then
			logger.error("[WizardDiscoveryFixtures] Missing curated tile at {},{},{}", fixture.position.x, fixture.position.y, fixture.position.z)
		elseif not hasFixture(tile, fixture.actionId) then
			local item = Game.createItem(fixture.itemId, 1, fixture.position)
			if item then item:setActionId(fixture.actionId) end
		end
	end
	return true
end

event:register()
