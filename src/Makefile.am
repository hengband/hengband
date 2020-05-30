## Makefile.am -- Process this file with automake to process Makefile.in

AUTOMAKE_OPTIONS = foreign subdir-objects

bin_PROGRAMS = hengband

hengband_SOURCES = \
	system/angband.h system/angband-version.h \
	system/h-basic.h system/h-config.h \
	system/h-define.h system/h-system.h system/h-type.h \
	system/system-variables.c system/system-variables.h \
	system/gamevalue.h \
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
	store/store.c store/store.h \
	store/black-market.c store/black-market.h \
	store/rumor.c store/rumor.h \
	store/say-comments.c store/say-comments.h \
	store/store-util.c store/store-util.h \
	store/store-owners.c store/store-owners.h \
	store/store-owner-comments.c store/store-owner-comments.h \
	\
	view/display-characteristic.c view/display-characteristic.h \
	view/display-fruit.c view/display-fruit.h \
	view/display-player-stat-info.c view/display-player-stat-info.h \
	view/display-player-misc-info.c view/display-player-misc-info.h \
	view/display-player-middle.c view/display-player-middle.h \
	view/display-player.c view/display-player.h \
	view/status-first-page.c view/status-first-page.h \
	view/display-util.c view/display-util.h \
	view/display-birth.c view/display-birth.h \
	\
	player/avatar.h player/avatar.c \
	player/mimic-info-table.c player/mimic-info-table.h \
	player/process-death.c player/process-death.h \
	player/process-name.c player/process-name.h \
	player/race-info-table.c player/race-info-table.h\
	player/race-resistances.c player/race-resistances.h \
	player/permanent-resistances.c player/permanent-resistances.h \
	player/temporary-resistances.c player/temporary-resistances.h \
	player/digestion-processor.c player/digestion-processor.h \
	\
	io/character-dump.c io/character-dump.h \
	io/dump-remover.c io/dump-remover.h \
	io/files-util.c io/files-util.h \
	io/gf-descriptions.c io/gf-descriptions.h \
	io/targeting.c io/targeting.h \
	io/tokenizer.c io/tokenizer.h \
	io/interpret-pref-file.c io/interpret-pref-file.h \
	io/player-status-dump.c io/player-status-dump.h \
	io/special-class-dump.c io/special-class-dump.h \
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
	dungeon/dungeon.c dungeon/dungeon.h \
	dungeon/dungeon-processor.c dungeon/dungeon-processor.h \
	dungeon/dungeon-file.c dungeon/dungeon-file.h \
	dungeon/quest.h dungeon/quest.c \
	\
	floor/floor.c floor/floor.h floor/floor-events.c floor/floor-events.h \
	floor/floor-generate.c floor/floor-generate.h \
	floor/floor-save.h floor/floor-save.c floor/floor-town.h floor/floor-town.c \
	floor/floor-streams.c floor/floor-streams.h \
	floor/pattern-walk.c floor/pattern-walk.h \
	floor/wild.h floor/wild.c \
	floor/geometry.c floor/geometry.h \
	\
	locale/japanese.c locale/japanese.h locale/english.c locale/english.h \
	\
	grid/feature.c grid/feature.h \
	grid/grid.c grid/grid.h grid/trap.c grid/trap.h \
	\
	main.c main-x11.c main-gcu.c \
	\
	mind/mind.c mind/mind.h \
	mind/racial.c mind/racial.h \
	mind/racial-samurai.c mind/racial-samurai.h \
	mind/racial-mirror-master.c mind/racial-mirror-master.h \
	mind/racial-force-trainer.c mind/racial-force-trainer.h \
	mind/monk-attack.c mind/monk-attack.h \
	mind/surprise-attack.c mind/surprise-attack.h \
	\
	monster/creature.h monster/creature.c \
	monster/monster.h monster/monster-race.c monster/monster-race.h \
	monster/monster-util.c monster/monster-util.h \
	monster/monster-direction.c monster/monster-direction.h \
	monster/monster-attack.c monster/monster-attack.h \
	monster/monster-move.c monster/monster-move.h \
	monster/monster-object.c monster/monster-object.h \
	monster/monster-runaway.c monster/monster-runaway.h \
	monster/monster-safety-hiding.c monster/monster-safety-hiding.h \
	monster/monster-sweep-grid.c monster/monster-sweep-grid.h \
	monster/monster-update.c monster/monster-update.h \
	monster/quantum-effect.c monster/quantum-effect.h \
	monster/monster-race-hook.c monster/monster-race-hook.h \
	monster/monster-dist-offsets.c monster/monster-dist-offsets.h \
	monster/monster-process.c monster/monster-process.h \
	monster/monster-status.c monster/monster-status.h \
	monster/monster1.c monster/monster2.c \
	monster/horror-descriptions.c monster/horror-descriptions.h \
	\
	pet/pet-fall-off.c pet/pet-fall-off.h \
	pet/pet-util.c pet/pet-util.h \
	\
	mutation/mutation.c mutation/mutation.h \
	mutation/mutation-processor.c mutation/mutation-processor.h \
	\
	object/artifact.c object/artifact.h \
	object/object-boost.c object/object-boost.h \
	object/chest.c object/chest.h \
	object/object-curse.c object/object-curse.h \
	object/object-broken.c object/object-broken.h \
	object/object-hook.c object/object-hook.h \
	object/object1.c object/object1.h object/object2.c object/object2.h \
	object/object-kind.c object/object-kind.h \
	object/object-kind-hook.c object/object-kind-hook.h \
	object/lite-processor.c object/lite-processor.h \
	object/object-flavor.c object/object-flavor.h \
	object/object-ego.c object/object-ego.h \
	object/warning.c object/warning.h \
	object/death-scythe.c object/death-scythe.h \
	object/tr-types.h object/trc-types.h object/tval-type.h \
	object/old-ego-extra-values.h object/special-object-flags.h \
	object/object-mark-types.h object/object-util.h \
	object/item-apply-magic.h \
	object/sv-amulet-types.h object/sv-bow-types.h \
	object/sv-digging-types.h object/sv-food-types.h \
	object/sv-weapon-types.h object/sv-protector-types.h object/sv-armor-types.h \
	object/sv-lite-types.h object/sv-potion-types.h object/sv-ring-types.h \
	object/sv-rod-types.h object/sv-scroll-types.h \
	object/sv-staff-types.h object/sv-wand-types.h object/sv-other-types.h \
	object/item-feeling.h object/item-use-flags.h \
	object/torch.c object/torch.h \
	\
	inventory/simple-appraiser.c inventory/simple-appraiser.h \
	inventory/inventory-curse.c inventory/inventory-curse.h \
	inventory/recharge-processor.c inventory/recharge-processor.h \
	inventory/pack-overflow.c inventory/pack-overflow.h \
	inventory/player-inventory.c inventory/player-inventory.h \
	inventory/inventory-damage.c inventory/inventory-damage.h \
	\
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
	realm/realm.c realm/realm.h \
	realm/realm-arcane.c realm/realm-arcane.h \
	realm/realm-chaos.c realm/realm-chaos.h \
	realm/realm-craft.c realm/realm-craft.h \
	realm/realm-crusade.c realm/realm-crusade.h \
	realm/realm-daemon.c realm/realm-daemon.h \
	realm/realm-death.c realm/realm-death.h \
	realm/realm-hex.c realm/realm-hex.h 
	realm/realm-hissatsu.c realm/realm-hissatsu.h \
	realm/realm-life.c realm/realm-life.h 
	realm/realm-nature.c realm/realm-nature.h realm/realm-song.c \
	realm/realm-song.h realm/realm-sorcery.c \
	realm/realm-sorcery.h realm/realm-trump.c realm/realm-trump.h \
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
	effect/effect-feature.c effect/effect-feature.h \
	effect/effect-item.c effect/effect-item.h \
	effect/spells-effect-util.c effect/spells-effect-util.h \
	effect/effect-player.c effect/effect-player.h \
	effect/effect-monster.c effect/effect-monster.h \
	effect/effect-monster-util.c effect/effect-monster-util.h \
	effect/effect-monster-resist-hurt.c effect/effect-monster-resist-hurt.h \
	effect/effect-monster-psi.c effect/effect-monster-psi.h \
	effect/effect-monster-domination.c effect/effect-monster-domination.h \
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
	spell/technic-info-table.c spell/technic-info-table.h \
	spell/spells-execution.c spell/spells-execution.h \
	spell/spells-util.h spell/spells-type.h \
	spell/process-effect.c spell/process-effect.h \
	spell/spells1.c spell/spells1.h spell/spells2.c spell/spells2.h \
	spell/spells3.c spell/spells3.h \
	spell/music-checker.c spell/music-checker.h \
	spell/spells-diceroll.c spell/spells-diceroll.h \
	spell/spells-floor.c spell/spells-floor.h \
	spell/spells-object.c spell/spells-object.h \
	spell/spells-status.c spell/spells-status.h \
	spell/spells-summon.c spell/spells-summon.h \
	spell/spells-world.c spell/spells-world.h \
	spell/spells-detection.c spell/spells-detection.h \
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
	mspell/mspell-status.c mspell/mspell-status.h \
	mspell/mspell-special.c mspell/mspell-special.h \
	mspell/mspell-particularity.c mspell/mspell-particularity.h \
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
	\
	cmd/cmd-autopick.c cmd/cmd-autopick.c \
	cmd/cmd-building.c cmd/cmd-building.h \
	cmd/dump-util.c cmd/dump-util.h cmd/feeling-table.c cmd/feeling-table.h \
	cmd/monster-group-table.c cmd/monster-group-table.h \
	cmd/diary-subtitle-table.c cmd/diary-subtitle-table.h \
	cmd/object-group-table.c cmd/object-group-table.h \
	cmd/cmd-activate.c cmd/cmd-activate.h cmd/cmd-basic.c cmd/cmd-basic.h \
	cmd/cmd-diary.c cmd/cmd-diary.h cmd/cmd-dump.c cmd/cmd-dump.h \
	cmd/cmd-draw.c cmd/cmd-draw.h cmd/cmd-eat.c cmd/cmd-eat.h \
	cmd/cmd-gameoption.c cmd/cmd-gameoption.h cmd/cmd-help.c cmd/cmd-help.h \
	cmd/cmd-hissatsu.c cmd/cmd-hissatsu.h cmd/cmd-inn.c cmd/cmd-inn.h \
	cmd/cmd-item.c cmd/cmd-item.h cmd/cmd-knowledge.c cmd/cmd-knowledge.h \
	cmd/lighting-level-table.c cmd/lighting-level-table.h \
	cmd/cmd-magiceat.c cmd/cmd-magiceat.h cmd/cmd-mane.c cmd/cmd-mane.h \
	cmd/cmd-macro.c cmd/cmd-macro.h cmd/cmd-pet.c cmd/cmd-pet.h \
	cmd/cmd-quaff.c cmd/cmd-quaff.h cmd/cmd-read.c cmd/cmd-read.h \
	cmd/cmd-process-screen.c cmd/cmd-process-screen.h \
	cmd/cmd-save.c cmd/cmd-save.h cmd/cmd-smith.c cmd/cmd-smith.h \
	cmd/cmd-spell.c cmd/cmd-spell.h cmd/cmd-visuals.c cmd/cmd-visuals.h \
	cmd/cmd-usestaff.c cmd/cmd-usestaff.h \
	cmd/cmd-zaprod.c cmd/cmd-zaprod.h cmd/cmd-zapwand.c cmd/cmd-zapwand.h \
	cmd/cmd-menu-content-table.c cmd/cmd-menu-content-table.h \
	cmd/cmd-attack.c cmd/cmd-attack.h \
	\
	combat/player-attack-util.h \
	combat/attack-power-table.c combat/attack-power-table.h \
	combat/aura-counterattack.c combat/aura-counterattack.h \
	combat/hallucination-attacks-table.c combat/hallucination-attacks-table.h \
	combat/monster-attack-types.c combat/monster-attack-types.h \
	combat/martial-arts-table.c combat/martial-arts-table.h \
	combat/insults-moans.c combat/insults-moans.h \
	combat/monster-attack-effects.h \
	combat/attack-accuracy.c combat/attack-accuracy.h \
	combat/slaying.c combat/slaying.h \
	combat/samurai-slaying.c combat/samurai-slaying.h \
	combat/combat-options-type.h \
	combat/attack-criticality.c combat/attack-criticality.h \
	combat/player-attack.c combat/player-attack.h \
	combat/blood-sucking-processor.c combat/blood-sucking-processor.h \
	combat/vorpal-weapon.c combat/vorpal-weapon.h \
	combat/attack-chaos-effect.c combat/attack-chaos-effect.h \
	combat/monster-attack-player.c combat/monster-attack-player.h \
	combat/monster-attack-monster.c combat/monster-attack-monster.h \
	combat/melee-postprocess.c combat/melee-postprocess.h \
	combat/melee-switcher.c combat/melee-switcher.h \
	combat/melee-util.c combat/melee-util.h \
	combat/monster-attack-util.c combat/monster-attack-util.h \
	combat/monster-attack-describer.c combat/monster-attack-describer.h \
	combat/monster-eating.c combat/monster-eating.h \
	combat/monster-attack-status.c combat/monster-attack-status.h \
	combat/monster-attack-switcher.c combat/monster-attack-switcher.h \
	combat/shoot.c combat/shoot.h \
	combat/snipe.c combat/snipe.h \
	\
	term/gameterm.c term/gameterm.h \
	term/z-form.c term/z-form.h term/z-rand.c term/z-rand.h \
	term/z-term.c term/z-term.h term/z-util.c term/z-util.h \
	term/z-virt.c term/z-virt.h \
	\
	util/util.c util/util.h \
	\
	wizard/wizard-spoiler.c wizard/wizard-spoiler.h\
	wizard/wizard-special-process.c wizard/wizard-special-process.h \
	\
	world/world.c world/world.h \
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
	-Iautopick -Ibirth -Icmd -Icombat -Icore -Idungeon -Ieffect -Ifloor \
	-Igrid -Iinventory -Iio -Iknowledge -Imarket -Imind -Imonster -Imspell \
	-Imutation -Iobject -Ipet -Iplayer -Irealm -Iroom -Ispell -Isystem -Iterm \
	-Iutil -Iview -Iwizard -Iworld

install-exec-hook:
if SET_GID
	chgrp "@GAMEGROUP@" "$(DESTDIR)$(bindir)/hengband"
	chmod g+s "$(DESTDIR)$(bindir)/hengband"
endif
