local function normalized(param)
	return param:trim():lower()
end

local skills = TalkAction("!wskills")

function skills.onSay(player)
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format(
		"Wizard skills: Power %d, Control %d, Knowledge %d, Combat %d.",
		player:getWizardSkill("power"), player:getWizardSkill("control"),
		player:getWizardSkill("knowledge"), player:getWizardSkill("combat")
	))
	return true
end

skills:groupType("normal")
skills:register()

local armSpell = TalkAction("!wspell")

function armSpell.onSay(player, words, param)
	local info = player:getWizardSpellInfo(normalized(param))
	if not info then
		player:sendCancelMessage("Wizard spell not found.")
		return true
	end
	if not info.learned then
		player:sendCancelMessage("You have not learned " .. info.name .. ".")
		return true
	end
	player:sendExtendedOpcode(91, json.encode({
		spellId = info.id,
		name = info.name,
		range = info.range,
		baseMana = info.baseMana,
		finalMana = info.finalMana,
		castTime = info.castTime,
		recovery = info.recovery,
		cooldown = info.cooldown,
		effectiveSquares = info.effectiveSquares,
		areaOffsets = info.areaOffsets,
	}))
	return true
end

armSpell:separator(" ")
armSpell:groupType("normal")
armSpell:register()
