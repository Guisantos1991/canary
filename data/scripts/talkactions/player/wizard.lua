local function normalized(param)
	return param:trim():lower()
end

local function jsonString(value)
	local escaped = tostring(value):gsub('[%z\1-\31\\"]', function(character)
		local replacements = {
			['"'] = '\\"',
			['\\'] = '\\\\',
			['\b'] = '\\b',
			['\f'] = '\\f',
			['\n'] = '\\n',
			['\r'] = '\\r',
			['\t'] = '\\t',
		}
		return replacements[character] or string.format('\\u%04x', character:byte())
	end)
	return '"' .. escaped .. '"'
end

local function wizardArmPayload(info)
	local areaOffsets = {}
	for _, offset in ipairs(info.areaOffsets or {}) do
		areaOffsets[#areaOffsets + 1] = string.format('{"x":%d,"y":%d}', offset.x, offset.y)
	end

	return string.format(
		'{"spellId":%d,"name":%s,"range":%d,"baseMana":%d,"finalMana":%d,"castTime":%d,"recovery":%d,"cooldown":%d,"effectiveSquares":%d,"areaOffsets":[%s]}',
		info.id,
		jsonString(info.name),
		info.range,
		info.baseMana,
		info.finalMana,
		info.castTime,
		info.recovery,
		info.cooldown,
		info.effectiveSquares,
		table.concat(areaOffsets, ',')
	)
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
	local payload = wizardArmPayload(info)
	player:sendExtendedOpcode(91, payload)
	return true
end

armSpell:separator(" ")
armSpell:groupType("normal")
armSpell:register()

local learnSpell = TalkAction("!wlearn")

function learnSpell.onSay(player, words, param)
	local spell = normalized(param)
	if spell == "" then
		player:sendCancelMessage("Usage: !wlearn <spell>")
		return true
	end
	local result = player:tryLearnWizardSpell(spell)
	local messages = {
		SPELL_NOT_FOUND = "Wizard spell not found.",
		ALREADY_LEARNED = "You have already learned that spell.",
		INSUFFICIENT_SPELL_KNOWLEDGE = "You do not have enough knowledge of that spell.",
		INSUFFICIENT_MAGICAL_KNOWLEDGE = "Your Magical Knowledge is not high enough.",
		SOURCE_REQUIREMENT_NOT_MET = "You have not satisfied the spell's knowledge source requirements.",
		NOT_LEARNABLE = "That spell cannot be learned.",
	}
	if result ~= "SUCCESS" then
		player:sendCancelMessage(messages[result] or "The spell could not be learned.")
		return true
	end
	local info = player:getWizardSpellInfo(spell)
	if not player:save() then
		player:sendCancelMessage("The spell was learned in memory, but your progress could not be saved.")
		return true
	end
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "You learned " .. info.name .. ".")
	return true
end

learnSpell:separator(" ")
learnSpell:groupType("normal")
learnSpell:register()

local progress = TalkAction("!wprogress")

function progress.onSay(player, words, param)
	local info = player:getWizardSpellInfo(normalized(param))
	if not info then
		player:sendCancelMessage("Wizard spell not found.")
		return true
	end
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format(
		"%s | Knowledge %d/%d | Magical Knowledge %d/%d | Profile %s | Learnable %s | Learned %s | Mastery %d | Mastery XP %d | Uses %d",
		info.name, info.knowledge, info.knowledgeRequired, info.magicalKnowledge, info.magicalKnowledgeRequired,
		info.acquisitionProfile, info.learnable and "yes" or "no", info.learned and "yes" or "no",
		info.mastery, info.masteryXp, info.uses
	))
	return true
end

progress:separator(" ")
progress:groupType("normal")
progress:register()

local learnRecipe = TalkAction("!wlearnrecipe")

function learnRecipe.onSay(player, words, param)
	local recipe = normalized(param)
	if recipe == "" then
		player:sendCancelMessage("Usage: !wlearnrecipe <recipe>")
		return true
	end
	local result = player:tryLearnWizardRecipe(recipe)
	local messages = {
		RECIPE_NOT_FOUND = "Wizard recipe not found.",
		ALREADY_LEARNED = "You have already learned that recipe.",
		INSUFFICIENT_RECIPE_KNOWLEDGE = "You do not have enough knowledge of that recipe.",
		INSUFFICIENT_MAGICAL_KNOWLEDGE = "Your Magical Knowledge is not high enough.",
		SOURCE_REQUIREMENT_NOT_MET = "You have not satisfied the recipe's knowledge source requirements.",
	}
	if result ~= "SUCCESS" then
		player:sendCancelMessage(messages[result] or "The recipe could not be learned.")
		return true
	end
	local info = player:getWizardRecipeInfo(recipe)
	if not player:save() then
		player:sendCancelMessage("The recipe was learned in memory, but your progress could not be saved.")
		return true
	end
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "You learned recipe " .. info.name .. ".")
	return true
end

learnRecipe:separator(" ")
learnRecipe:groupType("normal")
learnRecipe:register()

local recipeProgress = TalkAction("!wrecipeprogress")

function recipeProgress.onSay(player, words, param)
	local info = player:getWizardRecipeInfo(normalized(param))
	if not info then
		player:sendCancelMessage("Wizard recipe not found.")
		return true
	end
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format(
		"%s | Knowledge %d/%d | Magical Knowledge %d/%d | Profile %s | Learnable %s | Learned %s | Brewing Mastery %d | Brewing XP %d | Brews %d",
		info.name, info.knowledge, info.knowledgeRequired, info.magicalKnowledge, info.magicalKnowledgeRequired,
		info.acquisitionProfile, info.learnable and "yes" or "no", info.learned and "yes" or "no",
		info.mastery, info.masteryXp, info.brews
	))
	return true
end

recipeProgress:separator(" ")
recipeProgress:groupType("normal")
recipeProgress:register()
