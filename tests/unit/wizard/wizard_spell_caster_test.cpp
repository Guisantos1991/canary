#include "creatures/monsters/monster.hpp"
#include "creatures/monsters/monsters.hpp"
#include "creatures/players/grouping/groups.hpp"
#include "creatures/players/player.hpp"
#include "creatures/players/vocations/vocation.hpp"
#include "game/game.hpp"
#include "items/tile.hpp"
#include "wizard/combat/wizard_area_system.hpp"
#include "wizard/spells/wizard_spell_caster.hpp"
#include "wizard/spells/wizard_spell_registry.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

namespace {
	class WizardSpellCasterTest : public ::testing::Test {
	protected:
		void SetUp() override {
			UPDATE_OTSYS_TIME();
			static uint32_t nextGuid = 700000;
			const auto offset = static_cast<uint16_t>(nextGuid++ - 700000);
			casterPosition = Position { static_cast<uint16_t>(200 + offset * 4), 200, 7 };
			targetPosition = Position { static_cast<uint16_t>(casterPosition.x + 1), casterPosition.y, casterPosition.z };

			casterTile = addTile(casterPosition);
			targetTile = addTile(targetPosition);
			caster = std::make_shared<Player>();
			caster->setGroup(std::make_shared<Group>());
			caster->setVocationForTesting(std::make_shared<Vocation>(1));
			caster->setLevel(100);
			caster->setGUID(nextGuid);
			caster->setName("WizardCaster" + std::to_string(nextGuid));
			ASSERT_TRUE(g_game().internalPlaceCreature(caster, casterPosition, false, true));
			ASSERT_TRUE(caster->learnWizardSpell(9001));
		}

		void TearDown() override {
			for (const auto &creature : placedCreatures) {
				removeTestCreature(creature);
			}
			removeTestCreature(caster);
		}

		static void SetUpTestSuite() {
			std::ifstream input(std::string(TESTS_SOURCE_DIR) + "/data/wizard/spells.json");
			auto json = nlohmann::json::parse(input);
			auto &spell = json["spells"][0];
			spell["castTimeMs"] = 0;
			spell["recoveryTimeMs"] = 1;
			spell["cooldownMs"] = 1;
			spell["requiresLineOfSight"] = false;
			spell["area"] = { { "pattern", "NONE" }, { "minSquares", 1 }, { "maxSquares", 1 } };
			spell["projectile"]["visualEffect"] = 0;
			spell["projectile"]["travelTimeMs"] = 0;
			const auto path = std::filesystem::temp_directory_path() / "wizard_spell_caster_registry.json";
			std::ofstream output(path);
			output << json.dump();
			output.close();
			std::string error;
			ASSERT_TRUE(g_wizardSpells().load(path.string(), error)) << error;
		}

		std::shared_ptr<Tile> addTile(const Position &position) {
			return g_game().map.getOrCreateTile(position, true);
		}

		std::shared_ptr<Monster> addMonster(const std::shared_ptr<Tile> &tile) {
			auto type = std::make_shared<MonsterType>("Wizard Target");
			type->info.health = 1000;
			type->info.healthMax = 1000;
			type->info.isAttackable = true;
			auto monster = std::make_shared<Monster>(type);
			if (!g_game().internalPlaceCreature(monster, tile->getPosition(), false, true)) {
				ADD_FAILURE() << "Failed to place Wizard test monster";
			}
			placedCreatures.emplace_back(monster);
			return monster;
		}

		std::shared_ptr<Player> addPlayerTarget(const std::shared_ptr<Tile> &tile) {
			static uint32_t nextTargetGuid = 800000;
			auto target = std::make_shared<Player>();
			target->setGroup(std::make_shared<Group>());
			target->setVocationForTesting(std::make_shared<Vocation>(1));
			target->setLevel(100);
			target->setGUID(nextTargetGuid++);
			target->setName("WizardPvpTarget" + std::to_string(nextTargetGuid));
			if (!g_game().internalPlaceCreature(target, tile->getPosition(), false, true)) {
				ADD_FAILURE() << "Failed to place Wizard PvP target";
			}
			placedCreatures.emplace_back(target);
			return target;
		}

		static void removeTestCreature(const std::shared_ptr<Creature> &creature) {
			if (!creature || creature->isRemoved()) {
				return;
			}
			if (const auto &tile = creature->getTile()) {
				tile->removeCreature(creature);
			}
			creature->removeList();
			creature->setRemoved();
		}

		void giveMana(uint32_t amount) {
			caster->setManaForTesting(amount, amount);
		}

		Position casterPosition;
		Position targetPosition;
		std::shared_ptr<Tile> casterTile;
		std::shared_ptr<Tile> targetTile;
		std::shared_ptr<Player> caster;
		std::vector<std::shared_ptr<Creature>> placedCreatures;
	};
}

