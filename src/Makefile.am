## Makefile.am -- Process this file with automake to process Makefile.in

AUTOMAKE_OPTIONS = foreign subdir-objects nostdinc

bin_PROGRAMS = hengband

hengband_SOURCES = \
	action/action-limited.cpp action/action-limited.h \
	action/activation-execution.cpp action/activation-execution.h \
	action/movement-execution.cpp action/movement-execution.h \
	action/mutation-execution.cpp action/mutation-execution.h \
	action/open-util.cpp action/open-util.h \
	action/open-close-execution.cpp action/open-close-execution.h \
	action/racial-execution.cpp action/racial-execution.h \
	action/run-execution.cpp action/run-execution.h \
	action/travel-execution.cpp action/travel-execution.h \
	action/tunnel-execution.cpp action/tunnel-execution.h \
	action/weapon-shield.cpp action/weapon-shield.h \
	\
	artifact/fixed-art-generator.cpp artifact/fixed-art-generator.h \
	artifact/fixed-art-types.h \
	artifact/random-art-activation.cpp artifact/random-art-activation.h \
	artifact/random-art-bias-types.h \
	artifact/random-art-characteristics.cpp artifact/random-art-characteristics.h \
	artifact/random-art-effects.h \
	artifact/random-art-generator.cpp artifact/random-art-generator.h \
	artifact/random-art-misc.cpp artifact/random-art-misc.h \
	artifact/random-art-pval-investor.cpp artifact/random-art-pval-investor.h \
	artifact/random-art-resistance.cpp artifact/random-art-resistance.h \
	artifact/random-art-slay.cpp artifact/random-art-slay.h \
	\
	autopick/autopick.cpp autopick/autopick.h \
	autopick/autopick-commands-table.h autopick/autopick-dirty-flags.h \
	autopick/autopick-flags-table.h \
	autopick/autopick-key-flag-process.h autopick/autopick-keys-table.h \
	autopick/autopick-menu-data-table.cpp autopick/autopick-menu-data-table.h \
	autopick/autopick-methods-table.h \
	autopick/autopick-util.cpp autopick/autopick-util.h \
	autopick/autopick-entry.cpp autopick/autopick-entry.h \
	autopick/autopick-initializer.cpp autopick/autopick-initializer.h \
	autopick/autopick-matcher.cpp autopick/autopick-matcher.h \
	autopick/autopick-describer.cpp autopick/autopick-describer.h \
	autopick/autopick-destroyer.cpp autopick/autopick-destroyer.h \
	autopick/autopick-reader-writer.cpp autopick/autopick-reader-writer.h \
	autopick/autopick-finder.cpp autopick/autopick-finder.h \
	autopick/autopick-pref-processor.cpp autopick/autopick-pref-processor.h \
	autopick/autopick-drawer.cpp autopick/autopick-drawer.h \
	autopick/autopick-inserter-killer.cpp autopick/autopick-inserter-killer.h \
	autopick/autopick-registry.cpp autopick/autopick-registry.h \
	autopick/autopick-command-menu.cpp autopick/autopick-command-menu.h \
	autopick/autopick-editor-util.cpp autopick/autopick-editor-util.h \
	autopick/autopick-editor-command.cpp autopick/autopick-editor-command.h \
	\
	avatar/avatar.h avatar/avatar.cpp \
	avatar/avatar-changer.cpp avatar/avatar-changer.h \
	\
	birth/birth-explanations-table.cpp birth/birth-explanations-table.h \
	birth/character-builder.cpp birth/character-builder.h \
	birth/history.cpp birth/history.h \
	birth/birth-util.cpp birth/birth-util.h \
	birth/birth-select-realm.cpp birth/birth-select-realm.h \
	birth/quick-start.cpp birth/quick-start.h \
	birth/birth-stat.cpp birth/birth-stat.h \
	birth/history-generator.cpp birth/history-generator.h \
	birth/birth-body-spec.cpp birth/birth-body-spec.h \
	birth/initial-equipments-table.cpp birth/initial-equipments-table.h \
	birth/inventory-initializer.cpp birth/inventory-initializer.h \
	birth/game-play-initializer.cpp birth/game-play-initializer.h \
	birth/history-editor.cpp birth/history-editor.h \
	birth/birth-select-race.cpp birth/birth-select-race.h \
	birth/birth-select-class.cpp birth/birth-select-class.h \
	birth/birth-select-personality.cpp birth/birth-select-personality.h \
	birth/auto-roller.cpp birth/auto-roller.h \
	birth/birth-wizard.cpp birth/birth-wizard.h \
	\
	blue-magic/blue-magic-ball-bolt.cpp blue-magic/blue-magic-ball-bolt.h \
	blue-magic/blue-magic-breath.cpp blue-magic/blue-magic-breath.h \
	blue-magic/blue-magic-caster.cpp blue-magic/blue-magic-caster.h \
	blue-magic/blue-magic-checker.cpp blue-magic/blue-magic-checker.h \
	blue-magic/blue-magic-spirit-curse.cpp blue-magic/blue-magic-spirit-curse.h \
	blue-magic/blue-magic-status.cpp blue-magic/blue-magic-status.h \
	blue-magic/blue-magic-summon.cpp blue-magic/blue-magic-summon.h \
	blue-magic/blue-magic-util.cpp blue-magic/blue-magic-util.h \
	blue-magic/learnt-info.cpp blue-magic/learnt-info.h \
	blue-magic/learnt-power-getter.cpp blue-magic/learnt-power-getter.h \
	\
	cmd-action/cmd-attack.cpp cmd-action/cmd-attack.h \
	cmd-action/cmd-hissatsu.cpp cmd-action/cmd-hissatsu.h \
	cmd-action/cmd-mane.cpp cmd-action/cmd-mane.h \
	cmd-action/cmd-mind.cpp cmd-action/cmd-mind.h \
	cmd-action/cmd-move.cpp cmd-action/cmd-move.h \
	cmd-action/cmd-open-close.cpp cmd-action/cmd-open-close.h \
	cmd-action/cmd-others.cpp cmd-action/cmd-others.h \
	cmd-action/cmd-pet.cpp cmd-action/cmd-pet.h \
	cmd-action/cmd-racial.cpp cmd-action/cmd-racial.h \
	cmd-action/cmd-shoot.cpp cmd-action/cmd-shoot.h \
	cmd-action/cmd-spell.cpp cmd-action/cmd-spell.h \
	cmd-action/cmd-travel.cpp cmd-action/cmd-travel.h \
	cmd-action/cmd-tunnel.cpp cmd-action/cmd-tunnel.h \
	\
	cmd-building/cmd-building.cpp cmd-building/cmd-building.h \
	cmd-building/cmd-inn.cpp cmd-building/cmd-inn.h \
	\
	cmd-io/cmd-autopick.cpp cmd-io/cmd-autopick.h \
	cmd-io/cmd-diary.cpp cmd-io/cmd-diary.h \
	cmd-io/cmd-dump.cpp cmd-io/cmd-dump.h \
	cmd-io/cmd-floor.cpp cmd-io/cmd-floor.h \
	cmd-io/cmd-gameoption.cpp cmd-io/cmd-gameoption.h \
	cmd-io/cmd-help.cpp cmd-io/cmd-help.h \
	cmd-io/cmd-knowledge.cpp cmd-io/cmd-knowledge.h \
	cmd-io/cmd-lore.cpp cmd-io/cmd-lore.h \
	cmd-io/cmd-macro.cpp cmd-io/cmd-macro.h \
	cmd-io/cmd-menu-content-table.cpp cmd-io/cmd-menu-content-table.h \
	cmd-io/cmd-process-screen.cpp cmd-io/cmd-process-screen.h \
	cmd-io/cmd-save.cpp cmd-io/cmd-save.h \
	cmd-io/diary-subtitle-table.cpp cmd-io/diary-subtitle-table.h \
	cmd-io/feeling-table.cpp cmd-io/feeling-table.h \
	cmd-io/macro-util.cpp cmd-io/macro-util.h \
	\
	cmd-item/cmd-destroy.cpp cmd-item/cmd-destroy.h \
	cmd-item/cmd-eat.cpp cmd-item/cmd-eat.h \
	cmd-item/cmd-equipment.cpp cmd-item/cmd-equipment.h \
	cmd-item/cmd-item.cpp cmd-item/cmd-item.h \
	cmd-item/cmd-magiceat.cpp cmd-item/cmd-magiceat.h \
	cmd-item/cmd-quaff.cpp cmd-item/cmd-quaff.h \
	cmd-item/cmd-read.cpp cmd-item/cmd-read.h \
	cmd-item/cmd-refill.cpp cmd-item/cmd-refill.h \
	cmd-item/cmd-throw.cpp cmd-item/cmd-throw.h \
	cmd-item/cmd-usestaff.cpp cmd-item/cmd-usestaff.h \
	cmd-item/cmd-zaprod.cpp cmd-item/cmd-zaprod.h \
	cmd-item/cmd-zapwand.cpp cmd-item/cmd-zapwand.h \
	\
	cmd-visual/cmd-draw.cpp cmd-visual/cmd-draw.h \
	cmd-visual/cmd-map.cpp cmd-visual/cmd-map.h \
	cmd-visual/cmd-visuals.cpp cmd-visual/cmd-visuals.h \
	\
	combat/attack-power-table.cpp combat/attack-power-table.h \
	combat/aura-counterattack.cpp combat/aura-counterattack.h \
	combat/hallucination-attacks-table.cpp combat/hallucination-attacks-table.h \
	combat/martial-arts-table.cpp combat/martial-arts-table.h \
	combat/attack-accuracy.cpp combat/attack-accuracy.h \
	combat/slaying.cpp combat/slaying.h \
	combat/combat-options-type.h \
	combat/attack-criticality.cpp combat/attack-criticality.h \
	combat/shoot.cpp combat/shoot.h \
	\
	core/asking-player.cpp core/asking-player.h \
	core/disturbance.cpp core/disturbance.h \
	core/game-closer.cpp core/game-closer.h \
	core/game-play.cpp core/game-play.h \
	core/magic-effects-timeout-reducer.cpp core/magic-effects-timeout-reducer.h \
	core/object-compressor.cpp core/object-compressor.h \
	core/player-processor.cpp core/player-processor.h \
	core/score-util.cpp core/score-util.h \
	core/scores.cpp core/scores.h \
	core/show-file.cpp core/show-file.h \
	core/special-internal-keys.h \
	core/speed-table.cpp core/speed-table.h \
	core/stuff-handler.cpp core/stuff-handler.h \
	core/turn-compensator.cpp core/turn-compensator.h \
	core/visuals-reseter.cpp core/visuals-reseter.h \
	core/window-redrawer.cpp core/window-redrawer.h \
	\
	dungeon/dungeon-flag-mask.h \
	dungeon/dungeon-flag-types.h \
	dungeon/dungeon-processor.cpp dungeon/dungeon-processor.h \
	dungeon/quest.h dungeon/quest.cpp \
	dungeon/quest-completion-checker.cpp dungeon/quest-completion-checker.h \
	dungeon/quest-monster-placer.cpp dungeon/quest-monster-placer.h \
	\
	effect/attribute-types.h \
	effect/effect-feature.cpp effect/effect-feature.h \
	effect/effect-item.cpp effect/effect-item.h \
	effect/spells-effect-util.cpp effect/spells-effect-util.h \
	effect/effect-player.cpp effect/effect-player.h \
	effect/effect-monster.cpp effect/effect-monster.h \
	effect/effect-monster-curse.cpp effect/effect-monster-curse.h \
	effect/effect-monster-util.cpp effect/effect-monster-util.h \
	effect/effect-monster-resist-hurt.cpp effect/effect-monster-resist-hurt.h \
	effect/effect-monster-psi.cpp effect/effect-monster-psi.h \
	effect/effect-monster-oldies.cpp effect/effect-monster-oldies.h \
	effect/effect-monster-charm.cpp effect/effect-monster-charm.h \
	effect/effect-monster-lite-dark.cpp effect/effect-monster-lite-dark.h \
	effect/effect-monster-evil.cpp effect/effect-monster-evil.h \
	effect/effect-monster-spirit.cpp effect/effect-monster-spirit.h \
	effect/effect-monster-switcher.cpp effect/effect-monster-switcher.h \
	effect/effect-player-resist-hurt.cpp effect/effect-player-resist-hurt.h \
	effect/effect-player-oldies.cpp effect/effect-player-oldies.h \
	effect/effect-player-curse.cpp effect/effect-player-curse.h \
	effect/effect-player-spirit.cpp effect/effect-player-spirit.h \
	effect/effect-player-switcher.cpp effect/effect-player-switcher.h \
	effect/effect-processor.cpp effect/effect-processor.h \
	effect/effect-characteristics.h \
	\
	external-lib/include-json.h \
	external-lib/json.hpp \
	\
	flavor/flag-inscriptions-table.cpp flavor/flag-inscriptions-table.h \
	flavor/flavor-describer.cpp flavor/flavor-describer.h \
	flavor/flavor-util.cpp flavor/flavor-util.h \
	flavor/named-item-describer.cpp flavor/named-item-describer.h \
	flavor/object-flavor-types.h \
	flavor/object-flavor.cpp flavor/object-flavor.h \
	flavor/tval-description-switcher.cpp flavor/tval-description-switcher.h \
	\
	floor/cave.cpp floor/cave.h \
	floor/cave-generator.cpp floor/cave-generator.h \
	floor/dungeon-tunnel-util.cpp floor/dungeon-tunnel-util.h \
	floor/fixed-map-generator.cpp floor/fixed-map-generator.h \
	floor/floor-allocation-types.h \
	floor/floor-base-definitions.h \
	floor/floor-changer.cpp floor/floor-changer.h \
	floor/floor-events.cpp floor/floor-events.h \
	floor/floor-generator-util.h \
	floor/floor-generator.cpp floor/floor-generator.h \
	floor/floor-leaver.cpp floor/floor-leaver.h \
	floor/floor-mode-changer.cpp floor/floor-mode-changer.h \
	floor/floor-object.cpp floor/floor-object.h \
	floor/floor-save.cpp floor/floor-save.h \
	floor/floor-save-util.cpp floor/floor-save-util.h \
	floor/floor-streams.cpp floor/floor-streams.h \
	floor/floor-util.cpp floor/floor-util.h \
	floor/geometry.cpp floor/geometry.h \
	floor/line-of-sight.cpp floor/line-of-sight.h \
	floor/object-allocator.cpp floor/object-allocator.h \
	floor/object-scanner.cpp floor/object-scanner.h \
	floor/pattern-walk.cpp floor/pattern-walk.h \
	floor/tunnel-generator.cpp floor/tunnel-generator.h \
	floor/wild.h floor/wild.cpp \
	\
	game-option/auto-destruction-options.cpp game-option/auto-destruction-options.h \
	game-option/birth-options.cpp game-option/birth-options.h \
	game-option/cheat-options.cpp game-option/cheat-options.h \
	game-option/cheat-types.h \
	game-option/disturbance-options.cpp game-option/disturbance-options.h \
	game-option/game-option-page.h \
	game-option/game-play-options.cpp game-option/game-play-options.h \
	game-option/input-options.cpp game-option/input-options.h \
	game-option/keymap-directory-getter.cpp game-option/keymap-directory-getter.h \
	game-option/map-screen-options.cpp game-option/map-screen-options.h \
	game-option/option-flags.cpp game-option/option-flags.h \
	game-option/option-types-table.cpp game-option/option-types-table.h \
	game-option/play-record-options.cpp game-option/play-record-options.h \
	game-option/runtime-arguments.cpp game-option/runtime-arguments.h \
	game-option/special-options.cpp game-option/special-options.h \
	game-option/text-display-options.cpp game-option/text-display-options.h \
	\
	grid/door.cpp grid/door.h \
	grid/feature-generator.cpp grid/feature-generator.h \
	grid/feature.cpp grid/feature.h \
	grid/grid.cpp grid/grid.h \
	grid/lighting-colors-table.cpp grid/lighting-colors-table.h \
	grid/object-placer.cpp grid/object-placer.h \
	grid/trap.cpp grid/trap.h \
	\
	hpmp/hp-mp-processor.cpp hpmp/hp-mp-processor.h \
	hpmp/hp-mp-regenerator.cpp hpmp/hp-mp-regenerator.h \
	\
	info-reader/artifact-reader.cpp info-reader/artifact-reader.h \
	info-reader/baseitem-reader.cpp info-reader/baseitem-reader.h \
	info-reader/baseitem-tokens-table.cpp info-reader/baseitem-tokens-table.h \
	info-reader/dungeon-info-tokens-table.cpp info-reader/dungeon-info-tokens-table.h \
	info-reader/dungeon-reader.cpp info-reader/dungeon-reader.h \
	info-reader/ego-reader.cpp info-reader/ego-reader.h \
	info-reader/feature-info-tokens-table.cpp info-reader/feature-info-tokens-table.h \
	info-reader/feature-reader.cpp info-reader/feature-reader.h \
	info-reader/fixed-map-parser.cpp info-reader/fixed-map-parser.h \
	info-reader/general-parser.cpp info-reader/general-parser.h \
	info-reader/info-reader-util.cpp info-reader/info-reader-util.h \
	info-reader/json-reader-util.cpp info-reader/json-reader-util.h \
	info-reader/magic-reader.cpp info-reader/magic-reader.h \
	info-reader/parse-error-types.h \
	info-reader/race-info-tokens-table.cpp info-reader/race-info-tokens-table.h \
	info-reader/race-reader.cpp info-reader/race-reader.h \
	info-reader/random-grid-effect-types.h \
	info-reader/skill-reader.cpp info-reader/skill-reader.h \
	info-reader/spell-reader.cpp info-reader/spell-reader.h \
	info-reader/vault-reader.cpp info-reader/vault-reader.h \
	\
	inventory/floor-item-getter.cpp inventory/floor-item-getter.h \
	inventory/inventory-curse.cpp inventory/inventory-curse.h \
	inventory/inventory-damage.cpp inventory/inventory-damage.h \
	inventory/inventory-describer.cpp inventory/inventory-describer.h \
	inventory/inventory-object.cpp inventory/inventory-object.h \
	inventory/inventory-slot-types.h \
	inventory/inventory-util.cpp inventory/inventory-util.h \
	inventory/item-getter.cpp inventory/item-getter.h \
	inventory/item-selection-util.cpp inventory/item-selection-util.h \
	inventory/pack-overflow.cpp inventory/pack-overflow.h \
	inventory/player-inventory.cpp inventory/player-inventory.h \
	inventory/recharge-processor.cpp inventory/recharge-processor.h \
	\
	io/command-repeater.cpp io/command-repeater.h \
	io/cursor.cpp io/cursor.h \
	io/exit-panic.cpp io/exit-panic.h \
	io/files-util.cpp io/files-util.h \
	io/gf-descriptions.cpp io/gf-descriptions.h \
	io/input-key-acceptor.cpp io/input-key-acceptor.h \
	io/input-key-processor.cpp io/input-key-processor.h \
	io/input-key-requester.cpp io/input-key-requester.h \
	io/interpret-pref-file.cpp io/interpret-pref-file.h \
	io/mutations-dump.cpp io/mutations-dump.h \
	io/pref-file-expressor.cpp io/pref-file-expressor.h \
	io/read-pref-file.cpp io/read-pref-file.h \
	io/record-play-movie.cpp io/record-play-movie.h \
	io/report.cpp io/report.h \
	io/screen-util.cpp io/screen-util.h \
	io/signal-handlers.cpp io/signal-handlers.h \
	io/tokenizer.cpp io/tokenizer.h \
	io/uid-checker.cpp io/uid-checker.h \
	io/write-diary.cpp io/write-diary.h \
	\
	io-dump/character-dump.cpp io-dump/character-dump.h \
	io-dump/dump-remover.cpp io-dump/dump-remover.h \
	io-dump/dump-util.cpp io-dump/dump-util.h \
	io-dump/player-status-dump.cpp io-dump/player-status-dump.h \
	io-dump/random-art-info-dumper.cpp io-dump/random-art-info-dumper.h \
	io-dump/special-class-dump.cpp io-dump/special-class-dump.h \
	\
	item-info/flavor-initializer.cpp item-info/flavor-initializer.h \
	\
	knowledge/item-group-table.cpp knowledge/item-group-table.h \
	knowledge/knowledge-autopick.cpp knowledge/knowledge-autopick.h \
	knowledge/knowledge-experiences.cpp knowledge/knowledge-experiences.h \
	knowledge/knowledge-features.cpp knowledge/knowledge-features.h \
	knowledge/knowledge-inventory.cpp knowledge/knowledge-inventory.h \
	knowledge/knowledge-items.cpp knowledge/knowledge-items.h \
	knowledge/knowledge-monsters.cpp knowledge/knowledge-monsters.h \
	knowledge/knowledge-mutations.cpp knowledge/knowledge-mutations.h \
	knowledge/knowledge-uniques.cpp knowledge/knowledge-uniques.h \
	knowledge/knowledge-self.cpp knowledge/knowledge-self.h \
	knowledge/knowledge-quests.cpp knowledge/knowledge-quests.h \
	knowledge/lighting-level-table.cpp knowledge/lighting-level-table.h \
	knowledge/monster-group-table.cpp knowledge/monster-group-table.h \
	\
	load/angband-version-comparer.cpp load/angband-version-comparer.h \
	load/birth-loader.cpp load/birth-loader.h \
	load/dummy-loader.cpp load/dummy-loader.h \
	load/dungeon-loader.cpp load/dungeon-loader.h \
	load/extra-loader.cpp load/extra-loader.h \
	load/floor-loader.cpp load/floor-loader.h \
	load/info-loader.cpp load/info-loader.h \
	load/inventory-loader.cpp load/inventory-loader.h \
	load/old-feature-types.h \
	load/load-util.cpp load/load-util.h \
	load/load-zangband.cpp load/load-zangband.h \
	load/quest-loader.cpp load/quest-loader.h \
	load/load.cpp load/load.h \
	load/lore-loader.cpp load/lore-loader.h \
	load/option-loader.cpp load/option-loader.h \
	load/player-attack-loader.cpp load/player-attack-loader.h \
	load/player-class-specific-data-loader.cpp load/player-class-specific-data-loader.h \
	load/player-info-loader.cpp load/player-info-loader.h \
	load/savedata-old-flag-types.h \
	load/store-loader.cpp load/store-loader.h \
	load/world-loader.cpp load/world-loader.h \
	\
	load/item/item-loader-base.cpp load/item/item-loader-base.h \
	load/item/item-loader-factory.cpp load/item/item-loader-factory.h \
	load/item/item-loader-version-types.h \
	\
	load/monster/monster-loader-base.h \
	load/monster/monster-loader-factory.cpp load/monster/monster-loader-factory.h \
	load/monster/monster-loader-version-types.h \
	\
	load/old/item-flag-types-savefile50.h \
	load/old/item-loader-savefile50.cpp load/old/item-loader-savefile50.h \
	load/old/load-v1-5-0.cpp load/old/load-v1-5-0.h \
	load/old/load-v1-7-0.cpp load/old/load-v1-7-0.h \
	load/old/monster-flag-types-savefile50.h \
	load/old/monster-loader-savefile50.cpp load/old/monster-loader-savefile50.h \
	\
	locale/character-encoding.h \
	locale/english.cpp locale/english.h \
	locale/japanese.cpp locale/japanese.h \
	locale/localized-string.cpp locale/localized-string.h \
	locale/utf-8.cpp locale/utf-8.h \
	locale/language-switcher.h \
	\
	lore/combat-types-setter.cpp lore/combat-types-setter.h \
	lore/lore-calculator.cpp lore/lore-calculator.h \
	lore/lore-util.cpp lore/lore-util.h \
	lore/magic-types-setter.cpp lore/magic-types-setter.h \
	lore/monster-lore.cpp lore/monster-lore.h \
	\
	main.cpp main-x11.cpp main-gcu.cpp \
	\
	main/angband-headers.cpp main/angband-headers.h \
	main/angband-initializer.cpp main/angband-initializer.h \
	main/game-data-initializer.cpp main/game-data-initializer.h \
	main/info-initializer.cpp main/info-initializer.h \
	main/init-error-messages-table.cpp main/init-error-messages-table.h \
	main/music-definitions-table.cpp main/music-definitions-table.h \
	main/scene-table.cpp main/scene-table.h \
	main/scene-table-floor.cpp main/scene-table-floor.h \
	main/scene-table-monster.cpp main/scene-table-monster.h \
	main/sound-definitions-table.cpp main/sound-definitions-table.h \
	main/sound-of-music.cpp main/sound-of-music.h \
	\
	main-unix/stack-trace-unix.cpp \
	main-unix/unix-user-ids.cpp main-unix/unix-user-ids.h \
	main-unix/x11-gamma-builder.cpp main-unix/x11-gamma-builder.h \
	main-unix/x11-type-string.cpp main-unix/x11-type-string.h \
	\
	market/arena.cpp market/arena.h \
	market/arena-entry.cpp market/arena-entry.h \
	market/building-initializer.cpp market/building-initializer.h \
	market/building-util.cpp market/building-util.h \
	market/play-gamble.cpp market/play-gamble.h \
	market/poker.cpp market/poker.h \
	market/bounty-prize-table.cpp market/bounty-prize-table.h \
	market/bounty-type-definition.h \
	market/bounty.cpp market/bounty.h \
	market/building-recharger.cpp market/building-recharger.h \
	market/building-quest.cpp market/building-quest.h \
	market/building-service.cpp market/building-service.h \
	market/building-craft-weapon.cpp market/building-craft-weapon.h \
	market/building-craft-armor.cpp market/building-craft-armor.h \
	market/building-craft-fix.cpp market/building-craft-fix.h \
	market/building-monster.cpp market/building-monster.h \
	market/building-enchanter.cpp market/building-enchanter.h \
	market/building-actions-table.h \
	market/melee-arena.cpp market/melee-arena.h \
	\
	melee/monster-attack-monster.cpp melee/monster-attack-monster.h \
	melee/melee-postprocess.cpp melee/melee-postprocess.h \
	melee/melee-spell.cpp melee/melee-spell.h \
	melee/melee-spell-flags-checker.cpp melee/melee-spell-flags-checker.h \
	melee/melee-spell-util.cpp melee/melee-spell-util.h \
	melee/melee-switcher.cpp melee/melee-switcher.h \
	melee/melee-util.cpp melee/melee-util.h \
	\
	mind/drs-types.h \
	mind/mind-archer.cpp mind/mind-archer.h \
	mind/mind-berserker.cpp mind/mind-berserker.h \
	mind/mind-blue-mage.cpp mind/mind-blue-mage.h \
	mind/mind-cavalry.cpp mind/mind-cavalry.h \
	mind/mind-chaos-warrior.cpp mind/mind-chaos-warrior.h \
	mind/mind-elementalist.cpp mind/mind-elementalist.h \
	mind/mind-explanations-table.cpp mind/mind-explanations-table.h \
	mind/mind-force-trainer.cpp mind/mind-force-trainer.h \
	mind/mind-info.cpp mind/mind-info.h \
	mind/mind-hobbit.cpp mind/mind-hobbit.h \
	mind/mind-mage.cpp mind/mind-mage.h \
	mind/mind-magic-eater.cpp mind/mind-magic-eater.h \
	mind/mind-magic-resistance.cpp mind/mind-magic-resistance.h \
	mind/mind-mindcrafter.cpp mind/mind-mindcrafter.h \
	mind/mind-mirror-master.cpp mind/mind-mirror-master.h \
	mind/mind-monk.cpp mind/mind-monk.h \
	mind/mind-ninja.cpp mind/mind-ninja.h \
	mind/mind-numbers.h \
	mind/mind-power-getter.cpp mind/mind-power-getter.h \
	mind/mind-priest.cpp mind/mind-priest.h \
	mind/mind-samurai.cpp mind/mind-samurai.h \
	mind/mind-sniper.cpp mind/mind-sniper.h \
	mind/mind-types.h \
	mind/mind-warrior.cpp mind/mind-warrior.h \
	mind/mind-warrior-mage.cpp mind/mind-warrior-mage.h \
	mind/mind-weaponsmith.cpp mind/mind-weaponsmith.h \
	mind/monk-attack.cpp mind/monk-attack.h \
	mind/snipe-types.h \
	mind/stances-table.cpp mind/stances-table.h \
	\
	monster/horror-descriptions.cpp monster/horror-descriptions.h \
	monster/monster-compaction.cpp monster/monster-compaction.h \
	monster/monster-damage.cpp monster/monster-damage.h \
	monster/monster-describer.cpp monster/monster-describer.h \
	monster/monster-description-types.h \
	monster/monster-flag-types.h \
	monster/monster-info.cpp monster/monster-info.h \
	monster/monster-list.cpp monster/monster-list.h \
	monster/monster-pain-describer.cpp monster/monster-pain-describer.h \
	monster/monster-processor.cpp monster/monster-processor.h \
	monster/monster-processor-util.cpp monster/monster-processor-util.h \
	monster/monster-timed-effects.cpp monster/monster-timed-effects.h \
	monster/monster-status.cpp monster/monster-status.h \
	monster/monster-status-setter.cpp monster/monster-status-setter.h \
	monster/monster-update.cpp monster/monster-update.h \
	monster/monster-util.cpp monster/monster-util.h \
	monster/smart-learn-types.h \
	\
	monster-attack/insults-moans.cpp monster-attack/insults-moans.h \
	monster-attack/monster-attack-describer.cpp monster-attack/monster-attack-describer.h \
	monster-attack/monster-attack-effect.h \
	monster-attack/monster-attack-lose.cpp monster-attack/monster-attack-lose.h \
	monster-attack/monster-attack-player.cpp monster-attack/monster-attack-player.h \
	monster-attack/monster-attack-processor.cpp monster-attack/monster-attack-processor.h \
	monster-attack/monster-attack-status.cpp monster-attack/monster-attack-status.h \
	monster-attack/monster-attack-switcher.cpp monster-attack/monster-attack-switcher.h \
	monster-attack/monster-attack-table.cpp monster-attack/monster-attack-table.h \
	monster-attack/monster-eating.cpp monster-attack/monster-eating.h \
	\
	monster-floor/monster-death.cpp monster-floor/monster-death.h \
	monster-floor/monster-death-util.cpp monster-floor/monster-death-util.h \
	monster-floor/monster-direction.cpp monster-floor/monster-direction.h \
	monster-floor/monster-dist-offsets.cpp monster-floor/monster-dist-offsets.h \
	monster-floor/monster-generator.cpp monster-floor/monster-generator.h \
	monster-floor/monster-move.cpp monster-floor/monster-move.h \
	monster-floor/monster-object.cpp monster-floor/monster-object.h \
	monster-floor/monster-remover.cpp monster-floor/monster-remover.h \
	monster-floor/monster-runaway.cpp monster-floor/monster-runaway.h \
	monster-floor/monster-safety-hiding.cpp monster-floor/monster-safety-hiding.h \
	monster-floor/monster-summon.cpp monster-floor/monster-summon.h \
	monster-floor/monster-sweep-grid.cpp monster-floor/monster-sweep-grid.h \
	monster-floor/monster-lite.cpp monster-floor/monster-lite.h \
	monster-floor/monster-lite-util.cpp monster-floor/monster-lite-util.h \
	monster-floor/one-monster-placer.cpp monster-floor/one-monster-placer.h \
	monster-floor/place-monster-types.h \
	monster-floor/quantum-effect.cpp monster-floor/quantum-effect.h \
	monster-floor/special-death-switcher.cpp monster-floor/special-death-switcher.h \
	\
	monster-race/monster-aura-types.h \
	monster-race/monster-kind-mask.cpp monster-race/monster-kind-mask.h \
	monster-race/monster-race-hook.cpp monster-race/monster-race-hook.h \
	monster-race/race-ability-flags.h \
	monster-race/race-ability-mask.cpp monster-race/race-ability-mask.h \
	monster-race/race-behavior-flags.h \
	monster-race/race-brightness-flags.h \
	monster-race/race-brightness-mask.h monster-race/race-brightness-mask.cpp \
	monster-race/race-drop-flags.h \
	monster-race/race-feature-flags.h \
	monster-race/race-flags-resistance.h \
	monster-race/race-kind-flags.h \
	monster-race/race-misc-flags.h \
	monster-race/race-population-flags.h \
	monster-race/race-resistance-mask.cpp monster-race/race-resistance-mask.h \
	monster-race/race-sex.h \
	monster-race/race-speak-flags.h \
	monster-race/race-special-flags.h \
	monster-race/race-visual-flags.h \
	monster-race/race-wilderness-flags.h \
	\
	mspell/assign-monster-spell.cpp mspell/assign-monster-spell.h \
	mspell/element-resistance-checker.cpp mspell/element-resistance-checker.h \
	mspell/high-resistance-checker.cpp mspell/high-resistance-checker.h \
	mspell/improper-mspell-remover.cpp mspell/improper-mspell-remover.h \
	mspell/monster-power-table.cpp mspell/monster-power-table.h \
	mspell/mspell-attack.cpp mspell/mspell-attack.h \
	mspell/mspell-attack-util.cpp mspell/mspell-attack-util.h \
	mspell/mspell-checker.cpp mspell/mspell-checker.h \
	mspell/mspell-damage-calculator.cpp mspell/mspell-damage-calculator.h \
	mspell/mspell-data.cpp mspell/mspell-data.h \
	mspell/mspell-dispel.cpp mspell/mspell-dispel.h \
	mspell/mspell-floor.cpp mspell/mspell-floor.h \
	mspell/mspell-judgement.cpp mspell/mspell-judgement.h \
	mspell/mspell-learn-checker.cpp mspell/mspell-learn-checker.h \
	mspell/mspell-lite.cpp mspell/mspell-lite.h \
	mspell/mspell-result.h \
	mspell/mspell-special.cpp mspell/mspell-special.h \
	mspell/mspell-summon.cpp mspell/mspell-summon.h \
	mspell/mspell-status.cpp mspell/mspell-status.h \
	mspell/mspell-selector.cpp mspell/mspell-selector.h \
	mspell/mspell-util.cpp mspell/mspell-util.h \
	mspell/smart-mspell-util.cpp mspell/smart-mspell-util.h \
	mspell/specified-summon.cpp mspell/specified-summon.h \
	mspell/summon-checker.cpp mspell/summon-checker.h \
	\
	mspell/mspell-attack/abstract-mspell.cpp mspell/mspell-attack/abstract-mspell.h \
	mspell/mspell-attack/mspell-ball.cpp mspell/mspell-attack/mspell-ball.h \
	mspell/mspell-attack/mspell-bolt.cpp mspell/mspell-attack/mspell-bolt.h \
	mspell/mspell-attack/mspell-breath.cpp mspell/mspell-attack/mspell-breath.h \
	mspell/mspell-attack/mspell-curse.cpp mspell/mspell-attack/mspell-curse.h \
	mspell/mspell-attack/mspell-particularity.cpp mspell/mspell-attack/mspell-particularity.h \
	\
	mutation/gain-mutation-switcher.cpp mutation/gain-mutation-switcher.h \
	mutation/lose-mutation-switcher.cpp mutation/lose-mutation-switcher.h \
	mutation/mutation-calculator.cpp mutation/mutation-calculator.h \
	mutation/mutation-investor-remover.cpp mutation/mutation-investor-remover.h \
	mutation/mutation-util.cpp mutation/mutation-util.h \
	mutation/mutation-flag-types.h \
	mutation/mutation-processor.cpp mutation/mutation-processor.h \
	mutation/mutation-techniques.cpp mutation/mutation-techniques.h \
	\
	net/curl-easy-session.cpp net/curl-easy-session.h \
	net/curl-slist.cpp net/curl-slist.h \
	net/http-client.cpp net/http-client.h \
	net/report-error.cpp net/report-error.h \
	\
	object/item-tester-hooker.cpp object/item-tester-hooker.h \
	object/object-broken.cpp object/object-broken.h \
	object/object-index-list.cpp object/object-index-list.h \
	object/object-info.cpp object/object-info.h \
	object/object-kind-hook.cpp object/object-kind-hook.h \
	object/object-stack.cpp object/object-stack.h \
	object/object-value.cpp object/object-value.h \
	object/object-value-calc.cpp object/object-value-calc.h \
	object/lite-processor.cpp object/lite-processor.h \
	object/tval-types.h \
	object/warning.cpp object/warning.h \
	object/object-mark-types.h \
	object/item-use-flags.h \
	\
	object-activation/activation-bolt-ball.cpp object-activation/activation-bolt-ball.h \
	object-activation/activation-breath.cpp object-activation/activation-breath.h \
	object-activation/activation-charm.cpp object-activation/activation-charm.h \
	object-activation/activation-genocide.cpp object-activation/activation-genocide.h \
	object-activation/activation-others.cpp object-activation/activation-others.h \
	object-activation/activation-resistance.cpp object-activation/activation-resistance.h \
	object-activation/activation-switcher.cpp object-activation/activation-switcher.h \
	object-activation/activation-teleport.cpp object-activation/activation-teleport.h \
	object-activation/activation-util.cpp object-activation/activation-util.h \
	\
	object-enchant/activation-info-table.cpp object-enchant/activation-info-table.h \
	object-enchant/dragon-breaths-table.cpp object-enchant/dragon-breaths-table.h \
	object-enchant/enchanter-base.h \
	object-enchant/enchanter-factory.cpp object-enchant/enchanter-factory.h \
	object-enchant/item-magic-applier.cpp object-enchant/item-magic-applier.h \
	object-enchant/object-boost.cpp object-enchant/object-boost.h \
	object-enchant/object-curse.cpp object-enchant/object-curse.h \
	object-enchant/object-ego.cpp object-enchant/object-ego.h \
	object-enchant/item-apply-magic.h object-enchant/item-feeling.h \
	object-enchant/old-ego-extra-values.h object-enchant/special-object-flags.h \
	object-enchant/tr-flags.h \
	object-enchant/tr-types.h object-enchant/trc-types.h \
	object-enchant/trg-types.h \
	object-enchant/vorpal-weapon.cpp object-enchant/vorpal-weapon.h \
	\
	object-enchant/others/apply-magic-amulet.cpp object-enchant/others/apply-magic-amulet.h \
	object-enchant/others/apply-magic-lite.cpp object-enchant/others/apply-magic-lite.h \
	object-enchant/others/apply-magic-ring.cpp object-enchant/others/apply-magic-ring.h \
	object-enchant/others/apply-magic-others.cpp object-enchant/others/apply-magic-others.h \
	\
	object-enchant/protector/abstract-protector-enchanter.cpp object-enchant/protector/abstract-protector-enchanter.h \
	object-enchant/protector/apply-magic-armor.cpp object-enchant/protector/apply-magic-armor.h \
	object-enchant/protector/apply-magic-boots.cpp object-enchant/protector/apply-magic-boots.h \
	object-enchant/protector/apply-magic-cloak.cpp object-enchant/protector/apply-magic-cloak.h \
	object-enchant/protector/apply-magic-crown.cpp object-enchant/protector/apply-magic-crown.h \
	object-enchant/protector/apply-magic-dragon-armor.cpp object-enchant/protector/apply-magic-dragon-armor.h \
	object-enchant/protector/apply-magic-gloves.cpp object-enchant/protector/apply-magic-gloves.h \
	object-enchant/protector/apply-magic-hard-armor.cpp object-enchant/protector/apply-magic-hard-armor.h \
	object-enchant/protector/apply-magic-helm.cpp object-enchant/protector/apply-magic-helm.h \
	object-enchant/protector/apply-magic-shield.cpp object-enchant/protector/apply-magic-shield.h \
	object-enchant/protector/apply-magic-soft-armor.cpp object-enchant/protector/apply-magic-soft-armor.h \
	\
	object-enchant/weapon/abstract-weapon-enchanter.cpp object-enchant/weapon/abstract-weapon-enchanter.h \
	object-enchant/weapon/apply-magic-arrow.cpp object-enchant/weapon/apply-magic-arrow.h \
	object-enchant/weapon/apply-magic-bow.cpp object-enchant/weapon/apply-magic-bow.h \
	object-enchant/weapon/apply-magic-digging.cpp object-enchant/weapon/apply-magic-digging.h \
	object-enchant/weapon/apply-magic-hafted.cpp object-enchant/weapon/apply-magic-hafted.h \
	object-enchant/weapon/apply-magic-polearm.cpp object-enchant/weapon/apply-magic-polearm.h \
	object-enchant/weapon/apply-magic-sword.cpp object-enchant/weapon/apply-magic-sword.h \
	object-enchant/weapon/melee-weapon-enchanter.cpp object-enchant/weapon/melee-weapon-enchanter.h \
	\
	object-hook/hook-armor.cpp object-hook/hook-armor.h \
	object-hook/hook-expendable.cpp object-hook/hook-expendable.h \
	object-hook/hook-magic.cpp object-hook/hook-magic.h \
	object-hook/hook-perception.cpp object-hook/hook-perception.h \
	object-hook/hook-weapon.cpp object-hook/hook-weapon.h \
	\
	object-use/item-use-checker.cpp object-use/item-use-checker.h \
	object-use/throw-execution.cpp object-use/throw-execution.h \
	object-use/use-execution.cpp object-use/use-execution.h \
	object-use/zaprod-execution.cpp object-use/zaprod-execution.h \
	object-use/zapwand-execution.cpp object-use/zapwand-execution.h \
	\
	object-use/quaff/quaff-effects.cpp object-use/quaff/quaff-effects.h \
	object-use/quaff/quaff-execution.cpp object-use/quaff/quaff-execution.h \
	\
	object-use/read/read-execution.cpp object-use/read/read-execution.h \
	object-use/read/gbh-shirt-read-executor.cpp object-use/read/gbh-shirt-read-executor.h \
	object-use/read/parchment-read-executor.cpp object-use/read/parchment-read-executor.h \
	object-use/read/read-executor-base.h \
	object-use/read/read-executor-factory.cpp object-use/read/read-executor-factory.h \
	object-use/read/ring-power-read-executor.cpp object-use/read/ring-power-read-executor.h \
	object-use/read/scroll-read-executor.cpp object-use/read/scroll-read-executor.h \
	\
	perception/identification.cpp perception/identification.h \
	perception/object-perception.cpp perception/object-perception.h \
	perception/simple-perception.cpp perception/simple-perception.h \
	\
	pet/pet-fall-off.cpp pet/pet-fall-off.h \
	pet/pet-util.cpp pet/pet-util.h \
	\
	player/attack-defense-types.h \
	player/eldritch-horror.cpp player/eldritch-horror.h \
	player/patron.cpp player/patron.h \
	player/process-death.cpp player/process-death.h \
	player/process-name.cpp player/process-name.h \
	player/race-info-table.cpp player/race-info-table.h \
	player/race-resistances.cpp player/race-resistances.h \
	player/permanent-resistances.cpp player/permanent-resistances.h \
	player/temporary-resistances.cpp player/temporary-resistances.h \
	player/digestion-processor.cpp player/digestion-processor.h \
	player/player-damage.cpp player/player-damage.h \
	player/player-move.cpp player/player-move.h \
	player/player-personality.cpp player/player-personality.h \
	player/player-realm.cpp player/player-realm.h \
	player/player-skill.cpp player/player-skill.h \
	player/player-spell-status.cpp player/player-spell-status.h \
	player/player-status.cpp player/player-status.h \
	player/player-status-flags.cpp player/player-status-flags.h \
	player/player-status-table.cpp player/player-status-table.h \
	player/player-status-resist.cpp player/player-status-resist.h \
	player/player-personality-types.h \
	player/player-sex.cpp player/player-sex.h \
	player/player-view.cpp player/player-view.h \
	player/special-defense-types.h \
	\
	player-ability/player-ability-types.h \
	player-ability/player-charisma.cpp player-ability/player-charisma.h \
	player-ability/player-constitution.cpp player-ability/player-constitution.h \
	player-ability/player-dexterity.cpp player-ability/player-dexterity.h \
	player-ability/player-intelligence.cpp player-ability/player-intelligence.h \
	player-ability/player-strength.cpp player-ability/player-strength.h \
	player-ability/player-wisdom.cpp player-ability/player-wisdom.h \
	\
	player-attack/attack-chaos-effect.cpp player-attack/attack-chaos-effect.h \
	player-attack/blood-sucking-processor.cpp player-attack/blood-sucking-processor.h \
	player-attack/player-attack.cpp player-attack/player-attack.h \
	\
	player-base/player-class.cpp player-base/player-class.h \
	player-base/player-race.cpp player-base/player-race.h \
	\
	player-info/alignment.cpp player-info/alignment.h \
	player-info/bard-data-type.h \
	player-info/base-status-info.cpp player-info/base-status-info.h \
	player-info/bluemage-data-type.h \
	player-info/body-improvement-info.cpp player-info/body-improvement-info.h \
	player-info/class-ability-info.cpp player-info/class-ability-info.h \
	player-info/class-types.h \
	player-info/class-info.cpp player-info/class-info.h \
	player-info/class-specific-data.h \
	player-info/equipment-info.cpp player-info/equipment-info.h \
	player-info/force-trainer-data-type.h \
	player-info/magic-eater-data-type.cpp player-info/magic-eater-data-type.h \
	player-info/mane-data-type.h \
	player-info/mimic-info-table.cpp player-info/mimic-info-table.h \
	player-info/monk-data-type.h \
	player-info/mutation-info.cpp player-info/mutation-info.h \
	player-info/ninja-data-type.h \
	player-info/race-ability-info.cpp player-info/race-ability-info.h \
	player-info/race-info.cpp player-info/race-info.h \
	player-info/race-types.h \
	player-info/resistance-info.cpp player-info/resistance-info.h \
	player-info/samurai-data-type.h \
	player-info/self-info.cpp player-info/self-info.h \
	player-info/self-info-util.cpp player-info/self-info-util.h \
	player-info/smith-data-type.h \
	player-info/sniper-data-type.h \
	player-info/spell-hex-data-type.h \
	player-info/weapon-effect-info.cpp player-info/weapon-effect-info.h \
	\
	player-status/player-hand-types.h \
	player-status/player-infravision.cpp player-status/player-infravision.h \
	player-status/player-basic-statistics.cpp player-status/player-basic-statistics.h \
	player-status/player-energy.cpp player-status/player-energy.h \
	player-status/player-speed.cpp player-status/player-speed.h \
	player-status/player-status-base.cpp player-status/player-status-base.h \
	player-status/player-stealth.cpp player-status/player-stealth.h \
	\
	racial/class-racial-switcher.cpp racial/class-racial-switcher.h \
	racial/mutation-racial-selector.cpp racial/mutation-racial-selector.h \
	racial/race-racial-command-setter.cpp racial/race-racial-command-setter.h \
	racial/racial-android.cpp racial/racial-android.h \
	racial/racial-balrog.cpp racial/racial-balrog.h \
	racial/racial-draconian.cpp racial/racial-draconian.h \
	racial/racial-kutar.cpp racial/racial-kutar.h \
	racial/racial-switcher.cpp racial/racial-switcher.h \
	racial/racial-util.cpp racial/racial-util.h \
	racial/racial-vampire.cpp racial/racial-vampire.h \
	\
	realm/realm-arcane.cpp realm/realm-arcane.h \
	realm/realm-chaos.cpp realm/realm-chaos.h \
	realm/realm-craft.cpp realm/realm-craft.h \
	realm/realm-crusade.cpp realm/realm-crusade.h \
	realm/realm-demon.cpp realm/realm-demon.h \
	realm/realm-death.cpp realm/realm-death.h \
	realm/realm-hex.cpp realm/realm-hex.h \
	realm/realm-hex-numbers.h \
	realm/realm-hissatsu.cpp realm/realm-hissatsu.h \
	realm/realm-life.cpp realm/realm-life.h \
	realm/realm-nature.cpp realm/realm-nature.h \
	realm/realm-song.cpp realm/realm-song.h \
	realm/realm-song-numbers.h \
	realm/realm-sorcery.cpp realm/realm-sorcery.h \
	realm/realm-trump.cpp realm/realm-trump.h \
	realm/realm-types.h \
	\
	room/cave-filler.cpp room/cave-filler.h \
	room/door-definition.cpp room/door-definition.h \
	room/lake-types.h room/room-types.h \
	room/pit-nest-util.cpp room/pit-nest-util.h \
	room/room-generator.cpp room/room-generator.h \
	room/room-info-table.cpp room/room-info-table.h \
	room/rooms-builder.cpp room/rooms-builder.h \
	room/rooms-city.cpp room/rooms-city.h \
	room/rooms-fractal.cpp room/rooms-fractal.h \
	room/rooms-maze-vault.cpp room/rooms-maze-vault.h \
	room/rooms-nest.cpp room/rooms-nest.h \
	room/rooms-normal.cpp room/rooms-normal.h \
	room/rooms-pit.cpp room/rooms-pit.h\
	room/rooms-special.cpp room/rooms-special.h \
	room/rooms-trap.cpp room/rooms-trap.h \
	room/rooms-vault.cpp room/rooms-vault.h \
	room/space-finder.cpp room/space-finder.h \
	room/treasure-deployment.cpp room/treasure-deployment.h \
	room/vault-builder.cpp room/vault-builder.h \
	\
	save/floor-writer.cpp save/floor-writer.h \
	save/info-writer.cpp save/info-writer.h \
	save/item-writer.cpp save/item-writer.h \
	save/lore-writer.cpp save/lore-writer.h \
	save/monster-entity-writer.cpp save/monster-entity-writer.h \
	save/player-class-specific-data-writer.cpp save/player-class-specific-data-writer.h \
	save/player-writer.cpp save/player-writer.h \
	save/save.cpp save/save.h \
	save/save-util.cpp save/save-util.h \
	\
	smith/object-smith.cpp smith/object-smith.h \
	smith/smith-info.cpp smith/smith-info.h \
	smith/smith-tables.cpp smith/smith-tables.h \
	smith/smith-types.h \
	\
	specific-object/blade-turner.cpp specific-object/blade-turner.h \
	specific-object/bloody-moon.cpp specific-object/bloody-moon.h \
	specific-object/chest.cpp specific-object/chest.h \
	specific-object/death-crimson.cpp specific-object/death-crimson.h \
	specific-object/death-scythe.cpp specific-object/death-scythe.h \
	specific-object/monster-ball.cpp specific-object/monster-ball.h \
	specific-object/muramasa.cpp specific-object/muramasa.h \
	specific-object/ring-of-power.cpp specific-object/ring-of-power.h \
	specific-object/stone-of-lore.cpp specific-object/stone-of-lore.h \
	specific-object/toragoroshi.cpp specific-object/toragoroshi.h \
	specific-object/torch.cpp specific-object/torch.h \
	\
	spell/range-calc.cpp spell/range-calc.h \
	spell/spells-describer.cpp spell/spells-describer.h \
	spell/spells-diceroll.cpp spell/spells-diceroll.h \
	spell/spells-execution.cpp spell/spells-execution.h \
	spell/spell-info.cpp spell/spell-info.h \
	spell/spells-object.cpp spell/spells-object.h \
	spell/spells-staff-only.cpp spell/spells-staff-only.h \
	spell/spells-summon.cpp spell/spells-summon.h \
	spell/spells-status.cpp spell/spells-status.h \
	spell/spells-util.h \
	spell/summon-types.h \
	spell/technic-info-table.cpp spell/technic-info-table.h \
	\
	spell-class/spells-mirror-master.cpp spell-class/spells-mirror-master.h \
	\
	spell-kind/blood-curse.cpp spell-kind/blood-curse.h \
	spell-kind/earthquake.cpp spell-kind/earthquake.h \
	spell-kind/magic-item-recharger.cpp spell-kind/magic-item-recharger.h \
	spell-kind/spells-beam.cpp spell-kind/spells-beam.h \
	spell-kind/spells-charm.cpp spell-kind/spells-charm.h \
	spell-kind/spells-curse-removal.cpp spell-kind/spells-curse-removal.h \
	spell-kind/spells-detection.cpp spell-kind/spells-detection.h \
	spell-kind/spells-enchant.cpp spell-kind/spells-enchant.h \
	spell-kind/spells-equipment.cpp spell-kind/spells-equipment.h \
	spell-kind/spells-fetcher.cpp spell-kind/spells-fetcher.h \
	spell-kind/spells-floor.cpp spell-kind/spells-floor.h \
	spell-kind/spells-genocide.cpp spell-kind/spells-genocide.h \
	spell-kind/spells-grid.cpp spell-kind/spells-grid.h \
	spell-kind/spells-launcher.cpp spell-kind/spells-launcher.h \
	spell-kind/spells-lite.cpp spell-kind/spells-lite.h \
	spell-kind/spells-neighbor.cpp spell-kind/spells-neighbor.h \
	spell-kind/spells-pet.cpp spell-kind/spells-pet.h \
	spell-kind/spells-perception.cpp spell-kind/spells-perception.h \
	spell-kind/spells-polymorph.cpp spell-kind/spells-polymorph.h \
	spell-kind/spells-random.cpp spell-kind/spells-random.h \
	spell-kind/spells-sight.cpp spell-kind/spells-sight.h \
	spell-kind/spells-specific-bolt.cpp spell-kind/spells-specific-bolt.h \
	spell-kind/spells-teleport.cpp spell-kind/spells-teleport.h \
	spell-kind/spells-world.cpp spell-kind/spells-world.h \
	\
	spell-realm/spells-arcane.cpp spell-realm/spells-arcane.h \
	spell-realm/spells-chaos.cpp spell-realm/spells-chaos.h \
	spell-realm/spells-craft.cpp spell-realm/spells-craft.h \
	spell-realm/spells-crusade.cpp spell-realm/spells-crusade.h \
	spell-realm/spells-demon.cpp spell-realm/spells-demon.h \
	spell-realm/spells-hex.cpp spell-realm/spells-hex.h \
	spell-realm/spells-nature.cpp spell-realm/spells-nature.h \
	spell-realm/spells-song.cpp spell-realm/spells-song.h \
	spell-realm/spells-sorcery.cpp spell-realm/spells-sorcery.h \
	spell-realm/spells-trump.cpp spell-realm/spells-trump.h \
	\
	status/action-setter.cpp status/action-setter.h \
	status/bad-status-setter.cpp status/bad-status-setter.h \
	status/base-status.cpp status/base-status.h \
	status/body-improvement.cpp status/body-improvement.h \
	status/buff-setter.cpp status/buff-setter.h \
	status/element-resistance.cpp status/element-resistance.h \
	status/experience.cpp status/experience.h \
	status/shape-changer.cpp status/shape-changer.h \
	status/sight-setter.cpp status/sight-setter.h \
	status/temporary-resistance.cpp status/temporary-resistance.h \
	\
	store/home.cpp store/home.h \
	store/store.cpp store/store.h \
	store/articles-on-sale.cpp store/articles-on-sale.h \
	store/black-market.cpp store/black-market.h \
	store/cmd-store.cpp store/cmd-store.h \
	store/gold-magnification-table.cpp store/gold-magnification-table.h \
	store/museum.cpp store/museum.h \
	store/pricing.cpp store/pricing.h \
	store/purchase-order.cpp store/purchase-order.h \
	store/rumor.cpp store/rumor.h \
	store/say-comments.cpp store/say-comments.h \
	store/sell-order.cpp store/sell-order.h \
	store/service-checker.cpp store/service-checker.h \
	store/store-key-processor.cpp store/store-key-processor.h \
	store/store-util.cpp store/store-util.h \
	store/store-owners.cpp store/store-owners.h \
	store/store-owner-comments.cpp store/store-owner-comments.h \
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
	system/angband.h \
	system/angband-exceptions.h \
	system/angband-system.h system/angband-system.cpp \
	system/angband-version.cpp system/angband-version.h \
	system/artifact-type-definition.cpp system/artifact-type-definition.h \
	system/building-type-definition.cpp system/building-type-definition.h \
	system/grid-type-definition.cpp system/grid-type-definition.h \
	system/h-basic.h system/h-config.h \
	system/h-system.h system/h-type.h \
	system/inner-game-data.cpp system/inner-game-data.h \
	system/item-entity.cpp system/item-entity.h \
	system/monster-entity.cpp system/monster-entity.h \
	system/player-type-definition.cpp system/player-type-definition.h \
	system/redrawing-flags-updater.cpp system/redrawing-flags-updater.h \
	system/spell-info-list.cpp system/spell-info-list.h \
	system/system-variables.cpp system/system-variables.h \
	system/gamevalue.h \
	\
	system/baseitem/baseitem-allocation.cpp system/baseitem/baseitem-allocation.h \
	system/baseitem/baseitem-definition.cpp system/baseitem/baseitem-definition.h \
	system/baseitem/baseitem-key.cpp system/baseitem/baseitem-key.h \
	system/baseitem/baseitem-list.cpp system/baseitem/baseitem-list.h \
	\
	system/dungeon/dungeon-data-definition.cpp system/dungeon/dungeon-data-definition.h \
	system/dungeon/dungeon-definition.cpp system/dungeon/dungeon-definition.h \
	system/dungeon/dungeon-list.cpp system/dungeon/dungeon-list.h \
	system/dungeon/dungeon-record.cpp system/dungeon/dungeon-record.h \
	\
	system/enums/grid-flow.h \
	system/enums/grid-count-kind.h \
	system/enums/dungeon/dungeon-id.h \
	system/enums/monrace/monrace-id.h \
	system/enums/terrain/terrain-characteristics.h \
	system/enums/terrain/terrain-tag.h \
	\
	system/floor/floor-info.cpp system/floor/floor-info.h \
	system/floor/floor-list.cpp system/floor/floor-list.h \
	system/floor/town-info.cpp system/floor/town-info.h \
	system/floor/town-list.cpp system/floor/town-list.h \
	\
	system/monrace/monrace-allocation.cpp system/monrace/monrace-allocation.h \
	system/monrace/monrace-definition.cpp system/monrace/monrace-definition.h \
	system/monrace/monrace-list.cpp system/monrace/monrace-list.h \
	\
	system/services/baseitem-monrace-service.cpp system/services/baseitem-monrace-service.h \
	system/services/dungeon-service.cpp system/services/dungeon-service.h \
	\
	system/terrain/terrain-definition.cpp system/terrain/terrain-definition.h \
	system/terrain/terrain-list.cpp system/terrain/terrain-list.h \
	\
	target/grid-selector.cpp target/grid-selector.h \
	target/projection-path-calculator.cpp target/projection-path-calculator.h \
	target/target-checker.cpp target/target-checker.h \
	target/target-describer.cpp target/target-describer.h \
	target/target-getter.cpp target/target-getter.h \
	target/target-preparation.cpp target/target-preparation.h \
	target/target-setter.cpp target/target-setter.h \
	target/target-sorter.cpp target/target-sorter.h \
	target/target-types.h \
	\
	term/gameterm.cpp term/gameterm.h \
	term/screen-processor.cpp term/screen-processor.h \
	term/term-color-types.h \
	term/z-form.cpp term/z-form.h term/z-rand.cpp term/z-rand.h \
	term/z-term.cpp term/z-term.h term/z-util.cpp term/z-util.h \
	term/z-virt.cpp term/z-virt.h \
	\
	timed-effect/player-acceleration.cpp timed-effect/player-acceleration.h \
	timed-effect/player-blindness.cpp timed-effect/player-blindness.h \
	timed-effect/player-confusion.cpp timed-effect/player-confusion.h \
	timed-effect/player-cut.cpp timed-effect/player-cut.h \
	timed-effect/player-deceleration.cpp timed-effect/player-deceleration.h \
	timed-effect/player-fear.cpp timed-effect/player-fear.h \
	timed-effect/player-hallucination.cpp timed-effect/player-hallucination.h \
	timed-effect/player-paralysis.cpp timed-effect/player-paralysis.h \
	timed-effect/player-poison.cpp timed-effect/player-poison.h \
	timed-effect/player-protection.cpp timed-effect/player-protection.h \
	timed-effect/player-stun.cpp timed-effect/player-stun.h \
	timed-effect/timed-effects.cpp timed-effect/timed-effects.h \
	\
	tracking/baseitem-tracker.cpp tracking/baseitem-tracker.h \
	tracking/health-bar-tracker.cpp tracking/health-bar-tracker.h \
	tracking/lore-tracker.cpp tracking/lore-tracker.h \
	\
	util/abstract-vector-wrapper.h \
	util/angband-files.cpp util/angband-files.h \
	util/buffer-shaper.cpp util/buffer-shaper.h \
	util/bit-flags-calculator.h \
	util/candidate-selector.cpp util/candidate-selector.h \
	util/enum-converter.h \
	util/enum-range.h \
	util/finalizer.h \
	util/flag-group.h \
	util/dice.cpp util/dice.h \
	util/int-char-converter.h \
	util/object-sort.cpp util/object-sort.h \
	util/point-2d.h \
	util/probability-table.h \
	util/rng-xoshiro.cpp util/rng-xoshiro.h \
	util/sha256.cpp util/sha256.h \
	util/stack-trace.h \
	util/string-processor.cpp util/string-processor.h \
	\
	view/display-birth.cpp view/display-birth.h \
	view/display-characteristic.cpp view/display-characteristic.h \
	view/display-fruit.cpp view/display-fruit.h \
	view/display-inventory.cpp view/display-inventory.h\
	view/display-lore-attacks.cpp view/display-lore-attacks.h \
	view/display-lore-drops.cpp view/display-lore-drops.h \
	view/display-lore-magics.cpp view/display-lore-magics.h \
	view/display-lore-status.cpp view/display-lore-status.h \
	view/display-lore.cpp view/display-lore.h \
	view/display-map.cpp view/display-map.h \
	view/display-messages.cpp view/display-messages.h \
	view/display-monster-status.cpp view/display-monster-status.h \
	view/display-player-stat-info.cpp view/display-player-stat-info.h \
	view/display-player-misc-info.cpp view/display-player-misc-info.h \
	view/display-player-middle.cpp view/display-player-middle.h \
	view/display-player.cpp view/display-player.h \
	view/display-scores.cpp view/display-scores.h \
	view/display-self-info.cpp view/display-self-info.h \
	view/display-store.cpp view/display-store.h \
	view/display-symbol.h \
	view/display-util.cpp view/display-util.h \
	view/object-describer.cpp view/object-describer.h \
	view/status-first-page.cpp view/status-first-page.h \
	view/status-bars-table.cpp view/status-bars-table.h \
	\
	window/display-sub-windows.cpp window/display-sub-windows.h \
	window/main-window-left-frame.cpp window/main-window-left-frame.h \
	window/main-window-row-column.h \
	window/main-window-stat-poster.cpp window/main-window-stat-poster.h \
	window/main-window-util.cpp window/main-window-util.h \
	window/main-window-equipments.cpp window/main-window-equipments.h \
	\
	wizard/artifact-analyzer.cpp wizard/artifact-analyzer.h \
	wizard/artifact-bias-table.cpp wizard/artifact-bias-table.h \
	wizard/cmd-wizard.cpp wizard/cmd-wizard.h \
	wizard/fixed-artifacts-spoiler.cpp wizard/fixed-artifacts-spoiler.h \
	wizard/items-spoiler.cpp wizard/items-spoiler.h \
	wizard/monster-info-spoiler.cpp wizard/monster-info-spoiler.h \
	wizard/spoiler-table.cpp wizard/spoiler-table.h \
	wizard/spoiler-util.cpp wizard/spoiler-util.h \
	wizard/tval-descriptions-table.cpp wizard/tval-descriptions-table.h \
	wizard/wizard-game-modifier.cpp wizard/wizard-game-modifier.h \
	wizard/wizard-item-modifier.cpp wizard/wizard-item-modifier.h \
	wizard/wizard-player-modifier.cpp wizard/wizard-player-modifier.h \
	wizard/wizard-messages.cpp wizard/wizard-messages.h \
	wizard/wizard-spells.cpp wizard/wizard-spells.h \
	wizard/wizard-spoiler.cpp wizard/wizard-spoiler.h\
	wizard/wizard-special-process.cpp wizard/wizard-special-process.h \
	\
	world/world.cpp world/world.h \
	world/world-movement-processor.cpp world/world-movement-processor.h \
	world/world-turn-processor.cpp world/world-turn-processor.h

