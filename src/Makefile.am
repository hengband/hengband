## Makefile.am -- Process this file with automake to process Makefile.in

AUTOMAKE_OPTIONS = foreign subdir-objects

bin_PROGRAMS = hengband

hengband_SOURCES = \
	art-definition/art-accessory-types.h \
	art-definition/art-armor-types.h \
	art-definition/art-bow-types.h \
	art-definition/art-protector-types.h \
	art-definition/art-sword-types.h \
	art-definition/art-weapon-types.h \
	art-definition/random-art-effects.h \
	\
	autopick/autopick.c autopick/autopick.h \
	autopick/autopick-commands-table.h autopick/autopick-dirty-flags.h \
	autopick/autopick-editor-table.h autopick/autopick-flags-table.h \
	autopick/autopick-key-flag-process.h autopick/autopick-keys-table.h \
	autopick/autopick-menu-data-table.c autopick/autopick-menu-data-table.h \
	autopick/autopick-methods-table.h \
	autopick/autopick-util.c autopick/autopick-util.h \
	autopick/autopick-entry.c autopick/autopick-entry.h \
	autopick/autopick-initializer.c autopick/autopick-initializer.h \
	autopick/autopick-matcher.c autopick/autopick-matcher.h \
	autopick/autopick-describer.c autopick/autopick-describer.h \
	autopick/autopick-destroyer.c autopick/autopick-destroyer.h \
	autopick/autopick-reader-writer.c autopick/autopick-reader-writer.h \
	autopick/autopick-finder.c autopick/autopick-finder.h \
	autopick/autopick-pref-processor.c autopick/autopick-pref-processor.h \
	autopick/autopick-drawer.c autopick/autopick-drawer.h \
	autopick/autopick-inserter-killer.c autopick/autopick-inserter-killer.h \
	autopick/autopick-registry.c autopick/autopick-registry.h \
	autopick/autopick-command-menu.c autopick/autopick-command-menu.h \
	autopick/autopick-editor-util.c autopick/autopick-editor-util.h \
	autopick/autopick-editor-command.c autopick/autopick-editor-command.h \
	\
	birth/birth-explanations-table.c birth/birth-explanations-table.h \
	birth/character-builder.c birth/character-builder.h \
	birth/history.c birth/history.h \
	birth/birth-util.c birth/birth-util.h \
	birth/birth-select-realm.c birth/birth-select-realm.h \
	birth/quick-start.c birth/quick-start.h \
	birth/birth-stat.c birth/birth-stat.h \
	birth/history-generator.c birth/history-generator.h \
	birth/birth-body-spec.c birth/birth-body-spec.h \
	birth/initial-equipments-table.c birth/initial-equipments-table.h \
	birth/inventory-initializer.c birth/inventory-initializer.h \
	birth/game-play-initializer.c birth/game-play-initializer.h \
	birth/history-editor.c birth/history-editor.h \
	birth/birth-select-race.c birth/birth-select-race.h \
	birth/birth-select-class.c birth/birth-select-class.h \
	birth/birth-select-personality.c birth/birth-select-personality.h \
	birth/auto-roller.c birth/auto-roller.h \
	birth/birth-wizard.c birth/birth-wizard.h \
	\
	cmd/cmd-basic.c cmd/cmd-basic.h \
	cmd/cmd-draw.c cmd/cmd-draw.h \
	cmd/cmd-visuals.c cmd/cmd-visuals.h \
	\
	cmd-action/cmd-attack.c cmd-action/cmd-attack.h \
	cmd-action/cmd-hissatsu.c cmd-action/cmd-hissatsu.h \
	cmd-action/cmd-mane.c cmd-action/cmd-mane.h \
	cmd-action/cmd-pet.c cmd-action/cmd-pet.h \
	cmd-action/cmd-spell.c cmd-action/cmd-spell.h \
	\
	cmd-building/cmd-building.c cmd-building/cmd-building.h \
	cmd-building/cmd-inn.c cmd-building/cmd-inn.h \
	\
	cmd-io/cmd-autopick.c cmd-io/cmd-autopick.h \
	cmd-io/cmd-diary.c cmd-io/cmd-diary.h \
	cmd-io/cmd-dump.c cmd-io/cmd-dump.h \
	cmd-io/cmd-gameoption.c cmd-io/cmd-gameoption.h \
	cmd-io/cmd-help.c cmd-io/cmd-help.h \
	cmd-io/cmd-knowledge.c cmd-io/cmd-knowledge.h \
	cmd-io/cmd-lore.c cmd-io/cmd-lore.h \
	cmd-io/cmd-macro.c cmd-io/cmd-macro.h \
	cmd-io/cmd-menu-content-table.c cmd-io/cmd-menu-content-table.h \
	cmd-io/cmd-process-screen.c cmd-io/cmd-process-screen.h \
	cmd-io/cmd-save.c cmd-io/cmd-save.h \
	cmd-io/diary-subtitle-table.c cmd-io/diary-subtitle-table.h \
	cmd-io/feeling-table.c cmd-io/feeling-table.h \
	\
	cmd-item/cmd-activate.c cmd-item/cmd-activate.h \
	cmd-item/cmd-eat.c cmd-item/cmd-eat.h \
	cmd-item/cmd-item.c cmd-item/cmd-item.h \
	cmd-item/cmd-magiceat.c cmd-item/cmd-magiceat.h \
	cmd-item/cmd-quaff.c cmd-item/cmd-quaff.h \
	cmd-item/cmd-read.c cmd-item/cmd-read.h \
	cmd-item/cmd-smith.c cmd-item/cmd-smith.h \
	cmd-item/cmd-usestaff.c cmd-item/cmd-usestaff.h \
	cmd-item/cmd-zaprod.c cmd-item/cmd-zaprod.h \
	cmd-item/cmd-zapwand.c cmd-item/cmd-zapwand.h \
	\
	combat/attack-power-table.c combat/attack-power-table.h \
	combat/aura-counterattack.c combat/aura-counterattack.h \
	combat/hallucination-attacks-table.c combat/hallucination-attacks-table.h \
	combat/martial-arts-table.c combat/martial-arts-table.h \
	combat/attack-accuracy.c combat/attack-accuracy.h \
	combat/slaying.c combat/slaying.h \
	combat/combat-options-type.h \
	combat/attack-criticality.c combat/attack-criticality.h \
	combat/vorpal-weapon.c combat/vorpal-weapon.h \
	combat/shoot.c combat/shoot.h \
	\
	core/game-play.c core/game-play.h \
	core/show-file.c core/show-file.h \
	core/stuff-handler.c core/stuff-handler.h \
	core/special-internal-keys.h \
	core/output-updater.c core/output-updater.h \
	core/game-closer.c core/game-closer.h \
	core/turn-compensator.c core/turn-compensator.h \
	core/hp-mp-regenerator.c core/hp-mp-regenerator.h \
	core/hp-mp-processor.c core/hp-mp-processor.h \
	core/magic-effects-timeout-reducer.c core/magic-effects-timeout-reducer.h \
	core/player-processor.c core/player-processor.h \
	core/scores.c core/scores.h \
	core/sort.c core/sort.h \
	core/speed-table.c core/speed-table.h \
	core/visuals-reseter.c core/visuals-reseter.h \
	\
	dungeon/dungeon.c dungeon/dungeon.h \
	dungeon/dungeon-processor.c dungeon/dungeon-processor.h \
	dungeon/quest.h dungeon/quest.c \
	\
	effect/effect-feature.c effect/effect-feature.h \
	effect/effect-item.c effect/effect-item.h \
	effect/spells-effect-util.c effect/spells-effect-util.h \
	effect/effect-player.c effect/effect-player.h \
	effect/effect-monster.c effect/effect-monster.h \
	effect/effect-monster-curse.c effect/effect-monster-curse.h \
	effect/effect-monster-util.c effect/effect-monster-util.h \
	effect/effect-monster-resist-hurt.c effect/effect-monster-resist-hurt.h \
	effect/effect-monster-psi.c effect/effect-monster-psi.h \
	effect/effect-monster-oldies.c effect/effect-monster-oldies.h \
	effect/effect-monster-charm.c effect/effect-monster-charm.h \
	effect/effect-monster-lite-dark.c effect/effect-monster-lite-dark.h \
	effect/effect-monster-evil.c effect/effect-monster-evil.h \
	effect/effect-monster-spirit.c effect/effect-monster-spirit.h \
	effect/effect-monster-switcher.c effect/effect-monster-switcher.h \
	effect/effect-player-util.h \
	effect/effect-player-resist-hurt.c effect/effect-player-resist-hurt.h \
	effect/effect-player-oldies.c effect/effect-player-oldies.h \
	effect/effect-player-curse.c effect/effect-player-curse.h \
	effect/effect-player-spirit.c effect/effect-player-spirit.h \
	effect/effect-player-switcher.c effect/effect-player-switcher.h \
	effect/effect-characteristics.h \
	\
	floor/fixed-map-generator.c floor/fixed-map-generator.h \
	floor/floor.c floor/floor.h \
	floor/floor-events.c floor/floor-events.h \
	floor/floor-generate.c floor/floor-generate.h \
	floor/floor-object.c floor/floor-object.h \
	floor/floor-save.h floor/floor-save.c \
	floor/floor-streams.c floor/floor-streams.h \
	floor/floor-town.h floor/floor-town.c \
	floor/geometry.c floor/geometry.h \
	floor/pattern-walk.c floor/pattern-walk.h \
	floor/wild.h floor/wild.c \
	\
	grid/feature.c grid/feature.h \
	grid/grid.c grid/grid.h grid/trap.c grid/trap.h \
	\
	info-reader/artifact-reader.c info-reader/artifact-reader.h \
	info-reader/dungeon-info-tokens-table.c info-reader/dungeon-info-tokens-table.h \
	info-reader/dungeon-reader.c info-reader/dungeon-reader.h \
	info-reader/ego-reader.c info-reader/ego-reader.h \
	info-reader/feature-info-tokens-table.c info-reader/feature-info-tokens-table.h \
	info-reader/feature-reader.c info-reader/feature-reader.h \
	info-reader/fixed-map-parser.c info-reader/fixed-map-parser.h \
	info-reader/general-parser.c info-reader/general-parser.h \
	info-reader/info-reader-util.c info-reader/info-reader-util.h \
	info-reader/kind-info-tokens-table.c info-reader/kind-info-tokens-table.h \
	info-reader/kind-reader.c info-reader/kind-reader.h \
	info-reader/magic-reader.c info-reader/magic-reader.h \
	info-reader/parse-error-types.h \
	info-reader/race-info-tokens-table.c info-reader/race-info-tokens-table.h \
	info-reader/race-reader.c info-reader/race-reader.h \
	info-reader/random-grid-effect-types.h \
	info-reader/skill-reader.c info-reader/skill-reader.h \
	info-reader/vault-reader.c info-reader/vault-reader.h \
	\
	inventory/inventory-curse.c inventory/inventory-curse.h \
	inventory/inventory-damage.c inventory/inventory-damage.h \
	inventory/inventory-object.c inventory/inventory-object.h \
	inventory/pack-overflow.c inventory/pack-overflow.h \
	inventory/player-inventory.c inventory/player-inventory.h \
	inventory/recharge-processor.c inventory/recharge-processor.h \
	\
	io/files-util.c io/files-util.h \
	io/gf-descriptions.c io/gf-descriptions.h \
	io/targeting.c io/targeting.h \
	io/tokenizer.c io/tokenizer.h \
	io/interpret-pref-file.c io/interpret-pref-file.h \
	io/read-pref-file.c io/read-pref-file.h \
	io/exit-panic.c io/exit-panic.h \
	io/write-diary.c io/write-diary.h \
	io/mutations-dump.c io/mutations-dump.h \
	io/report.c io/report.h \
	io/signal-handlers.c io/signal-handlers.h \
	io/uid-checker.c io/uid-checker.h \
	io/input-key-processor.c io/input-key-processor.h \
	io/load.c io/load.h \
	io/save.c io/save.h \
	io/inet.c io/inet.h \
	io/chuukei.c io/chuukei.h \
	io/pref-file-expressor.c io/pref-file-expressor.h \
	\
	io-dump/character-dump.c io-dump/character-dump.h \
	io-dump/dump-remover.c io-dump/dump-remover.h \
	io-dump/dump-util.c io-dump/dump-util.h \
	io-dump/player-status-dump.c io-dump/player-status-dump.h \
	io-dump/special-class-dump.c io-dump/special-class-dump.h \
	\
	knowledge/knowledge-autopick.c knowledge/knowledge-autopick.h \
	knowledge/knowledge-experiences.c knowledge/knowledge-experiences.h \
	knowledge/knowledge-features.c knowledge/knowledge-features.h \
	knowledge/knowledge-inventory.c knowledge/knowledge-inventory.h \
	knowledge/knowledge-items.c knowledge/knowledge-items.h \
	knowledge/knowledge-monsters.c knowledge/knowledge-monsters.h \
	knowledge/knowledge-mutations.c knowledge/knowledge-mutations.h \
	knowledge/knowledge-uniques.c knowledge/knowledge-uniques.h \
	knowledge/knowledge-self.c knowledge/knowledge-self.h \
	knowledge/knowledge-quests.c knowledge/knowledge-quests.h \
	knowledge/lighting-level-table.c knowledge/lighting-level-table.h \
	knowledge/monster-group-table.c knowledge/monster-group-table.h \
	knowledge/object-group-table.c knowledge/object-group-table.h \
	\
	locale/japanese.c locale/japanese.h locale/english.c locale/english.h \
	\
	main.c main-x11.c main-gcu.c \
	\
	main/music-definitions-table.c main/music-definitions-table.h \
	main/sound-definitions-table.c main/sound-definitions-table.h \
	main/x11-gamma-builder.c main/x11-gamma-builder.h \
	main/init.c main/init.h \
	\
	market/arena-info-table.c market/arena-info-table.h \
	market/building-util.c market/building-util.h \
	market/play-gamble.c market/play-gamble.h \
	market/poker.c market/poker.h \
	market/articles-on-sale.c market/articles-on-sale.h \
	market/gold-magnification-table.c market/gold-magnification-table.h \
	market/arena.c market/arena.h \
	market/bounty-prize-table.c market/bounty-prize-table.h \
	market/bounty.c market/bounty.h \
	market/building-recharger.c market/building-recharger.h \
	market/building-quest.c market/building-quest.h \
	market/building-service.c market/building-service.h \
	market/building-craft-weapon.c market/building-craft-weapon.h \
	market/building-craft-armor.c market/building-craft-armor.h \
	market/building-craft-fix.c market/building-craft-fix.h \
	market/building-monster.c market/building-monster.h \
	market/building-enchanter.c market/building-enchanter.h \
	market/building-actions-table.h \
	\
	melee/monster-attack-monster.c melee/monster-attack-monster.h \
	melee/melee-postprocess.c melee/melee-postprocess.h \
	melee/melee-switcher.c melee/melee-switcher.h \
	melee/melee-util.c melee/melee-util.h \
	\
	mind/drs-typs.h \
	mind/mind.c mind/mind.h \
	mind/mind-cavalry.c mind/mind-cavalry.h \
	mind/mind-force-trainer.c mind/mind-force-trainer.h \
	mind/mind-mindcrafter.c mind/mind-mindcrafter.h \
	mind/mind-mirror-master.c mind/mind-mirror-master.h \
	mind/mind-ninja.c mind/mind-ninja.h \
	mind/mind-samurai.c mind/mind-samurai.h \
	mind/mind-sniper.c mind/mind-sniper.h \
	mind/mind-warrior.c mind/mind-warrior.h \
	mind/mind-warrior-mage.c mind/mind-warrior-mage.h \
	mind/monk-attack.c mind/monk-attack.h \
	mind/snipe-types.h \
	mind/racial.c mind/racial.h \
	mind/racial-android.c mind/racial-android.h \
	mind/racial-balrog.c mind/racial-balrog.h \
	mind/racial-draconian.c mind/racial-draconian.h \
	mind/racial-vampire.c mind/racial-vampire.h \
	\
	monster/horror-descriptions.c monster/horror-descriptions.h \
	monster/monster1.c monster/monster1.h \
	monster/monster2.c monster/monster2.h \
	monster/monster-attack.c monster/monster-attack.h \
	monster/monster-death.c monster/monster-death.h \
	monster/monster-description-types.h \
	monster/monster-direction.c monster/monster-direction.h \
	monster/monster-dist-offsets.c monster/monster-dist-offsets.h \
	monster/monster-flag-types.h \
	monster/monster-move.c monster/monster-move.h \
	monster/monster-object.c monster/monster-object.h \
	monster/monster-processor.c monster/monster-processor.h \
	monster/monster-processor-util.c monster/monster-processor-util.h \
	monster/monster-runaway.c monster/monster-runaway.h \
	monster/monster-safety-hiding.c monster/monster-safety-hiding.h \
	monster/monster-timed-effect-types.h \
	monster/place-monster-types.h \
	monster/smart-learn-types.h \
	monster/monster-status.c monster/monster-status.h \
	monster/monster-sweep-grid.c monster/monster-sweep-grid.h \
	monster/monster-update.c monster/monster-update.h \
	monster/monster-util.c monster/monster-util.h \
	monster/quantum-effect.c monster/quantum-effect.h \
	\
	monster-attack/insults-moans.c monster-attack/insults-moans.h \
	monster-attack/monster-attack-describer.c monster-attack/monster-attack-describer.h \
	monster-attack/monster-attack-effect.h \
	monster-attack/monster-attack-player.c monster-attack/monster-attack-player.h \
	monster-attack/monster-attack-status.c monster-attack/monster-attack-status.h \
	monster-attack/monster-attack-switcher.c monster-attack/monster-attack-switcher.h \
	monster-attack/monster-attack-types.c monster-attack/monster-attack-types.h \
	monster-attack/monster-attack-util.c monster-attack/monster-attack-util.h \
	monster-attack/monster-eating.c monster-attack/monster-eating.h \
	\
	monster-lore/lore-store.c monster-lore/lore-store.h \
	monster-lore/monster-lore.c monster-lore/monster-lore.h \
	\
	monster-race/monster-race.c monster-race/monster-race.h \
	monster-race/monster-race-hook.c monster-race/monster-race-hook.h \
	monster-race/race-flags-resistance.h \
	monster-race/race-flags1.h monster-race/race-flags2.h \
	monster-race/race-flags3.h monster-race/race-flags4.h \
	monster-race/race-flags-ability1.h monster-race/race-flags-ability2.h \
	monster-race/race-flags7.h monster-race/race-flags8.h \
	monster-race/race-flags9.h \
	monster-race/race-indice-types.h \
	\
	mspell/monster-spell.h \
	mspell/mspells1.c mspell/mspells2.c mspell/mspells3.c \
	mspell/assign-monster-spell.c mspell/assign-monster-spell.h \
	mspell/mspell-summon.c mspell/mspell-summon.h \
	mspell/mspell-util.c mspell/mspell-util.h \
	mspell/mspell-curse.c mspell/mspell-curse.h \
	mspell/mspell-type.h \
	mspell/mspell-damage-calculator.c mspell/mspell-damage-calculator.h \
	mspell/mspell-learn-checker.c mspell/mspell-learn-checker.h \
	mspell/mspell-breath.c mspell/mspell-breath.h \
	mspell/mspell-ball.c mspell/mspell-ball.h \
	mspell/mspell-bolt.c mspell/mspell-bolt.h \
	mspell/mspell-floor.c mspell/mspell-floor.h \
	mspell/mspell-special.c mspell/mspell-special.h \
	mspell/mspell-status.c mspell/mspell-status.h \
	mspell/mspell-summon-checker.c mspell/mspell-summon-checker.h \
	mspell/mspell-particularity.c mspell/mspell-particularity.h \
	\
	mutation/mutation.c mutation/mutation.h \
	mutation/mutation-processor.c mutation/mutation-processor.h \
	\
	object/object-broken.c object/object-broken.h \
	object/object-flags.c object/object-flags.h \
	object/object-flavor.c object/object-flavor.h \
	object/object-generator.c object/object-generator.h \
	object/object-hook.c object/object-hook.h \
	object/object-info.c object/object-info.h \
	object/object-kind.c object/object-kind.h \
	object/object-kind-hook.c object/object-kind-hook.h \
	object/object-stack.c object/object-stack.h \
	object/object-value.c object/object-value.h \
	object/object-value-calc.c object/object-value-calc.h \
	object/lite-processor.c object/lite-processor.h \
	object/tval-types.h \
	object/warning.c object/warning.h \
	object/object-mark-types.h \
	object/item-use-flags.h \
	\
	object-enchant/activation-info-table.c object-enchant/activation-info-table.h \
	object-enchant/apply-magic.c object-enchant/apply-magic.h \
	object-enchant/apply-magic-accessory.c object-enchant/apply-magic-accessory.h \
	object-enchant/apply-magic-armor.c object-enchant/apply-magic-armor.h \
	object-enchant/apply-magic-others.c object-enchant/apply-magic-others.h \
	object-enchant/apply-magic-weapon.c object-enchant/apply-magic-weapon.h \
	object-enchant/artifact.c object-enchant/artifact.h \
	object-enchant/dragon-breaths-table.c object-enchant/dragon-breaths-table.h \
	object-enchant/object-boost.c object-enchant/object-boost.h \
	object-enchant/object-curse.c object-enchant/object-curse.h \
	object-enchant/object-ego.c object-enchant/object-ego.h \
	object-enchant/item-apply-magic.h object-enchant/item-feeling.h \
	object-enchant/old-ego-extra-values.h object-enchant/special-object-flags.h \
	object-enchant/random-art-bias-types.h \
	object-enchant/tr-types.h object-enchant/trc-types.h \
	object-enchant/trg-types.h \
	\
	perception/identification.c perception/identification.h \
	perception/object-perception.c perception/object-perception.h \
	perception/simple-perception.c perception/simple-perception.h \
	\
	pet/pet-fall-off.c pet/pet-fall-off.h \
	pet/pet-util.c pet/pet-util.h \
	\
	player/avatar.h player/avatar.c \
	player/eldritch-horror.c player/eldritch-horror.h \
	player/mimic-info-table.c player/mimic-info-table.h \
	player/process-death.c player/process-death.h \
	player/process-name.c player/process-name.h \
	player/race-info-table.c player/race-info-table.h\
	player/race-resistances.c player/race-resistances.h \
	player/permanent-resistances.c player/permanent-resistances.h \
	player/temporary-resistances.c player/temporary-resistances.h \
	player/digestion-processor.c player/digestion-processor.h \
	player/player-move.c player/player-move.h \
	player/player-damage.c player/player-damage.h \
	player/player-status.c player/player-status.h \
	player/player-effects.c player/player-effects.h \
	player/player-skill.c player/player-skill.h \
	player/player-class.c player/player-class.h \
	player/player-race.c player/player-race.h \
	player/player-races-table.h \
	player/player-classes-table.h \
	player/player-personalities-table.h \
	player/patron.h player/patron.c \
	player/player-personality.c player/player-personality.h \
	player/player-sex.c player/player-sex.h player/selfinfo.c player/selfinfo.h \
	\
	player-attack/player-attack-util.h \
	player-attack/attack-chaos-effect.c player-attack/attack-chaos-effect.h \
	player-attack/blood-sucking-processor.c player-attack/blood-sucking-processor.h \
	player-attack/player-attack.c player-attack/player-attack.h \
	\
	realm/realm-arcane.c realm/realm-arcane.h \
	realm/realm-chaos.c realm/realm-chaos.h \
	realm/realm-craft.c realm/realm-craft.h \
	realm/realm-crusade.c realm/realm-crusade.h \
	realm/realm-daemon.c realm/realm-daemon.h \
	realm/realm-death.c realm/realm-death.h \
	realm/realm-hex.c realm/realm-hex.h \
	realm/realm-hex-numbers.h \
	realm/realm-hissatsu.c realm/realm-hissatsu.h \
	realm/realm-life.c realm/realm-life.h \
	realm/realm-names-table.c realm/realm-names-table.h \
	realm/realm-nature.c realm/realm-nature.h \
	realm/realm-song.c realm/realm-song.h \
	realm/realm-sorcery.c realm/realm-sorcery.h \
	realm/realm-trump.c realm/realm-trump.h \
	\
	room/rooms.c room/rooms.h room/rooms-city.c room/rooms-city.h \
	room/rooms-fractal.c room/rooms-fractal.h \
	room/rooms-normal.c room/rooms-normal.h \
	room/rooms-special.c room/rooms-special.h \
	room/rooms-trap.c room/rooms-trap.h \
	room/rooms-vault.c room/rooms-vault.h \
	room/rooms-pit-nest.c room/rooms-pit-nest.h \
	room/pit-nest-kinds-table.c room/pit-nest-kinds-table.h \
	\
	specific-object/bow.c specific-object/bow.h \
	specific-object/chest.c specific-object/chest.h \
	specific-object/death-scythe.c specific-object/death-scythe.h \
	specific-object/torch.c specific-object/torch.h \
	\
	spell/music-checker.c spell/music-checker.h \
	spell/process-effect.c spell/process-effect.h \
	spell/range-calc.c spell/range-calc.h \
	spell/spells3.c spell/spells3.h \
	spell/spells-describer.c spell/spells-describer.h \
	spell/spells-diceroll.c spell/spells-diceroll.h \
	spell/spells-execution.c spell/spells-execution.h \
	spell/spells-object.c spell/spells-object.h \
	spell/spells-staff-only.c spell/spells-staff-only.h \
	spell/spells-summon.c spell/spells-summon.h \
	spell/spells-status.c spell/spells-status.h \
	spell/spells-type.h spell/spells-util.h \
	spell/technic-info-table.c spell/technic-info-table.h \
	\
	spell-kind/earthquake.c spell-kind/earthquake.h \
	spell-kind/spells-beam.c spell-kind/spells-beam.h \
	spell-kind/spells-charm.c spell-kind/spells-charm.h \
	spell-kind/spells-detection.c spell-kind/spells-detection.h \
	spell-kind/spells-floor.c spell-kind/spells-floor.h \
	spell-kind/spells-genocide.c spell-kind/spells-genocide.h \
	spell-kind/spells-grid.c spell-kind/spells-grid.h \
	spell-kind/spells-launcher.c spell-kind/spells-launcher.h \
	spell-kind/spells-lite.c spell-kind/spells-lite.h \
	spell-kind/spells-neighbor.c spell-kind/spells-neighbor.h \
	spell-kind/spells-pet.c spell-kind/spells-pet.h \
	spell-kind/spells-random.c spell-kind/spells-random.h \
	spell-kind/spells-sight.c spell-kind/spells-sight.h \
	spell-kind/spells-specific-bolt.c spell-kind/spells-specific-bolt.h \
	spell-kind/spells-teleport.c spell-kind/spells-teleport.h \
	spell-kind/spells-world.c spell-kind/spells-world.h \
	\
	spell-realm/spells-crusade.c spell-realm/spells-crusade.h \
	spell-realm/spells-hex.c spell-realm/spells-hex.h \
	spell-realm/spells-trump.c spell-realm/spells-trump.h \
	\
	store/store.c store/store.h \
	store/black-market.c store/black-market.h \
	store/rumor.c store/rumor.h \
	store/say-comments.c store/say-comments.h \
	store/store-util.c store/store-util.h \
	store/store-owners.c store/store-owners.h \
	store/store-owner-comments.c store/store-owner-comments.h \
	\
	sv-definition/sv-amulet-types.h sv-definition/sv-armor-types.h \
	sv-definition/sv-bow-types.h sv-definition/sv-digging-types.h \
	sv-definition/sv-food-types.h sv-definition/sv-lite-types.h \
	sv-definition/sv-potion-types.h sv-definition/sv-protector-types.h \
	sv-definition/sv-ring-types.h sv-definition/sv-rod-types.h \
	sv-definition/sv-scroll-types.h sv-definition/sv-staff-types.h \
	sv-definition/sv-wand-types.h sv-definition/sv-weapon-types.h \
	sv-definition/sv-other-types.h \
	\
	system/angband.h system/angband-version.h \
	system/h-basic.h system/h-config.h \
	system/h-define.h system/h-system.h system/h-type.h \
	system/monster-type-definition.h \
	system/object-type-definition.h \
	system/system-variables.c system/system-variables.h \
	system/gamevalue.h \
	\
	term/gameterm.c term/gameterm.h \
	term/term-color-types.h \
	term/z-form.c term/z-form.h term/z-rand.c term/z-rand.h \
	term/z-term.c term/z-term.h term/z-util.c term/z-util.h \
	term/z-virt.c term/z-virt.h \
	\
	util/object-sort.c util/object-sort.h \
	util/util.c util/util.h \
	\
	view/display-characteristic.c view/display-characteristic.h \
	view/display-fruit.c view/display-fruit.h \
	view/display-main-window.c view/display-main-window.h \
	view/display-monster-lore.c view/display-monster-lore.h \
	view/display-monster-status.c view/display-monster-status.h \
	view/display-player-stat-info.c view/display-player-stat-info.h \
	view/display-player-misc-info.c view/display-player-misc-info.h \
	view/display-player-middle.c view/display-player-middle.h \
	view/display-player.c view/display-player.h \
	view/object-describer.c view/object-describer.h \
	view/status-first-page.c view/status-first-page.h \
	view/display-util.c view/display-util.h \
	view/display-birth.c view/display-birth.h \
	\
	wizard/wizard-spoiler.c wizard/wizard-spoiler.h\
	wizard/wizard-special-process.c wizard/wizard-special-process.h \
	\
	world/world.c world/world.h \
	world/world-object.c world/world-object.h \
	world/world-movement-processor.c world/world-movement-processor.h \
	world/world-turn-processor.c world/world-turn-processor.h