TEST_F(WizardSpellCasterTest, ProtectionZoneCasterIsRejectedBeforeResources) {
	casterTile->setFlag(TILESTATE_PROTECTIONZONE);
	giveMana(100);
	const auto manaBefore = caster->getMana();
	EXPECT_FALSE(WizardSpellCaster::cast(caster, 9001, targetPosition));
	EXPECT_EQ(caster->getMana(), manaBefore);
	EXPECT_EQ(caster->getWizardRecoveryUntil(), 0);
	EXPECT_EQ(caster->getWizardCooldownUntil(9001), 0);
	EXPECT_EQ(caster->getWizardSpellProgress(9001)->uses, 0);
}

TEST_F(WizardSpellCasterTest, ProtectionZoneDestinationIsRejected) {
	targetTile->setFlag(TILESTATE_PROTECTIONZONE);
	giveMana(100);
	EXPECT_FALSE(WizardSpellCaster::cast(caster, 9001, targetPosition));
	EXPECT_EQ(caster->getWizardSpellProgress(9001)->uses, 0);
}

TEST_F(WizardSpellCasterTest, NormalOffensiveCastOutsideProtectionZoneIsAllowed) {
	giveMana(100);
	EXPECT_TRUE(WizardSpellCaster::cast(caster, 9001, targetPosition));
	EXPECT_LT(caster->getMana(), 100);
	EXPECT_EQ(caster->getWizardSpellProgress(9001)->uses, 1);
}

TEST_F(WizardSpellCasterTest, InfiniteManaAllowsCastWithoutDeduction) {
	caster->setFlag(PlayerFlags_t::HasInfiniteMana);
	ASSERT_EQ(caster->getMana(), 0);
	EXPECT_TRUE(WizardSpellCaster::cast(caster, 9001, targetPosition));
	EXPECT_EQ(caster->getMana(), 0);
	EXPECT_EQ(caster->getWizardSpellProgress(9001)->uses, 1);
}

TEST_F(WizardSpellCasterTest, InsufficientManaRejectsNormalPlayer) {
	EXPECT_FALSE(WizardSpellCaster::cast(caster, 9001, targetPosition));
	EXPECT_EQ(caster->getWizardSpellProgress(9001)->uses, 0);
}

TEST_F(WizardSpellCasterTest, AffectedCountMatchesSingleAndAreaTargets) {
	CombatDamage damage;
	WizardSpellCaster::setAffectedCount(damage, 1);
	EXPECT_EQ(damage.affected, 1);
	WizardSpellCaster::setAffectedCount(damage, 4);
	EXPECT_EQ(damage.affected, 4);
}

TEST_F(WizardSpellCasterTest, AreaCastPropagatesActualAffectedTargetCount) {
	auto areaSpell = *g_wizardSpells().getById(9001);
	areaSpell.area = { WizardAreaPattern::CIRCLE, 4, 4 };
	areaSpell.impactEffect = 0;
	for (const auto &position : WizardAreaSystem::resolve(areaSpell.area, targetPosition, 50, DIRECTION_NORTH)) {
		addMonster(addTile(position));
	}

	const auto targets = WizardSpellCaster::collectImpactTargets(caster, areaSpell, targetPosition, DIRECTION_NORTH, 50);
	ASSERT_EQ(targets.size(), 4);
	CombatDamage damage;
	WizardSpellCaster::setAffectedCount(damage, targets.size());
	EXPECT_EQ(damage.affected, 4);
}

TEST_F(WizardSpellCasterTest, NormalOffensivePvpImpactUsesCanaryCombatRules) {
	const auto target = addPlayerTarget(targetTile);
	const auto targets = WizardSpellCaster::collectImpactTargets(caster, *g_wizardSpells().getById(9001), targetPosition, DIRECTION_EAST, 50);
	ASSERT_EQ(targets.size(), 1);
	EXPECT_EQ(targets.front(), target);
}

TEST_F(WizardSpellCasterTest, ImpactUsesCurrentTileOccupancy) {
	auto target = addMonster(targetTile);
	const auto targets = WizardSpellCaster::resolveImpactOccupants(*g_wizardSpells().getById(9001), targetPosition, DIRECTION_EAST, 50);
	ASSERT_EQ(targets.size(), 1);
	EXPECT_EQ(targets.front(), target);
}

TEST_F(WizardSpellCasterTest, TargetThatDodgesBeforeImpactTakesNoDamage) {
	auto target = addMonster(targetTile);
	auto dodgeTile = addTile(Position { static_cast<uint16_t>(targetPosition.x + 1), targetPosition.y, targetPosition.z });
	g_game().map.moveCreature(target, dodgeTile, true);
	const auto targets = WizardSpellCaster::resolveImpactOccupants(*g_wizardSpells().getById(9001), targetPosition, DIRECTION_EAST, 50);
	EXPECT_TRUE(targets.empty());
}

TEST_F(WizardSpellCasterTest, TargetProtectedAtImpactTakesNoDamage) {
	auto target = addMonster(targetTile);
	targetTile->setFlag(TILESTATE_PROTECTIONZONE);
	const auto targets = WizardSpellCaster::collectImpactTargets(caster, *g_wizardSpells().getById(9001), targetPosition, DIRECTION_EAST, 50);
	EXPECT_TRUE(targets.empty());
}
