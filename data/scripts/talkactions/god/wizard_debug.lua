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

local function threePartCommand(player, param, usage)
	local parts = param:split(",")
	if #parts ~= 3 then
		player:sendCancelMessage(usage)
		return nil
	end
	local target = findTarget(player, parts[1])
	local value = tonumber(parts[3]:trim())
	if not target or normalized(parts[2]) == "" or not value or value ~= math.floor(value) then
		if target then player:sendCancelMessage(usage) end
		return nil
	end
	return target, normalized(parts[2]), value
end

local setKnowledge = TalkAction("/wknowledge")

function setKnowledge.onSay(player, words, param)
	logCommand(player, words, param)
	local target, spell, value = threePartCommand(player, param, "Usage: /wknowledge <player>, <spell>, <value>")
	if not target then return true end
	if not target:setWizardSpellKnowledge(spell, value, "ADMIN") then
		player:sendCancelMessage("Wizard spell not found, source not allowed, or value invalid.")
		return true
	end
	if not target:save() then player:sendCancelMessage("Progress changed in memory but could not be saved.") return true end
	local info = target:getWizardSpellInfo(spell)
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format("%s's %s knowledge is now %d.", target:getName(), info.name, info.knowledge))
	return true
end

setKnowledge:separator(" ")
setKnowledge:groupType("god")
setKnowledge:register()

local setMastery = TalkAction("/wmastery")

function setMastery.onSay(player, words, param)
	logCommand(player, words, param)
	local target, spell, value = threePartCommand(player, param, "Usage: /wmastery <player>, <spell>, <value>")
	if not target then return true end
	if not target:setWizardSpellMastery(spell, value) then player:sendCancelMessage("Wizard spell not found.") return true end
	if not target:save() then player:sendCancelMessage("Progress changed in memory but could not be saved.") return true end
	local info = target:getWizardSpellInfo(spell)
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format("%s's %s mastery is now %d (XP %d).", target:getName(), info.name, info.mastery, info.masteryXp))
	return true
end

setMastery:separator(" ")
setMastery:groupType("god")
setMastery:register()

local spellInfo = TalkAction("/wspellinfo")

function spellInfo.onSay(player, words, param)
	logCommand(player, words, param)
	local parts = param:split(",")
	if #parts ~= 2 then player:sendCancelMessage("Usage: /wspellinfo <player>, <spell>") return true end
	local target = findTarget(player, parts[1])
	local info = target and target:getWizardSpellInfo(normalized(parts[2])) or nil
	if not info then player:sendCancelMessage("Wizard spell not found.") return true end
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format("%s: %s knowledge %d/%d, MK %d/%d, profile %s, learned %s, learnable %s, mastery %d, XP %d, uses %d.", target:getName(), info.name, info.knowledge, info.knowledgeRequired, info.magicalKnowledge, info.magicalKnowledgeRequired, info.acquisitionProfile, tostring(info.learned), tostring(info.learnable), info.mastery, info.masteryXp, info.uses))
	return true
end

spellInfo:separator(" ")
spellInfo:groupType("god")
spellInfo:register()

local setRecipeKnowledge = TalkAction("/wrecipeknowledge")

function setRecipeKnowledge.onSay(player, words, param)
	logCommand(player, words, param)
	local target, recipe, value = threePartCommand(player, param, "Usage: /wrecipeknowledge <player>, <recipe>, <value>")
	if not target then return true end
	if not target:setWizardRecipeKnowledge(recipe, value, "ADMIN") then player:sendCancelMessage("Wizard recipe not found or source not allowed.") return true end
	if not target:save() then player:sendCancelMessage("Progress changed in memory but could not be saved.") return true end
	local info = target:getWizardRecipeInfo(recipe)
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format("%s's %s recipe knowledge is now %d.", target:getName(), info.name, info.knowledge))
	return true
end

setRecipeKnowledge:separator(" ")
setRecipeKnowledge:groupType("god")
setRecipeKnowledge:register()

local setRecipeMastery = TalkAction("/wrecipemastery")

function setRecipeMastery.onSay(player, words, param)
	logCommand(player, words, param)
	local target, recipe, value = threePartCommand(player, param, "Usage: /wrecipemastery <player>, <recipe>, <value>")
	if not target then return true end
	if not target:setWizardRecipeMastery(recipe, value) then player:sendCancelMessage("Wizard recipe not found.") return true end
	if not target:save() then player:sendCancelMessage("Progress changed in memory but could not be saved.") return true end
	local info = target:getWizardRecipeInfo(recipe)
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format("%s's %s Brewing Mastery is now %d (XP %d).", target:getName(), info.name, info.mastery, info.masteryXp))
	return true
end

setRecipeMastery:separator(" ")
setRecipeMastery:groupType("god")
setRecipeMastery:register()

local recipeInfo = TalkAction("/wrecipeinfo")

function recipeInfo.onSay(player, words, param)
	logCommand(player, words, param)
	local parts = param:split(",")
	if #parts ~= 2 then player:sendCancelMessage("Usage: /wrecipeinfo <player>, <recipe>") return true end
	local target = findTarget(player, parts[1])
	local info = target and target:getWizardRecipeInfo(normalized(parts[2])) or nil
	if not info then player:sendCancelMessage("Wizard recipe not found.") return true end
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format("%s: %s knowledge %d/%d, MK %d/%d, profile %s, learned %s, learnable %s, mastery %d, XP %d, brews %d.", target:getName(), info.name, info.knowledge, info.knowledgeRequired, info.magicalKnowledge, info.magicalKnowledgeRequired, info.acquisitionProfile, tostring(info.learned), tostring(info.learnable), info.mastery, info.masteryXp, info.brews))
	return true
