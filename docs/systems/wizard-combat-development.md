# Wizard combat development and runtime validation

This guide covers the local Ignis interaction between Canary and OTClient.
Canary remains authoritative for learning, range, line of sight, protection
zones, mana, cast time, recovery, cooldown, impact occupancy, and damage.

## Protocol

Wizard combat uses OTClient extended opcodes:

| Direction | Extended opcode | Payload |
| --- | ---: | --- |
| Canary to OTClient | `91` | JSON arm metadata: `spellId`, `name`, `range`, mana/timing values, `effectiveSquares`, and server-resolved `areaOffsets` |
| OTClient to Canary | `90` | JSON cast request: `spellId`, `x`, `y`, and `z` |

Opcode `90` accepts only the spell identifier and a map position. It neither
requires nor accepts a Creature ID. Battle List selection and the creature
currently being attacked are not consulted by the Wizard module.

The flow is:

```text
!wspell ignis or the Arm Ignis keybind
  -> Canary validates the spell and learned state
  -> opcode 91 arms OTClient targeting with server-derived metadata
  -> mouse hover highlights the candidate tile and effective area
  -> left click sends opcode 90 with spellId + Position
  -> OTClient exits targeting immediately
  -> Canary validates the request and schedules cast/projectile/impact
  -> impact resolves the creatures occupying the affected tiles at that time
```

Pressing Escape or right-clicking during targeting cancels it. Game logout,
character change, module unload/reload, and invalid local game state also clear
the target cursor and highlights. Arming another valid spell replaces the
previous armed state.

The client marks a same-floor tile inside the advertised range as selectable
and marks a clearly invalid tile in red. This is UX feedback only. A red tile
can still be sent so Canary can reject it with the authoritative error message.
The orange area preview comes from exact offsets resolved by
`WizardAreaSystem`; OTClient does not duplicate the Magical Power formula.

## Low-memory development loop

Run one heavy task at a time. Keep `CMAKE_BUILD_PARALLEL_LEVEL=1`, use `-j1`
where supported, preserve CMake/vcpkg/Docker caches, and keep
`SPEED_UP_BUILD_UNITY=OFF` and `OPTIONS_ENABLE_IPO=OFF` for Canary development.

| Change | Required action |
| --- | --- |
| Canary C++ | Incremental native configure/build with one job, Wizard tests, then one cached Docker server build at the end |
| OTClient C++ | Incremental OTClient build with one job |
| OTClient Lua/OTMOD | Reload `game_wizard` with `g_modules.getModule('game_wizard'):reload()` when the client terminal is available, otherwise restart OTClient; no C++ rebuild is needed |
| `data/wizard/*.json` | `docker compose restart server`; Wizard JSON is loaded at server startup and the development bind mount avoids rebuilding the image |
| `data/scripts/**/*.lua` | Use `/reload scripts` as GOD when safe, otherwise `docker compose restart server`; the development bind mount avoids rebuilding the image |

From `canary/docker`, `docker-compose.override.yml` mounts these source
directories read-only over the matching paths already present in the local
runtime image:

```text
../data/wizard  -> /canary/data/wizard
../data/scripts -> /canary/data/scripts
```

The server image remains `wizard-canary-local:latest`. Do not use `--no-cache`
for the normal loop.

## Two-client test setup

The local database already has separate test accounts. Production connection
limits do not need to change. All default test accounts use password `test`.

1. Start the local stack and point OTClient at `http://localhost:8088/login`.
2. In client A, log in as `@test15` and select `ADM1`.
3. In client B, log in as `@test1` and select `Paladin 1`.
4. On `ADM1`, run:

```text
/wskill Paladin 1, power, 73
/wskill Paladin 1, control, 54
/wskill Paladin 1, knowledge, 81
/wskill Paladin 1, combat, 67
/wlearn Paladin 1, ignis
```

5. Log client A out, log in as `@test2`, and select `Paladin 2` as the target.
6. Keep client B on `Paladin 1` as the caster. Run `!wskills`, then
   `!wspell ignis`.

## Manual acceptance sequence

Use nearby same-floor tiles for the baseline and keep both characters outside
a protection zone unless a step says otherwise.

1. **Target mode and empty tile:** after `!wspell ignis`, move the mouse over
   the map. A selectable center tile is green/yellow and the effective area is
   orange. Click a valid empty tile. Target mode ends and a position-targeted
   projectile travels to that tile.
2. **Cancel:** arm again, press Escape, then left-click the map. The click must
   behave normally and must not cast. Repeat with right-click cancellation.
3. **Hotkey:** press the configured `Wizard Magic -> Arm Ignis` key (default
   `1`). It sends `!wspell ignis` for server validation and only arms targeting;
   it must not cast until a tile is clicked.
4. **Battle List independence:** attack or select `Paladin 2`, arm Ignis, and
   click an unrelated empty tile. The projectile must use the clicked tile and
   must not redirect to `Paladin 2`.
5. **Hit:** place `Paladin 2` on the selected tile, keep the target still, and
   click that tile. After the 500 ms travel time, the target should take damage.
6. **Dodge:** click the tile occupied by `Paladin 2`, then move the target off
   that tile before impact. The projectile still ends at the selected position
   and the moved target takes no damage.
7. **Miss and no autoaim:** click beside a stationary target. Repeat after
   setting Power to 100 and Combat to 100. The target must still take no damage
   unless its current tile is inside the actual area.
8. **Range:** hover and click a tile more than seven squares away. It should be
   marked invalid and Canary must answer `That tile is out of range.` without a
   projectile, mana use, or cooldown.
9. **Line of sight:** target a same-floor tile behind a projectile-blocking wall.
   Canary must answer `The path to that tile is blocked.` without spending
   resources or creating a projectile.
10. **Protection zone:** put `Paladin 1` inside PZ and `Paladin 2` outside. Arm
    and click the target tile. Canary must reject the offensive cast; there must
    be no projectile, mana loss, or cooldown.
11. **Mana:** with mastery below 10 uses, set Control to 1 and record mana before
    and after one successful Ignis cast; the current configuration costs 30.
    Wait for cooldown, set Control to 100, repeat, and expect 26. The UI values
    are observational; Canary performs the deduction.
12. **Exhaustion:** set Control and Combat to 1 and click repeated armed casts as
    quickly as possible. Recovery/cooldown messages must block spam. Repeat at
    100/100: reductions apply within configured caps, while the 2500 ms Ignis
    cooldown still prevents spam.
13. **Progressive area:** arm at Power 73 and count the orange preview (9
    squares). Repeat at Power 99 (11 squares) and Power 100 (12 squares).

Expected failure messages include unknown/unlearned spell, insufficient mana,
active recovery, active cooldown, out of range, blocked line of sight, caster
in protection zone, and invalid tile. A failed pre-cast validation must not
produce a projectile or consume mana/cooldown.