EXTRA_hengband_SOURCES = \
	angband.ico angband.rc ang_eng.rc ang_jp.rc maid-x11.cpp main-win.cpp \
	main-win/commandline-win.cpp main-win/commandline-win.h \
	main-win/graphics-win.cpp main-win/graphics-win.h \
	main-win/main-win-bg.cpp main-win/main-win-bg.h \
	main-win/main-win-cfg-reader.cpp main-win/main-win-cfg-reader.h \
	main-win/main-win-define.h \
	main-win/main-win-exception.cpp main-win/main-win-exception.h \
	main-win/main-win-mci.cpp main-win/main-win-mci.h \
	main-win/main-win-menuitem.h \
	main-win/main-win-mmsystem.h \
	main-win/main-win-music.cpp main-win/main-win-music.h \
	main-win/main-win-sound.cpp main-win/main-win-sound.h \
	main-win/main-win-term.cpp main-win/main-win-term.h \
	main-win/main-win-tokenizer.cpp main-win/main-win-tokenizer.h \
	main-win/main-win-utils.cpp main-win/main-win-utils.h \
	main-win/stack-trace-win.cpp \
	main-win/wav-reader.cpp main-win/wav-reader.h \
	test/test-sha256.cpp \
	test/test-probability-table.cpp \
	wall.bmp \
	stdafx.cpp stdafx.h

