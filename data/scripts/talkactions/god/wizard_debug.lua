local function normalized(param)
	return param:trim():lower()
end

local function findTarget(player, name)
	local target = Player(name:trim())
	if not target then
		player:sendCancelMessage("Player not found or not online: " .. name:trim())
	end
	return target
end

local learn = TalkAction("/wlearn")

function learn.onSay(player, words, param)
	logCommand(player, words, param)
	local parts = param:split(",")
	if #parts ~= 2 or parts[1]:trim() == "" or normalized(parts[2]) == "" then
		player:sendCancelMessage("Usage: /wlearn <player>, <spell>")
		return true
	end

	local target = findTarget(player, parts[1])
	if not target then
		return true
	end

	local spellName = normalized(parts[2])
	if not target:learnWizardSpell(spellName) then
		player:sendCancelMessage("Wizard spell not found or not learnable.")
		return true
	end
	if not target:save() then
		player:sendCancelMessage("The spell was learned in memory, but the player could not be saved.")
		return true
	end

	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format("%s learned wizard spell %s.", target:getName(), spellName))
	target:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format("%s taught you wizard spell %s.", player:getName(), spellName))
	return true
end

learn:separator(" ")
learn:groupType("god")
learn:register()

local setSkill = TalkAction("/wskill")

function setSkill.onSay(player, words, param)
	logCommand(player, words, param)
	local parts = param:split(",")
	if #parts ~= 3 then
		player:sendCancelMessage("Usage: /wskill <player>, <power|control|knowledge|combat>, <value>")
		return true
	end

	local target = findTarget(player, parts[1])
	if not target then
		return true
	end

	local skill = normalized(parts[2])
	local value = tonumber(parts[3]:trim())
	if not value or value ~= math.floor(value) or not target:setWizardSkill(skill, value) then
		player:sendCancelMessage("Usage: /wskill <player>, <power|control|knowledge|combat>, <value>")
		return true
	end
	if not target:save() then
		player:sendCancelMessage("The skill was changed in memory, but the player could not be saved.")
		return true
	end

	local applied = target:getWizardSkill(skill)
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format("%s's wizard skill %s is now %d.", target:getName(), skill, applied))
	target:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format("%s set your wizard skill %s to %d.", player:getName(), skill, applied))
	return true
end

setSkill:separator(" ")
setSkill:groupType("god")
setSkill:register()