EXTRA_hengband_SOURCES = \
	angband.ico angband.rc ang_eng.rc maid-x11.c main-win.c \
	makefile.bcc makefile.std term/readdib.c term/readdib.h wall.bmp

EXTRA_DIST = \
	gcc-wrap

CFLAGS += $(XFT_CFLAGS)
LIBS += $(XFT_LIBS)
COMPILE = $(srcdir)/gcc-wrap $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	$(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) \
	-Iart-definition -Iautopick -Ibirth -Icmd -Icmd-io -Icmd-item -Icombat \
	-Icore -Idungeon -Ieffect -Ifloor -Igrid -Iinfo-reader -Iinventory -Iio \
	-Iio-dump -Iknowledge -Imarket -Imelee -Imind -Imonster -Imonster-attack \
	-Imonster-lore -Imonster-race -Imspell -Imutation -Iobject -Iobject-enchant \
	-Ipet -Iplayer -Iplayer-attack -Irealm -Iroom -Ispecific-object -Ispell \
	-Ispell-kind -Ispell-realm -Isv-definition -Isystem -Iterm -Iutil -Iview \
	-Iwizard -Iworld

install-exec-hook:
if SET_GID
	chgrp "@GAMEGROUP@" "$(DESTDIR)$(bindir)/hengband"
	chmod g+s "$(DESTDIR)$(bindir)/hengband"
endif