EXTRA_DIST = \
	gcc-wrap

DEFAULT_INCLUDES = -I$(srcdir) -I$(top_builddir)/src
CPPFLAGS += $(XFT_CFLAGS) $(libcurl_CFLAGS)
if PCH
CPPFLAGS += -include ../src/stdafx.h
endif
LIBS += $(XFT_LIBS) $(libcurl_LIBS)
COMPILE = $(srcdir)/gcc-wrap $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	$(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
CXXCOMPILE = $(srcdir)/gcc-wrap $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	$(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CXXFLAGS)

if PCH
stdafx.h.gch: stdafx.h stdafx.cpp Makefile
	$(CXX) -x c++-header $(CXXFLAGS) $(srcdir)/stdafx.cpp -o $@
	if test none != "$(PCH_CHECKSUMMER)" ; then \
		rm -f "$@".sum ; \
		$(PCH_CHECKSUMMER) $@ > "$@".sum ; \
	fi

$(hengband_SOURCES:.cpp=.$(OBJEXT)): stdafx.h.gch
endif

install-exec-hook:
if SET_GID
	chgrp "@GAMEGROUP@" "$(DESTDIR)$(bindir)/hengband"
	chmod g+s "$(DESTDIR)$(bindir)/hengband"
endif

clean-local:
	-rm stdafx.h.gch stdafx.h.gch.sum
	-rm -rf ../build
