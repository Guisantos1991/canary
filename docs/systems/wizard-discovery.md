# Wizard Discovery System

Sprint 4 turns reading and exploration events into personal, persistent, one-shot discoveries. The server owns definitions, validation, assignment, rewards, persistence, and personal object visibility. The client never submits a discovery id or reward amount.

## Data model

Definitions live in `data/wizard/discoveries.json`. The root contains `locations` and `discoveries`.

A `WizardDiscoveryDefinition` contains:

- `id`: stable string persisted per character;
- `type`: `BOOK`, `PLAQUE`, `PAGE`, `SCROLL`, `WORLD_OBJECT`, `LOCATION`, `INGREDIENT`, `CREATURE`, `NPC`, or `ITEM`;
- `progressionSource`: `READING` or `EXPLORATION`;
- `repeatability`: currently only `ONE_SHOT`;
- `visibility`: `SHARED` or `PERSONAL`;
- optional `personalObject`, `actionId`, `ingredientItemId`, narrative `text`, and `developmentFixture`;
- `placement`: `FIXED` with one `locationId`, or `PLAYER_RANDOM` with a non-empty curated `locationPool`;
- `requirements.allOf`: discovery prerequisites;
- `rewards`: zero or more `SPELL_KNOWLEDGE`, `RECIPE_KNOWLEDGE`, or `DISCOVERY_UNLOCK`/`CLUE` entries.

A `WizardDiscoveryLocationDefinition` has a stable `id` and either one exact `position` or a small rectangular `area` with inclusive `from`/`to` coordinates on one floor. Coordinates are designer-curated; the system never generates arbitrary X/Y/Z values.

The registry rejects duplicate ids/action ids/ingredient ids, unknown enums, invalid positions and areas, missing or empty pools, unknown locations, unknown spell/recipe/requisite/unlock references, zero or negative numeric rewards, self-dependencies, and dependency cycles. Unlock targets are rewardless, fixed discoveries so granting a clue cannot silently skip another reward chain.

## Personal state and persistence

`player_wizard_discoveries` is keyed by `(player_id, discovery_id)` and stores only `ASSIGNED` or `DISCOVERED`. No row means unassigned. It also stores `assigned_location_id`, `assigned_at`, `discovered_at`, and `reward_applied_at`. Character login loads this state together with Wizard progression. Discovery writes are immediate and do not trigger a full player save.

Migration 62 creates the table for existing databases; `schema.sql` contains the same table for fresh databases. Existing spell and recipe progression is not removed or rewritten.

## Assignment

Assignment is lazy. It happens when the server first needs an eligible discovery's location: explicit assignment, visibility evaluation for a personal object, or location-trigger evaluation.

For `FIXED`, the only configured location is selected. For `PLAYER_RANDOM`, `std::uniform_int_distribution` chooses an index over the configured location vector. The server writes it using `INSERT IGNORE`, then reads the row back. The primary key arbitrates concurrent first assignment attempts, so all contenders use the database winner. Later calls and reconnects use the persisted value and never invoke randomness again.

If a persisted location no longer exists in configuration, the system logs an error, returns `CORRUPT_ASSIGNMENT`, and refuses a silent reroll. Administrators may intentionally clear the assignment with `/wdiscoveryreset <player>, <discovery>, clear-assignment`; this does not revert Knowledge or any other progression reward.

## Claim and one-shot rewards

An interaction resolves a real server-side item/action id, ingredient item, or indexed location trigger. The server validates the definition, trigger type, prerequisites, assignment, assigned location, and interaction distance. It then locks the in-process claim path and, for persistent players, locks the database row with `SELECT ... FOR UPDATE`.

Rewards are applied through `WizardKnowledgeSystem` and `WizardRecipeKnowledgeSystem`. Their existing allowed-source rules remain authoritative: for example, a `READING` reward aimed at an exploration-only spell is rejected and does not change Knowledge. The Discovery system does not duplicate or bypass acquisition-profile rules.

Changed spell/recipe rows, clue unlocks, the discovery transition, `discovered_at`, and `reward_applied_at` are written in one MariaDB transaction. On failure, the transaction rolls back and in-memory spell, recipe, and discovery snapshots are restored. Repeated, reconnect, and concurrent attempts see `DISCOVERED` and apply zero reward.

There is no safe incremental Magical Knowledge API in the current progression layer. Therefore Sprint 4 deliberately does not implement `MAGICAL_KNOWLEDGE` rewards or directly edit the skill column; a future progression API can be added as another validated reward adapter.

## Gameplay paths

- Books show their configured text on every use. The first valid use claims the discovery; rereads show text and apply nothing.
- Plaques and shared world objects remain on the map, continue to show text, and reward once per character.
- Personal pages/objects are world objects, never inventory rewards. Their discovery action ids make them immovable. Protocol map descriptions and incremental tile packets omit them unless the viewing character is eligible, assigned to that exact curated location, and has not discovered them.
- On successful personal-object interaction the server sends a complete tile refresh. The object disappears only for that player. Login, floor changes, teleports, walking out/in, and map refreshes all pass through the same server filter, so persisted state keeps it absent.
- Ingredient discovery is attached to a server-resolved look target by `ingredientItemId`. The first examination claims it; later examinations and any amount of collection do not add Knowledge.
- Location discoveries use a 32x32/floor bucket index. A step checks only triggers in the current bucket and exact containing areas, never every registered discovery.
- Chains currently implement `requirements.allOf`. A clue is represented by a personal discovery marker and can be used by the same prerequisite mechanism.
- `CREATURE` and `NPC` are accepted registry types but have no gameplay trigger in this sprint.

## Security and non-transferability

Normal network traffic supplies only an item position/stack/id or player movement. The server resolves the real object and maps its action id to a definition. A forged discovery id, reward amount, location, remote interaction, wrong personal assignment, unmet prerequisite, or unrelated action/item id cannot claim a discovery.

All map objects registered as discovery action ids return false from `Item::canBeMoved()`, blocking pickup and therefore inventory, trade, parcel, market, depot, and player-to-player transfer paths. Personal objects remain shared server map objects; only their network representation is character-specific.

## Administration

All commands are GOD-only:

- `/wdiscoveryinfo <player>, <discovery>` assigns if necessary and reports state, curated location/area, and timestamps;
- `/wdiscoveries <player>` lists the target's persisted in-memory states for diagnosis;
- `/wdiscoveryassign <player>, <discovery>` performs/queries stable assignment;
- `/wdiscover <player>, <discovery>` force-claims through the same reward/persistence transaction;
- `/wdiscoveryreset <player>, <discovery>[, clear-assignment]` resets the one-shot state and optionally the assignment. It never rolls progression back.

No player-facing discovery checklist is exposed.

## Development fixtures

The JSON contains one book, plaque, player-random secret page, fixed world object, player-random location, ingredient observation, clue, A+B chain, Reading/Exploration rewards, recipe rewards, and an exploration-only spell fixture. `wizard_discovery_fixtures.lua` places only the development interaction objects at the matching curated coordinates during startup and avoids duplicates.

These ids and texts are explicitly development-only. Sprint 5 can add real spell/potion content by referencing stable discovery and location ids without changing the claim, assignment, persistence, or visibility architecture.

## Future extensions

The registry types and reward/requirement boundaries allow later adapters for NPC teachers, creature observation, quests, puzzles, items, Magical Knowledge, mastery-gated secrets, passages, Dark Knowledge, seasonal/global events, and narrative UI. Those adapters must preserve server authority and must not turn combat farming into automatic exploration Knowledge.