end

recipeInfo:separator(" ")
recipeInfo:groupType("god")
recipeInfo:register()

local brewTest = TalkAction("/wbrewtest")

function brewTest.onSay(player, words, param)
	logCommand(player, words, param)
	local target, recipe, quality = threePartCommand(player, param, "Usage: /wbrewtest <player>, <recipe>, <ingredient quality>")
	if not target then return true end
	if quality < 0 or quality > 100 then
		player:sendCancelMessage("Ingredient quality must be between 0 and 100.")
		return true
	end
	local result = target:brewWizardRecipeForTest(recipe, quality)
	if not result or not result.success then player:sendCancelMessage("Brew rejected: recipe must exist, be learned, and use valid development inputs.") return true end
	if not target:save() then player:sendCancelMessage("Brew changed progress in memory but could not be saved.") return true end
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format("Brew quality %d, potency %.2f, duration %dms, yield %d, stability %.2f, XP +%d.", result.quality, result.potency, result.durationMs, result.yield, result.stability, result.xpGranted))
	return true
end

brewTest:separator(" ")
brewTest:groupType("god")
brewTest:register()

local function twoPartDiscoveryCommand(player, param, usage)
	local parts = param:split(",")
	if #parts < 2 or parts[1]:trim() == "" or parts[2]:trim() == "" then
		player:sendCancelMessage(usage)
		return nil
	end
	local target = findTarget(player, parts[1])
	if not target then return nil end
	return target, parts[2]:trim(), parts
end

local discoveryInfo = TalkAction("/wdiscoveryinfo")

function discoveryInfo.onSay(player, words, param)
	logCommand(player, words, param)
	local target, discovery = twoPartDiscoveryCommand(player, param, "Usage: /wdiscoveryinfo <player>, <discovery>")
	if not target then return true end
	local info = target:getWizardDiscoveryInfo(discovery)
	if not info then player:sendCancelMessage("Wizard discovery not found.") return true end
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format(
		"%s / %s: state %s, assigned %s, area (%d,%d,%d)-(%d,%d,%d), assignedAt %d, discoveredAt %d, rewardAppliedAt %d.",
		target:getName(), info.id, info.state, info.assignedLocationId, info.fromX, info.fromY, info.fromZ, info.toX, info.toY, info.fromZ,
		info.assignedAt, info.discoveredAt, info.rewardAppliedAt))
	return true
end

discoveryInfo:separator(" ")
discoveryInfo:groupType("god")
discoveryInfo:register()

local discoveries = TalkAction("/wdiscoveries")

function discoveries.onSay(player, words, param)
	logCommand(player, words, param)
	local target = findTarget(player, param)
	if not target then return true end
	local rows = target:getWizardDiscoveries()
	if #rows == 0 then player:sendTextMessage(MESSAGE_EVENT_ADVANCE, target:getName() .. " has no assigned discoveries.") return true end
	local output = {}
	for _, row in ipairs(rows) do output[#output + 1] = string.format("%s=%s@%s", row.id, row.state, row.assignedLocationId) end
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, target:getName() .. ": " .. table.concat(output, ", "))
	return true
end

discoveries:separator(" ")
discoveries:groupType("god")
discoveries:register()

local discover = TalkAction("/wdiscover")

function discover.onSay(player, words, param)
	logCommand(player, words, param)
	local target, discovery = twoPartDiscoveryCommand(player, param, "Usage: /wdiscover <player>, <discovery>")
	if not target then return true end
	if not target:discoverWizardDiscovery(discovery) then player:sendCancelMessage("Discovery could not be claimed.") return true end
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format("%s now has discovery %s.", target:getName(), discovery))
	return true
end

discover:separator(" ")
discover:groupType("god")
discover:register()

local discoveryAssign = TalkAction("/wdiscoveryassign")

function discoveryAssign.onSay(player, words, param)
	logCommand(player, words, param)
	local target, discovery = twoPartDiscoveryCommand(player, param, "Usage: /wdiscoveryassign <player>, <discovery>")
	if not target then return true end
	if not target:assignWizardDiscovery(discovery) then player:sendCancelMessage("Discovery assignment failed.") return true end
	local info = target:getWizardDiscoveryInfo(discovery)
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format("%s / %s remains assigned to %s.", target:getName(), discovery, info.assignedLocationId))
	return true
end

discoveryAssign:separator(" ")
discoveryAssign:groupType("god")
discoveryAssign:register()

local discoveryReset = TalkAction("/wdiscoveryreset")

function discoveryReset.onSay(player, words, param)
	logCommand(player, words, param)
	local target, discovery, parts = twoPartDiscoveryCommand(player, param, "Usage: /wdiscoveryreset <player>, <discovery>[, clear-assignment]")
	if not target then return true end
	local clearAssignment = #parts >= 3 and normalized(parts[3]) == "clear-assignment"
	if not target:resetWizardDiscovery(discovery, clearAssignment) then player:sendCancelMessage("Discovery reset failed.") return true end
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format(
		"%s / %s reset. Assignment cleared: %s. Progression rewards were NOT reverted.", target:getName(), discovery, tostring(clearAssignment)))
	return true
end

discoveryReset:separator(" ")
discoveryReset:groupType("god")
discoveryReset:register()
