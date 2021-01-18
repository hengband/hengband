## Makefile.am -- Process this file with automake to process Makefile.in

AUTOMAKE_OPTIONS = foreign subdir-objects nostdinc

bin_PROGRAMS = hengband

hengband_SOURCES = \
	action/action-limited.c action/action-limited.h \
	action/activation-execution.c action/activation-execution.h \
	action/movement-execution.c action/movement-execution.h \
	action/mutation-execution.c action/mutation-execution.h \
	action/open-util.c action/open-util.h \
	action/open-close-execution.c action/open-close-execution.h \
	action/racial-execution.c action/racial-execution.h \
	action/run-execution.c action/run-execution.h \
	action/throw-util.c action/throw-util.h \
	action/travel-execution.c action/travel-execution.h \
	action/tunnel-execution.c action/tunnel-execution.h \
	action/weapon-shield.c action/weapon-shield.h \
	\
	artifact/artifact-info.c artifact/artifact-info.h \
	artifact/fixed-art-generator.c artifact/fixed-art-generator.h \
	artifact/fixed-art-types.h \
	artifact/random-art-activation.c artifact/random-art-activation.h \
	artifact/random-art-bias-types.h \
	artifact/random-art-characteristics.c artifact/random-art-characteristics.h \
	artifact/random-art-effects.h \
	artifact/random-art-generator.c artifact/random-art-generator.h \
	artifact/random-art-misc.c artifact/random-art-misc.h \
	artifact/random-art-pval-investor.c artifact/random-art-pval-investor.h \
	artifact/random-art-resistance.c artifact/random-art-resistance.h \
	artifact/random-art-slay.c artifact/random-art-slay.h \
	\
	autopick/autopick.c autopick/autopick.h \
	autopick/autopick-commands-table.h autopick/autopick-dirty-flags.h \
	autopick/autopick-flags-table.h \
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
	blue-magic/blue-magic-ball-bolt.c blue-magic/blue-magic-ball-bolt.h \
	blue-magic/blue-magic-breath.c blue-magic/blue-magic-breath.h \
	blue-magic/blue-magic-caster.c blue-magic/blue-magic-caster.h \
	blue-magic/blue-magic-checker.c blue-magic/blue-magic-checker.h \
	blue-magic/blue-magic-spirit-curse.c blue-magic/blue-magic-spirit-curse.h \
	blue-magic/blue-magic-status.c blue-magic/blue-magic-status.h \
	blue-magic/blue-magic-summon.c blue-magic/blue-magic-summon.h \
	blue-magic/blue-magic-util.c blue-magic/blue-magic-util.h \
	blue-magic/learnt-info.c blue-magic/learnt-info.h \
	blue-magic/learnt-power-getter.c blue-magic/learnt-power-getter.h \
	\
	cmd-action/cmd-attack.c cmd-action/cmd-attack.h \
	cmd-action/cmd-hissatsu.c cmd-action/cmd-hissatsu.h \
	cmd-action/cmd-mane.c cmd-action/cmd-mane.h \
	cmd-action/cmd-mind.c cmd-action/cmd-mind.h \
	cmd-action/cmd-move.c cmd-action/cmd-move.h \
	cmd-action/cmd-open-close.c cmd-action/cmd-open-close.h \
	cmd-action/cmd-others.c cmd-action/cmd-others.h \
	cmd-action/cmd-pet.c cmd-action/cmd-pet.h \
	cmd-action/cmd-racial.c cmd-action/cmd-racial.h \
	cmd-action/cmd-shoot.c cmd-action/cmd-shoot.h \
	cmd-action/cmd-spell.c cmd-action/cmd-spell.h \
	cmd-action/cmd-travel.c cmd-action/cmd-travel.h \
	cmd-action/cmd-tunnel.c cmd-action/cmd-tunnel.h \
	\
	cmd-building/cmd-building.c cmd-building/cmd-building.h \
	cmd-building/cmd-inn.c cmd-building/cmd-inn.h \
	cmd-building/cmd-store.c cmd-building/cmd-store.h \
	\
	cmd-io/cmd-autopick.c cmd-io/cmd-autopick.h \
	cmd-io/cmd-diary.c cmd-io/cmd-diary.h \
	cmd-io/cmd-dump.c cmd-io/cmd-dump.h \
	cmd-io/cmd-floor.c cmd-io/cmd-floor.h \
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
	cmd-io/macro-util.c cmd-io/macro-util.h \
	\
	cmd-item/cmd-destroy.c cmd-item/cmd-destroy.h \
	cmd-item/cmd-eat.c cmd-item/cmd-eat.h \
	cmd-item/cmd-equipment.c cmd-item/cmd-equipment.h \
	cmd-item/cmd-item.c cmd-item/cmd-item.h \
	cmd-item/cmd-magiceat.c cmd-item/cmd-magiceat.h \
	cmd-item/cmd-quaff.c cmd-item/cmd-quaff.h \
	cmd-item/cmd-read.c cmd-item/cmd-read.h \
	cmd-item/cmd-refill.c cmd-item/cmd-refill.h \
	cmd-item/cmd-smith.c cmd-item/cmd-smith.h \
	cmd-item/cmd-throw.c cmd-item/cmd-throw.h \
	cmd-item/cmd-usestaff.c cmd-item/cmd-usestaff.h \
	cmd-item/cmd-zaprod.c cmd-item/cmd-zaprod.h \
	cmd-item/cmd-zapwand.c cmd-item/cmd-zapwand.h \
	\
	cmd-visual/cmd-draw.c cmd-visual/cmd-draw.h \
	cmd-visual/cmd-map.c cmd-visual/cmd-map.h \
	cmd-visual/cmd-visuals.c cmd-visual/cmd-visuals.h \
	\
	combat/attack-power-table.c combat/attack-power-table.h \
	combat/aura-counterattack.c combat/aura-counterattack.h \
	combat/hallucination-attacks-table.c combat/hallucination-attacks-table.h \
	combat/martial-arts-table.c combat/martial-arts-table.h \
	combat/attack-accuracy.c combat/attack-accuracy.h \
	combat/slaying.c combat/slaying.h \
	combat/combat-options-type.h \
	combat/attack-criticality.c combat/attack-criticality.h \
	combat/shoot.c combat/shoot.h \
	\
	core/asking-player.c core/asking-player.h \
	core/disturbance.c core/disturbance.h \
	core/game-closer.c core/game-closer.h \
	core/game-play.c core/game-play.h \
	core/hp-mp-processor.c core/hp-mp-processor.h \
	core/hp-mp-regenerator.c core/hp-mp-regenerator.h \
	core/magic-effects-timeout-reducer.c core/magic-effects-timeout-reducer.h \
	core/object-compressor.c core/object-compressor.h \
	core/output-updater.c core/output-updater.h \
	core/player-processor.c core/player-processor.h \
	core/player-redraw-types.h \
	core/player-update-types.h \
	core/scores.c core/scores.h \
	core/show-file.c core/show-file.h \
	core/special-internal-keys.h \
	core/speed-table.c core/speed-table.h \
	core/stuff-handler.c core/stuff-handler.h \
	core/turn-compensator.c core/turn-compensator.h \
	core/visuals-reseter.c core/visuals-reseter.h \
	core/window-redrawer.c core/window-redrawer.h \
	\
	dungeon/dungeon.c dungeon/dungeon.h \
	dungeon/dungeon-flag-types.h \
	dungeon/dungeon-processor.c dungeon/dungeon-processor.h \
	dungeon/quest.h dungeon/quest.c \
	dungeon/quest-completion-checker.c dungeon/quest-completion-checker.h \
	dungeon/quest-monster-placer.c dungeon/quest-monster-placer.h \
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
	effect/effect-processor.c effect/effect-processor.h \
	effect/effect-characteristics.h \
	\
	flavor/flag-inscriptions-table.c flavor/flag-inscriptions-table.h \
	flavor/flavor-describer.c flavor/flavor-describer.h \
	flavor/flavor-util.c flavor/flavor-util.h \
	flavor/named-item-describer.c flavor/named-item-describer.h \
	flavor/object-flavor-types.h \
	flavor/object-flavor.c flavor/object-flavor.h \
	flavor/tval-description-switcher.c flavor/tval-description-switcher.h \
	\
	floor/cave.c floor/cave.h \
	floor/cave-generator.c floor/cave-generator.h \
	floor/dungeon-tunnel-util.c floor/dungeon-tunnel-util.h \
	floor/fixed-map-generator.c floor/fixed-map-generator.h \
	floor/floor-allocation-types.h \
	floor/floor-base-definitions.h \
	floor/floor-changer.c floor/floor-changer.h \
	floor/floor-events.c floor/floor-events.h \
	floor/floor-generator-util.h \
	floor/floor-generator.c floor/floor-generator.h \
	floor/floor-leaver.c floor/floor-leaver.h \
	floor/floor-mode-changer.c floor/floor-mode-changer.h \
	floor/floor-object.c floor/floor-object.h \
	floor/floor-save.c floor/floor-save.h \
	floor/floor-save-util.c floor/floor-save-util.h \
	floor/floor-streams.c floor/floor-streams.h \
	floor/floor-town.h floor/floor-town.c \
	floor/floor-util.c floor/floor-util.h \
	floor/geometry.c floor/geometry.h \
	floor/line-of-sight.c floor/line-of-sight.h \
	floor/object-allocator.c floor/object-allocator.h \
	floor/object-scanner.c floor/object-scanner.h \
	floor/pattern-walk.c floor/pattern-walk.h \
	floor/sight-definitions.h \
	floor/tunnel-generator.c floor/tunnel-generator.h \
	floor/wild.h floor/wild.c \
	\
	game-option/auto-destruction-options.c game-option/auto-destruction-options.h \
	game-option/birth-options.c game-option/birth-options.h \
	game-option/cheat-options.c game-option/cheat-options.h \
	game-option/cheat-types.h \
	game-option/disturbance-options.c game-option/disturbance-options.h \
	game-option/game-play-options.c game-option/game-play-options.h \
	game-option/input-options.c game-option/input-options.h \
	game-option/keymap-directory-getter.c game-option/keymap-directory-getter.h \
	game-option/map-screen-options.c game-option/map-screen-options.h \
	game-option/option-flags.c game-option/option-flags.h \
	game-option/option-types-table.c game-option/option-types-table.h \
	game-option/play-record-options.c game-option/play-record-options.h \
	game-option/runtime-arguments.c game-option/runtime-arguments.h \
	game-option/special-options.c game-option/special-options.h \
	game-option/text-display-options.c game-option/text-display-options.h \
	\
	grid/door.c grid/door.h \
	grid/feature-flag-types.h \
	grid/feature-generator.c grid/feature-generator.h \
	grid/feature.c grid/feature.h \
	grid/grid.c grid/grid.h \
	grid/lighting-colors-table.c grid/lighting-colors-table.h \
	grid/object-placer.c grid/object-placer.h \
	grid/stair.c grid/stair.h \
	grid/trap.c grid/trap.h \
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
	inventory/floor-item-getter.c inventory/floor-item-getter.h \
	inventory/inventory-curse.c inventory/inventory-curse.h \
	inventory/inventory-damage.c inventory/inventory-damage.h \
	inventory/inventory-describer.c inventory/inventory-describer.h \
	inventory/inventory-object.c inventory/inventory-object.h \
	inventory/inventory-slot-types.h \
	inventory/inventory-util.c inventory/inventory-util.h \
	inventory/item-getter.c inventory/item-getter.h \
	inventory/item-selection-util.c inventory/item-selection-util.h \
	inventory/pack-overflow.c inventory/pack-overflow.h \
	inventory/player-inventory.c inventory/player-inventory.h \
	inventory/recharge-processor.c inventory/recharge-processor.h \
	\
	io/command-repeater.c io/command-repeater.h \
	io/cursor.c io/cursor.h \
	io/exit-panic.c io/exit-panic.h \
	io/files-util.c io/files-util.h \
	io/gf-descriptions.c io/gf-descriptions.h \
	io/inet.c io/inet.h \
	io/input-key-acceptor.c io/input-key-acceptor.h \
	io/input-key-processor.c io/input-key-processor.h \
	io/input-key-requester.c io/input-key-requester.h \
	io/interpret-pref-file.c io/interpret-pref-file.h \
	io/mutations-dump.c io/mutations-dump.h \
	io/pref-file-expressor.c io/pref-file-expressor.h \
	io/read-pref-file.c io/read-pref-file.h \
	io/record-play-movie.c io/record-play-movie.h \
	io/report.c io/report.h \
	io/screen-util.c io/screen-util.h \
	io/signal-handlers.c io/signal-handlers.h \
	io/store-key-processor.c io/store-key-processor.h \
	io/tokenizer.c io/tokenizer.h \
	io/uid-checker.c io/uid-checker.h \
	io/write-diary.c io/write-diary.h \
	\
	io-dump/character-dump.c io-dump/character-dump.h \
	io-dump/dump-remover.c io-dump/dump-remover.h \
	io-dump/dump-util.c io-dump/dump-util.h \
	io-dump/player-status-dump.c io-dump/player-status-dump.h \
	io-dump/random-art-info-dumper.c io-dump/random-art-info-dumper.h \
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
	load/angband-version-comparer.c load/angband-version-comparer.h \
	load/birth-loader.c load/birth-loader.h \
	load/dummy-loader.c load/dummy-loader.h \
	load/dungeon-loader.c load/dungeon-loader.h \
	load/extra-loader.c load/extra-loader.h \
	load/floor-loader.c load/floor-loader.h \
	load/info-loader.c load/info-loader.h \
	load/inventory-loader.c load/inventory-loader.h \
	load/item-loader.c load/item-loader.h \
	load/monster-loader.c load/monster-loader.h \
	load/old-feature-types.h \
	load/load-util.c load/load-util.h \
	load/load-v1-3-0.c load/load-v1-3-0.h \
	load/load-v1-5-0.c load/load-v1-5-0.h \
	load/load-v1-7-0.c load/load-v1-7-0.h \
	load/load-zangband.c load/load-zangband.h \
	load/quest-loader.c load/quest-loader.h \
	load/load.c load/load.h \
	load/lore-loader.c load/lore-loader.h \
	load/option-loader.c load/option-loader.h \
	load/player-attack-loader.c load/player-attack-loader.h \
	load/player-info-loader.c load/player-info-loader.h \
	load/savedata-flag-types.h \
	load/store-loader.c load/store-loader.h \
	load/world-loader.c load/world-loader.h \
	\
	locale/english.c locale/english.h \
	locale/japanese.c locale/japanese.h \
	locale/language-switcher.h \
	\
	lore/combat-types-setter.c lore/combat-types-setter.h \
	lore/lore-calculator.c lore/lore-calculator.h \
	lore/lore-store.c lore/lore-store.h \
	lore/lore-util.c lore/lore-util.h \
	lore/magic-types-setter.c lore/magic-types-setter.h \
	lore/monster-lore.c lore/monster-lore.h \
	\
	main.c main-x11.c main-gcu.c \
	\
	main/angband-headers.c main/angband-headers.h \
	main/angband-initializer.c main/angband-initializer.h \
	main/game-data-initializer.c main/game-data-initializer.h \
	main/info-initializer.c main/info-initializer.h \
	main/init-error-messages-table.c main/init-error-messages-table.h \
	main/music-definitions-table.c main/music-definitions-table.h \
	main/sound-definitions-table.c main/sound-definitions-table.h \
	main/sound-of-music.c main/sound-of-music.h \
	main/x11-gamma-builder.c main/x11-gamma-builder.h \
	main/x11-type-string.c main/x11-type-string.h \
	\
	market/arena-info-table.c market/arena-info-table.h \
	market/building-initializer.c market/building-initializer.h \
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
	melee/melee-spell.c melee/melee-spell.h \
	melee/melee-spell-flags-checker.c melee/melee-spell-flags-checker.h \
	melee/melee-spell-util.c melee/melee-spell-util.h \
	melee/melee-switcher.c melee/melee-switcher.h \
	melee/melee-util.c melee/melee-util.h \
	\
	mind/drs-types.h \
	mind/mind-archer.c mind/mind-archer.h \
	mind/mind-berserker.c mind/mind-berserker.h \
	mind/mind-blue-mage.c mind/mind-blue-mage.h \
	mind/mind-cavalry.c mind/mind-cavalry.h \
	mind/mind-chaos-warrior.c mind/mind-chaos-warrior.h \
	mind/mind-explanations-table.c mind/mind-explanations-table.h \
	mind/mind-force-trainer.c mind/mind-force-trainer.h \
	mind/mind-info.c mind/mind-info.h \
	mind/mind-hobbit.c mind/mind-hobbit.h \
	mind/mind-mage.c mind/mind-mage.h \
	mind/mind-magic-eater.c mind/mind-magic-eater.h \
	mind/mind-magic-resistance.c mind/mind-magic-resistance.h \
	mind/mind-mindcrafter.c mind/mind-mindcrafter.h \
	mind/mind-mirror-master.c mind/mind-mirror-master.h \
	mind/mind-monk.c mind/mind-monk.h \
	mind/mind-ninja.c mind/mind-ninja.h \
	mind/mind-numbers.h \
	mind/mind-power-getter.c mind/mind-power-getter.h \
	mind/mind-priest.c mind/mind-priest.h \
	mind/mind-samurai.c mind/mind-samurai.h \
	mind/mind-sniper.c mind/mind-sniper.h \
	mind/mind-types.h \
	mind/mind-warrior.c mind/mind-warrior.h \
	mind/mind-warrior-mage.c mind/mind-warrior-mage.h \
	mind/mind-weaponsmith.c mind/mind-weaponsmith.h \
	mind/monk-attack.c mind/monk-attack.h \
	mind/snipe-types.h \
	mind/stances-table.c mind/stances-table.h \
	\
	monster/horror-descriptions.c monster/horror-descriptions.h \
	monster/monster-compaction.c monster/monster-compaction.h \
	monster/monster-describer.c monster/monster-describer.h \
	monster/monster-description-types.h \
	monster/monster-flag-types.h \
	monster/monster-info.c monster/monster-info.h \
	monster/monster-list.c monster/monster-list.h \
	monster/monster-processor.c monster/monster-processor.h \
	monster/monster-processor-util.c monster/monster-processor-util.h \
	monster/monster-timed-effect-types.h \
	monster/smart-learn-types.h \
	monster/monster-status.c monster/monster-status.h \
	monster/monster-status-setter.c monster/monster-status-setter.h \
	monster/monster-update.c monster/monster-update.h \
	monster/monster-util.c monster/monster-util.h \
	\
	monster-attack/insults-moans.c monster-attack/insults-moans.h \
	monster-attack/monster-attack-describer.c monster-attack/monster-attack-describer.h \
	monster-attack/monster-attack-effect.h \
	monster-attack/monster-attack-lose.c monster-attack/monster-attack-lose.h \
	monster-attack/monster-attack-player.c monster-attack/monster-attack-player.h \
	monster-attack/monster-attack-processor.c monster-attack/monster-attack-processor.h \
	monster-attack/monster-attack-status.c monster-attack/monster-attack-status.h \
	monster-attack/monster-attack-switcher.c monster-attack/monster-attack-switcher.h \
	monster-attack/monster-attack-types.c monster-attack/monster-attack-types.h \
	monster-attack/monster-attack-util.c monster-attack/monster-attack-util.h \
	monster-attack/monster-eating.c monster-attack/monster-eating.h \
	\
	monster-floor/monster-death.c monster-floor/monster-death.h \
	monster-floor/monster-death-util.c monster-floor/monster-death-util.h \
	monster-floor/monster-direction.c monster-floor/monster-direction.h \
	monster-floor/monster-dist-offsets.c monster-floor/monster-dist-offsets.h \
	monster-floor/monster-generator.c monster-floor/monster-generator.h \
	monster-floor/monster-move.c monster-floor/monster-move.h \
	monster-floor/monster-object.c monster-floor/monster-object.h \
	monster-floor/monster-remover.c monster-floor/monster-remover.h \
	monster-floor/monster-runaway.c monster-floor/monster-runaway.h \
	monster-floor/monster-safety-hiding.c monster-floor/monster-safety-hiding.h \
	monster-floor/monster-summon.c monster-floor/monster-summon.h \
	monster-floor/monster-sweep-grid.c monster-floor/monster-sweep-grid.h \
	monster-floor/monster-lite.c monster-floor/monster-lite.h \
	monster-floor/monster-lite-util.c monster-floor/monster-lite-util.h \
	monster-floor/one-monster-placer.c monster-floor/one-monster-placer.h \
	monster-floor/place-monster-types.h \
	monster-floor/quantum-effect.c monster-floor/quantum-effect.h \
	monster-floor/special-death-switcher.c monster-floor/special-death-switcher.h \
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
	mspell/assign-monster-spell.c mspell/assign-monster-spell.h \
	mspell/element-resistance-checker.c mspell/element-resistance-checker.h \
	mspell/high-resistance-checker.c mspell/high-resistance-checker.h \
	mspell/improper-mspell-remover.c mspell/improper-mspell-remover.h \
	mspell/monster-power-table.c mspell/monster-power-table.h \
	mspell/mspell-attack.c mspell/mspell-attack.h \
	mspell/mspell-attack-util.c mspell/mspell-attack-util.h \
	mspell/mspell-breath.c mspell/mspell-breath.h \
	mspell/mspell-ball.c mspell/mspell-ball.h \
	mspell/mspell-bolt.c mspell/mspell-bolt.h \
	mspell/mspell-checker.c mspell/mspell-checker.h \
	mspell/mspell-curse.c mspell/mspell-curse.h \
	mspell/mspell-dispel.c mspell/mspell-dispel.h \
	mspell/mspell-judgement.c mspell/mspell-judgement.h \
	mspell/mspell-lite.c mspell/mspell-lite.h \
	mspell/mspell-summon.c mspell/mspell-summon.h \
	mspell/mspell-util.c mspell/mspell-util.h \
	mspell/mspell-type.h \
	mspell/mspell-damage-calculator.c mspell/mspell-damage-calculator.h \
	mspell/mspell-learn-checker.c mspell/mspell-learn-checker.h \
	mspell/mspell-floor.c mspell/mspell-floor.h \
	mspell/mspell-mask-definitions.h \
	mspell/mspell-special.c mspell/mspell-special.h \
	mspell/mspell-status.c mspell/mspell-status.h \
	mspell/mspell-particularity.c mspell/mspell-particularity.h \
	mspell/mspell-selector.c mspell/mspell-selector.h \
	mspell/smart-mspell-util.c mspell/smart-mspell-util.h \
	mspell/specified-summon.c mspell/specified-summon.h \
	mspell/summon-checker.c mspell/summon-checker.h \
	\
	mutation/gain-mutation-switcher.c mutation/gain-mutation-switcher.h \
	mutation/lose-mutation-switcher.c mutation/lose-mutation-switcher.h \
	mutation/mutation-calculator.c mutation/mutation-calculator.h \
	mutation/mutation-investor-remover.c mutation/mutation-investor-remover.h \
	mutation/mutation-util.c mutation/mutation-util.h \
	mutation/mutation-flag-types.h \
	mutation/mutation-processor.c mutation/mutation-processor.h \
	mutation/mutation-techniques.c mutation/mutation-techniques.h \
	\
	object/item-tester-hooker.c object/item-tester-hooker.h \
	object/object-broken.c object/object-broken.h \
	object/object-flags.c object/object-flags.h \
	object/object-generator.c object/object-generator.h \
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
	object-activation/activation-bolt-ball.c object-activation/activation-bolt-ball.h \
	object-activation/activation-breath.c object-activation/activation-breath.h \
	object-activation/activation-charm.c object-activation/activation-charm.h \
	object-activation/activation-genocide.c object-activation/activation-genocide.h \
	object-activation/activation-others.c object-activation/activation-others.h \
	object-activation/activation-resistance.c object-activation/activation-resistance.h \
	object-activation/activation-switcher.c object-activation/activation-switcher.h \
	object-activation/activation-teleport.c object-activation/activation-teleport.h \
	object-activation/activation-util.c object-activation/activation-util.h \
	\
	object-enchant/activation-info-table.c object-enchant/activation-info-table.h \
	object-enchant/apply-magic.c object-enchant/apply-magic.h \
	object-enchant/apply-magic-accessory.c object-enchant/apply-magic-accessory.h \
	object-enchant/apply-magic-armor.c object-enchant/apply-magic-armor.h \
	object-enchant/apply-magic-others.c object-enchant/apply-magic-others.h \
	object-enchant/apply-magic-weapon.c object-enchant/apply-magic-weapon.h \
	object-enchant/dragon-breaths-table.c object-enchant/dragon-breaths-table.h \
	object-enchant/object-boost.c object-enchant/object-boost.h \
	object-enchant/object-curse.c object-enchant/object-curse.h \
	object-enchant/object-ego.c object-enchant/object-ego.h \
	object-enchant/item-apply-magic.h object-enchant/item-feeling.h \
	object-enchant/old-ego-extra-values.h object-enchant/special-object-flags.h \
	object-enchant/tr-types.h object-enchant/trc-types.h \
	object-enchant/trg-types.h \
	object-enchant/vorpal-weapon.c object-enchant/vorpal-weapon.h \
	\
	object-hook/hook-armor.c object-hook/hook-armor.h \
	object-hook/hook-bow.c object-hook/hook-bow.h \
	object-hook/hook-checker.c object-hook/hook-checker.h \
	object-hook/hook-enchant.c object-hook/hook-enchant.h \
	object-hook/hook-expendable.c object-hook/hook-expendable.h \
	object-hook/hook-magic.c object-hook/hook-magic.h \
	object-hook/hook-perception.c object-hook/hook-perception.h \
	object-hook/hook-quest.c object-hook/hook-quest.h \
	object-hook/hook-weapon.c object-hook/hook-weapon.h \
	\
	object-use/quaff-execution.c object-use/quaff-execution.h \
	object-use/read-execution.c object-use/read-execution.h \
	\
	perception/identification.c perception/identification.h \
	perception/object-perception.c perception/object-perception.h \
	perception/simple-perception.c perception/simple-perception.h \
	\
	pet/pet-fall-off.c pet/pet-fall-off.h \
	pet/pet-util.c pet/pet-util.h \
	\
	player/attack-defense-types.h \
	player/eldritch-horror.c player/eldritch-horror.h \
	player/mimic-info-table.c player/mimic-info-table.h \
	player/patron.c player/patron.h \
	player/process-death.c player/process-death.h \
	player/process-name.c player/process-name.h \
	player/race-info-table.c player/race-info-table.h\
	player/race-resistances.c player/race-resistances.h \
	player/permanent-resistances.c player/permanent-resistances.h \
	player/temporary-resistances.c player/temporary-resistances.h \
	player/digestion-processor.c player/digestion-processor.h \
	player/player-classes-types.h \
	player/player-class.c player/player-class.h \
	player/player-damage.c player/player-damage.h \
	player/player-move.c player/player-move.h \
	player/player-personality.c player/player-personality.h \
	player/player-race.c player/player-race.h \
	player/player-race-types.h \
	player/player-realm.c player/player-realm.h \
	player/player-skill.c player/player-skill.h \
	player/player-status.c player/player-status.h \
	player/player-status-flags.c player/player-status-flags.h \
	player/player-status-table.c player/player-status-table.h \
	player/player-status-resist.c player/player-status-resist.h \
	player/player-personalities-types.h \
	player/player-sex.c player/player-sex.h \
	player/player-view.c player/player-view.h \
	player/special-defense-types.h \
	\
	player-attack/player-attack-util.h \
	player-attack/attack-chaos-effect.c player-attack/attack-chaos-effect.h \
	player-attack/blood-sucking-processor.c player-attack/blood-sucking-processor.h \
	player-attack/player-attack.c player-attack/player-attack.h \
	\
	player-info/avatar.h player-info/avatar.c \
	player-info/base-status-info.c player-info/base-status-info.h \
	player-info/base-status-types.h \
	player-info/body-improvement-info.c player-info/body-improvement-info.h \
	player-info/class-ability-info.c player-info/class-ability-info.h \
	player-info/mutation-info.c player-info/mutation-info.h \
	player-info/race-ability-info.c player-info/race-ability-info.h \
	player-info/resistance-info.c player-info/resistance-info.h \
	player-info/self-info.c player-info/self-info.h \
	player-info/self-info-util.c player-info/self-info-util.h \
	player-info/weapon-effect-info.c player-info/weapon-effect-info.h \
	\
	racial/class-racial-switcher.c racial/class-racial-switcher.h \
	racial/mutation-racial-selector.c racial/mutation-racial-selector.h \
	racial/race-racial-command-setter.c racial/race-racial-command-setter.h \
	racial/racial-android.c racial/racial-android.h \
	racial/racial-balrog.c racial/racial-balrog.h \
	racial/racial-draconian.c racial/racial-draconian.h \
	racial/racial-kutar.c racial/racial-kutar.h \
	racial/racial-switcher.c racial/racial-switcher.h \
	racial/racial-util.c racial/racial-util.h \
	racial/racial-vampire.c racial/racial-vampire.h \
	\
	realm/realm-arcane.c realm/realm-arcane.h \
	realm/realm-chaos.c realm/realm-chaos.h \
	realm/realm-craft.c realm/realm-craft.h \
	realm/realm-crusade.c realm/realm-crusade.h \
	realm/realm-demon.c realm/realm-demon.h \
	realm/realm-death.c realm/realm-death.h \
	realm/realm-hex.c realm/realm-hex.h \
	realm/realm-hex-numbers.h \
	realm/realm-hissatsu.c realm/realm-hissatsu.h \
	realm/realm-life.c realm/realm-life.h \
	realm/realm-names-table.c realm/realm-names-table.h \
	realm/realm-nature.c realm/realm-nature.h \
	realm/realm-song.c realm/realm-song.h \
	realm/realm-song-numbers.h \
	realm/realm-sorcery.c realm/realm-sorcery.h \
	realm/realm-trump.c realm/realm-trump.h \
	realm/realm-types.h \
	\
	room/cave-filler.c room/cave-filler.h \
	room/door-definition.c room/door-definition.h \
	room/lake-types.h room/room-types.h \
	room/room-generator.c room/room-generator.h \
	room/room-info-table.c room/room-info-table.h \
	room/rooms-builder.c room/rooms-builder.h \
	room/rooms-city.c room/rooms-city.h \
	room/rooms-fractal.c room/rooms-fractal.h \
	room/rooms-maze-vault.c room/rooms-maze-vault.h \
	room/rooms-normal.c room/rooms-normal.h \
	room/rooms-pit-nest.c room/rooms-pit-nest.h \
	room/rooms-special.c room/rooms-special.h \
	room/rooms-trap.c room/rooms-trap.h \
	room/rooms-vault.c room/rooms-vault.h \
	room/space-finder.c room/space-finder.h \
	room/pit-nest-kinds-table.c room/pit-nest-kinds-table.h \
	room/treasure-deployment.c room/treasure-deployment.h \
	room/vault-builder.c room/vault-builder.h \
	\
	save/floor-writer.c save/floor-writer.h \
	save/info-writer.c save/info-writer.h \
	save/item-writer.c save/item-writer.h \
	save/monster-writer.c save/monster-writer.h \
	save/player-writer.c save/player-writer.h \
	save/save.c save/save.h \
	save/save-util.c save/save-util.h \
	\
	specific-object/blade-turner.c specific-object/blade-turner.h \
	specific-object/bloody-moon.c specific-object/bloody-moon.h \
	specific-object/bow.c specific-object/bow.h \
	specific-object/chest.c specific-object/chest.h \
	specific-object/death-crimson.c specific-object/death-crimson.h \
	specific-object/death-scythe.c specific-object/death-scythe.h \
	specific-object/monster-ball.c specific-object/monster-ball.h \
	specific-object/muramasa.c specific-object/muramasa.h \
	specific-object/ring-of-power.c specific-object/ring-of-power.h \
	specific-object/toragoroshi.c specific-object/toragoroshi.h \
	specific-object/torch.c specific-object/torch.h \
	\
	spell/range-calc.c spell/range-calc.h \
	spell/spells-describer.c spell/spells-describer.h \
	spell/spells-diceroll.c spell/spells-diceroll.h \
	spell/spells-execution.c spell/spells-execution.h \
	spell/spell-info.c spell/spell-info.h \
	spell/spells-object.c spell/spells-object.h \
	spell/spells-staff-only.c spell/spells-staff-only.h \
	spell/spells-summon.c spell/spells-summon.h \
	spell/spells-status.c spell/spells-status.h \
	spell/spell-types.h spell/spells-util.h \
	spell/summon-types.h \
	spell/technic-info-table.c spell/technic-info-table.h \
	\
	spell-kind/blood-curse.c spell-kind/blood-curse.h \
	spell-kind/earthquake.c spell-kind/earthquake.h \
	spell-kind/magic-item-recharger.c spell-kind/magic-item-recharger.h \
	spell-kind/spells-beam.c spell-kind/spells-beam.h \
	spell-kind/spells-charm.c spell-kind/spells-charm.h \
	spell-kind/spells-curse-removal.c spell-kind/spells-curse-removal.h \
	spell-kind/spells-detection.c spell-kind/spells-detection.h \
	spell-kind/spells-enchant.c spell-kind/spells-enchant.h \
	spell-kind/spells-equipment.c spell-kind/spells-equipment.h \
	spell-kind/spells-fetcher.c spell-kind/spells-fetcher.h \
	spell-kind/spells-floor.c spell-kind/spells-floor.h \
	spell-kind/spells-genocide.c spell-kind/spells-genocide.h \
	spell-kind/spells-grid.c spell-kind/spells-grid.h \
	spell-kind/spells-launcher.c spell-kind/spells-launcher.h \
	spell-kind/spells-lite.c spell-kind/spells-lite.h \
	spell-kind/spells-neighbor.c spell-kind/spells-neighbor.h \
	spell-kind/spells-pet.c spell-kind/spells-pet.h \
	spell-kind/spells-perception.c spell-kind/spells-perception.h \
	spell-kind/spells-polymorph.c spell-kind/spells-polymorph.h \
	spell-kind/spells-random.c spell-kind/spells-random.h \
	spell-kind/spells-sight.c spell-kind/spells-sight.h \
	spell-kind/spells-specific-bolt.c spell-kind/spells-specific-bolt.h \
	spell-kind/spells-teleport.c spell-kind/spells-teleport.h \
	spell-kind/spells-world.c spell-kind/spells-world.h \
	\
	spell-realm/spells-arcane.c spell-realm/spells-arcane.h \
	spell-realm/spells-chaos.c spell-realm/spells-chaos.h \
	spell-realm/spells-craft.c spell-realm/spells-craft.h \
	spell-realm/spells-crusade.c spell-realm/spells-crusade.h \
	spell-realm/spells-demon.c spell-realm/spells-demon.h \
	spell-realm/spells-hex.c spell-realm/spells-hex.h \
	spell-realm/spells-nature.c spell-realm/spells-nature.h \
	spell-realm/spells-song.c spell-realm/spells-song.h \
	spell-realm/spells-sorcery.c spell-realm/spells-sorcery.h \
	spell-realm/spells-trump.c spell-realm/spells-trump.h \
	\
	status/action-setter.c status/action-setter.h \
	status/bad-status-setter.c status/bad-status-setter.h \
	status/base-status.c status/base-status.h \
	status/body-improvement.c status/body-improvement.h \
	status/buff-setter.c status/buff-setter.h \
	status/element-resistance.c status/element-resistance.h \
	status/experience.c status/experience.h \
	status/shape-changer.c status/shape-changer.h \
	status/sight-setter.c status/sight-setter.h \
	status/temporary-resistance.c status/temporary-resistance.h \
	\
	store/home.c store/home.h \
	store/store.c store/store.h \
	store/black-market.c store/black-market.h \
	store/museum.c store/museum.h \
	store/pricing.c store/pricing.h \
	store/purchase-order.c store/purchase-order.h \
	store/owner-insults.c store/owner-insults.h \
	store/rumor.c store/rumor.h \
	store/say-comments.c store/say-comments.h \
	store/sell-order.c store/sell-order.h \
	store/service-checker.c store/service-checker.h \
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
	system/alloc-entries.c system/alloc-entries.h \
	system/alloc-entry-definition.h \
	system/angband.h system/angband-version.h \
	system/artifact-type-definition.c system/artifact-type-definition.h \
	system/building-type-definition.c system/building-type-definition.h \
	system/dungeon-data-definition.h \
	system/floor-type-definition.h \
	system/game-option-types.h \
	system/h-basic.h system/h-config.h \
	system/h-define.h system/h-system.h system/h-type.h \
	system/monster-race-definition.h \
	system/monster-type-definition.h \
	system/object-type-definition.h \
	system/system-variables.c system/system-variables.h \
	system/gamevalue.h \
	\
	target/grid-selector.c target/grid-selector.h \
	target/projection-path-calculator.c target/projection-path-calculator.h \
	target/target-checker.c target/target-checker.h \
	target/target-describer.c target/target-describer.h \
	target/target-getter.c target/target-getter.h \
	target/target-preparation.c target/target-preparation.h \
	target/target-setter.c target/target-setter.h \
	target/target-types.h \
	\
	term/gameterm.c term/gameterm.h \
	term/screen-processor.c term/screen-processor.h \
	term/term-color-types.h \
	term/z-form.c term/z-form.h term/z-rand.c term/z-rand.h \
	term/z-term.c term/z-term.h term/z-util.c term/z-util.h \
	term/z-virt.c term/z-virt.h \
	\
	util/angband-files.c util/angband-files.h \
	util/buffer-shaper.c util/buffer-shaper.h \
	util/bit-flags-calculator.h \
	util/int-char-converter.h \
	util/object-sort.c util/object-sort.h \
	util/quarks.c util/quarks.h \
	util/sort.c util/sort.h \
	util/string-processor.c util/string-processor.h \
	util/tag-sorter.c util/tag-sorter.h \
	\
	view/display-birth.c view/display-birth.h \
	view/display-characteristic.c view/display-characteristic.h \
	view/display-fruit.c view/display-fruit.h \
	view/display-inventory.c view/display-inventory.h\
	view/display-lore-attacks.c view/display-lore-attacks.h \
	view/display-lore-drops.c view/display-lore-drops.h \
	view/display-lore-magics.c view/display-lore-magics.h \
	view/display-lore-status.c view/display-lore-status.h \
	view/display-lore.c view/display-lore.h \
	view/display-map.c view/display-map.h \
	view/display-messages.c view/display-messages.h \
	view/display-monster-status.c view/display-monster-status.h \
	view/display-player-stat-info.c view/display-player-stat-info.h \
	view/display-player-misc-info.c view/display-player-misc-info.h \
	view/display-player-middle.c view/display-player-middle.h \
	view/display-player.c view/display-player.h \
	view/display-self-info.c view/display-self-info.h \
	view/display-store.c view/display-store.h \
	view/display-util.c view/display-util.h \
	view/object-describer.c view/object-describer.h \
	view/status-first-page.c view/status-first-page.h \
	view/status-bars-table.c view/status-bars-table.h \
	\
	window/display-sub-window-items.c window/display-sub-window-items.h \
	window/display-sub-window-spells.c window/display-sub-window-spells.h \
	window/display-sub-windows.c window/display-sub-windows.h \
	window/main-window-left-frame.c window/main-window-left-frame.h \
	window/main-window-row-column.h \
	window/main-window-stat-poster.c window/main-window-stat-poster.h \
	window/main-window-util.c window/main-window-util.h \
	window/main-window-equipments.c window/main-window-equipments.h \
	\
	wizard/artifact-analyzer.c wizard/artifact-analyzer.h \
	wizard/artifact-bias-table.c wizard/artifact-bias-table.h \
	wizard/cmd-wizard.c wizard/cmd-wizard.h \
	wizard/fixed-artifacts-spoiler.c wizard/fixed-artifacts-spoiler.h \
	wizard/items-spoiler.c wizard/items-spoiler.h \
	wizard/monster-info-spoiler.c wizard/monster-info-spoiler.h \
	wizard/spoiler-table.c wizard/spoiler-table.h \
	wizard/spoiler-util.c wizard/spoiler-util.h \
	wizard/tval-descriptions-table.c wizard/tval-descriptions-table.h \
	wizard/wizard-item-modifier.c wizard/wizard-item-modifier.h \
	wizard/wizard-messages.c wizard/wizard-messages.h \
	wizard/wizard-spells.c wizard/wizard-spells.h \
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

DEFAULT_INCLUDES = -I$(srcdir) -I$(top_builddir)/src 
CFLAGS += $(XFT_CFLAGS)
LIBS += $(XFT_LIBS)
COMPILE = $(srcdir)/gcc-wrap $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	$(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) 

install-exec-hook:
if SET_GID
	chgrp "@GAMEGROUP@" "$(DESTDIR)$(bindir)/hengband"
	chmod g+s "$(DESTDIR)$(bindir)/hengband"
endif
