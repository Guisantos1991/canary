local function normalized(param)
	return param:trim():lower()
end

local learn = TalkAction("!wlearn")

function learn.onSay(player, words, param)
	local spellName = normalized(param)
	if spellName == "" then
		player:sendCancelMessage("Usage: !wlearn <spell>")
		return true
	end
	if not player:learnWizardSpell(spellName) then
		player:sendCancelMessage("Wizard spell not found or not learnable.")
		return true
	end
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "You learned " .. spellName .. ".")
	return true
end

learn:separator(" ")
learn:groupType("god")
learn:register()

local skills = TalkAction("!wskills")

function skills.onSay(player)
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format(
		"Wizard skills: Power %d, Control %d, Knowledge %d, Combat %d.",
		player:getWizardSkill("power"), player:getWizardSkill("control"),
		player:getWizardSkill("knowledge"), player:getWizardSkill("combat")
	))
	return true
end

skills:groupType("god")
skills:register()

local setSkill = TalkAction("!wskill")

function setSkill.onSay(player, words, param)
	local parts = param:splitTrimmed(" ")
	local value = tonumber(parts[2])
	if not parts[1] or not value or value < 1 or value > 100 or not player:setWizardSkill(parts[1]:lower(), value) then
		player:sendCancelMessage("Usage: !wskill <power|control|knowledge|combat> <1..100>")
		return true
	end
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format("Wizard skill %s set to %d.", parts[1], value))
	return true
end

setSkill:separator(" ")
setSkill:groupType("god")
setSkill:register()

local armSpell = TalkAction("!wspell")

function armSpell.onSay(player, words, param)
	local info = player:getWizardSpellInfo(normalized(param))
	if not info then
		player:sendCancelMessage("Wizard spell not found.")
		return true
	end
	if not info.learned then
		player:sendCancelMessage("Learn the spell first with !wlearn " .. info.incantation .. ".")
		return true
	end
	player:sendExtendedOpcode(91, json.encode({ spellId = info.id, name = info.name }))
	return true
end

armSpell:separator(" ")
armSpell:groupType("god")
armSpell:register()
