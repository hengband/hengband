## Makefile.am -- Process this file with automake to process Makefile.in

AUTOMAKE_OPTIONS = foreign subdir-objects

bin_PROGRAMS = hengband

hengband_SOURCES = \
	artifact.c artifact.h \
	\
	angband.h\
	\
	autopick/autopick.c autopick/autopick.h \
	autopick/autopick-commands-table.h autopick/autopick-dirty-flags.h \
	autopick/autopick-editor-table.h autopick/autopick-flags-table.h \
	autopick/autopick-key-flag-process.h autopick/autopick-key-table.h \
	autopick/autopick-menu-data-table.c autopick/autopick-menu-data-table.h \
	autopick/autopick-methods-table.h \
	autopick/autopick-util.c autopick/autopick-util.h \
	autopick/autopick-entry.c autopick/autopick-entry.h \
	autopick/autopick-initializer.c autopick/autopick-initializer.h \
	autopick/autopick-matcher.c autopick/autopick-matcher.h \
	autopick-describer.c autopick-describer.h \
	autopick-destroyer.c autopick-destroyer.h \
	autopick/autopick-reader-writer.c autopick/autopick-reader-writer.h \
	autopick/autopick-finder.c autopick/autopick-finder.h \
	autopick/autopick-pref-processor.c autopick/autopick-pref-processor.h \
	autopick/autopick-drawer.c autopick/autopick-drawer.h \
	autopick/autopick-inserter-killer.c autopick/autopick-inserter-killer.h \
	autopick/autopick-registry.c autopick/autopick-registry.h \
	autopick/autopick-command-menu.c autopick/autopick-command-menu.h \
	autopick-editor-util.c autopick-editor-util.h \
	autopick-editor-command.c autopick-editor-command.h \
	\
	avatar.h avatar.c birth.c birth.h \
	\
	main/music-definitions-table.c main/music-definitions-table.h \
	main/sound-definitions-table.c main/sound-definitions-table.h \
	market/arena-info-table.c market/arena-info-table.h \
	market/building.c market/building.h \
	market/building-util.c market/building-util.h \
	market/play-gamble.c market/play-gamble.h \
	market/poker.c market/poker.h market/store-owners.c market/store-owners.h \
	market/store-owner-comments.c market/store-owner-comments.h \
	market/articles-on-sale.c market/articles-on-sale.h \
	market/black-market.c market/black-market.h \
	market/say-comments.c market/say-comments.h \
	market/store-util.c market/store-util.h \
	market/gold-magnification-table.c market/gold-magnification-table.h \
	market/store.c market/store.h \
	chest.c chest.h chuukei.c chuukei.h \
	\
	view/display-characteristic.c view/display-characteristic.h \
	view/display-fruit.c view/display-fruit.h \
	view/display-player-stat-info.c view/display-player-stat-info.h \
	view/display-player-misc-info.c view/display-player-misc-info.h \
	view/display-player-middle.c view/display-player-middle.h \
	view/display-player.c view/display-player.h \
	view/status-first-page.c view/status-first-page.h \
	view/display-util.c view/display-util.h \
	\
	player/mimic-info-table.c player/mimic-info-table.h \
	player/process-death.c player/process-death.h \
	player/process-name.c player/process-name.h \
	player/race-info-table.c player/race-info-table.h\
	player/race-resistances.c player/race-resistances.h \
	player/permanent-resistances.c player/permanent-resistances.h \
	player/temporary-resistances.c player/temporary-resistances.h \
	\
	io/dump-remover.c io/dump-remover.h \
	io/gf-descriptions.c io/gf-descriptions.h \
	io/tokenizer.c io/tokenizer.h \
	io/interpret-pref-file.c io/interpret-pref-file.h \
	io/player-status-dump.c io/player-status-dump.h \
	io/special-class-dump.c io/special-class-dump.h \
	io/read-pref-file.c io/read-pref-file.h \
	io/exit-panic.c io/exit-panic.h \
	io/write-diary.c io/write-diary.h \
	io/mutations-dump.c io/mutations-dump.h \
	io/signal-handlers.c io/signal-handlers.h \
	io/uid-checker.c io/uid-checker.h \
	character-dump.c character-dump.h core.c core.h files.c files.h \
	core/show-file.c core/show-file.h \
	\
	dungeon.c dungeon.h \
	floor.c floor.h floor-events.c floor-events.h \
	floor-generate.c floor-generate.h \
	floor-save.h floor-save.c floor-town.h floor-town.c \
	floor-streams.c floor-streams.h \
	\
	feature.c feature.h \
	geometry.c geometry.h \
	\
	gamevalue.h \
	grid.c grid.h h-basic.h h-config.h h-define.h \
	history.h history.c \
	h-system.h h-type.h inet.c inet.h init.c init.h \
	japanese.c japanese.h english.c english.h \
	load.c load.h \
	dungeon-file.c dungeon-file.h \
	main.c main-x11.c main-gcu.c \
	mind.c mind.h \
	\
	monster.h monsterrace.c monsterrace.h \
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
	monsterrace-hook.c monsterrace-hook.h \
	monster-dist-offsets.c monster-dist-offsets.h \
	monster-process.c monster-process.h \
	monster-status.c monster-status.h monster1.c monster2.c \
	mspells1.c mspells2.c mspells3.c mspells4.c monster-spell.h \
	\
	mutation.h mutation.c \
	\
	object-boost.c object-boost.h \
	object-curse.c object-curse.h object-broken.c object-broken.h \
	object-hook.c object-hook.h object1.c object2.c object.h \
	object/object-kind.c object/object-kind.h \
	object/object-kind-hook.c object/object-kind-hook.h \
	object-flavor.c object-flavor.h object-ego.c object-ego.h \
	\
	patron.h patron.c \
	\
	creature.h creature.c \
	player-move.c player-move.h player-damage.c player-damage.h \
	horror-descriptions.c horror-descriptions.h \
	player-status.c player-status.h player-effects.c player-effects.h \
	player-skill.c player-skill.h player-class.c player-class.h \
	player-race.c player-race.h player-personality.c player-personality.h \
	player-sex.c player-sex.h player-inventory.c player-inventory.h \
	\
	quest.h quest.c\
	\
	racial.c racial.h \
	realm.c realm.h \
	realm-arcane.c realm-arcane.h realm-chaos.c realm-chaos.h \
	realm-craft.c realm-craft.h realm-crusade.c realm-crusade.h \
	realm-daemon.c realm-daemon.h realm-death.c realm-death.h \
	realm-hex.c realm-hex.h realm-hissatsu.c realm-hissatsu.h \
	realm-life.c realm-life.h realm-nature.c realm-nature.h realm-song.c \
	realm-song.h realm-sorcery.c realm-sorcery.h realm-trump.c realm-trump.h \
	report.c report.h \
	rooms.c rooms-city.c rooms-city.h rooms-fractal.c rooms-fractal.h \
	rooms.h rooms-normal.c rooms-normal.h \
	rooms-special.c rooms-special.h rooms-trap.c rooms-trap.h \
	rooms-vault.c rooms-vault.h \
	room/rooms-pit-nest.c room/rooms-pit-nest.h \
	room/pit-nest-kinds-table.c room/pit-nest-kinds-table.h \
	save.c save.h scores.c scores.h selfinfo.c selfinfo.h \
	snipe.c snipe.h \
	\
	rumor.h rumor.c \
	\
	effect/effect-feature.c effect/effect-feature.h \
	effect/effect-item.c effect/effect-item.h \
	\
	spells.h \
	spells1.c spells2.c spells3.c \
	spells-diceroll.c spells-diceroll.h \
	spells-floor.c spells-floor.h \
	spells-object.c spells-object.h \
	spells-status.c spells-status.h \
	spells-summon.c spells-summon.h \
	spells-world.c spells-world.h \
	\
	sort.c sort.h gameterm.c gameterm.h trap.c trap.h util.c util.h \
	wild.h wild.c wizard1.c wizard2.c \
	warning.c warning.h world.c world.h \
	\
	view-mainwindow.c view-mainwindow.h targeting.c targeting.h \
	\
	z-form.c z-form.h z-rand.c z-rand.h z-term.c z-term.h z-util.c z-util.h \
	z-virt.c z-virt.h \
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
	\
	combat/melee.h combat/melee.c combat/shoot.c combat/shoot.h

EXTRA_hengband_SOURCES = \
	angband.ico angband.rc ang_eng.rc maid-x11.c main-win.c \
	makefile.bcc makefile.std readdib.c readdib.h wall.bmp \

EXTRA_DIST = \
	gcc-wrap

CFLAGS += $(XFT_CFLAGS)
LIBS += $(XFT_LIBS)
COMPILE = $(srcdir)/gcc-wrap $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	$(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) \
	-Iautopick -Icmd -Icombat -Icore -Ieffect -Iio -Iknowledge -Imarket \
	-Imonster -Iobject -Iplayer -Iroom -Iview

install-exec-hook:
if SET_GID
	chgrp "@GAMEGROUP@" "$(DESTDIR)$(bindir)/hengband"
	chmod g+s "$(DESTDIR)$(bindir)/hengband"
endif
