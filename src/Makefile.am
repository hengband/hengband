## Makefile.am -- Process this file with automake to process Makefile.in

AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = hengband

hengband_SOURCES = \
	artifact.c artifact.h \
	\
	angband.h autopick.c \
	\
	avatar.h avatar.c \
	\
	birth.c bldg.c chest.c chest.h chuukei.c \
	cmd2.c cmd4.c cmd-activate.c cmd-activate.h cmd-eat.c cmd-eat.h \
	cmd-item.c cmd-item.h cmd-magiceat.c cmd-magiceat.h \
	cmd-mane.c cmd-mane.h cmd-pet.c cmd-pet.h \
	cmd-quaff.c cmd-quaff.h cmd-read.c cmd-read.h cmd-spell.c cmd-spell.h \
	cmd-usestaff.c cmd-usestaff.h cmd-zaprod.c cmd-zaprod.h cmd-zapwand.c \
	cmd-zapwand.h cmd-smith.c cmd-smith.h cmd-hissatsu.c cmd-hissatsu.h\
	\
	defines.h dungeon.c effects.c externs.h files.c flavor.c \
	\
	floor.h floor-events.c floor-events.h floor-generate.c floor-generate.h \
	floor-save.h floor-save.c \
	floor-streams.c floor-streams.h \
	\
	feature.c feature.h \
	geometry.c geometry.h \
	\
	gameoption.c gameoption.h gamevalue.h \
	generate.h grid.c grid.h h-basic.h h-config.h h-define.h \
	history.h history.c \
	h-system.h h-type.h inet.c init1.c init2.c init.h japanese.c load.c \
	main.c main-x11.c main-gcu.c \
	melee.h melee1.c mind.c \
	\
	monster.h \
	monsterrace-hook.c monsterrace-hook.h monster-process.c \
	monster-process.h monster-status.c monster-status.h monster1.c \
	monster2.c mspells1.c mspells2.c mspells3.c mspells4.c monster-spell.h \
	\
	mutation.h mutation.c \
	\
	object-boost.c object-boost.h \
	object-curse.c object-curse.h object-broken.c object-broken.h \
	object-hook.c object-hook.h object1.c object2.c \
	objectkind-hook.c objectkind-hook.h \
	\
	patron.h patron.c \
	\
	projection.h projection.c\
	\
	player-move.c player-move.h player-damage.c player-damage.h player-status.c player-status.h \
	\
	quest.h quest.c\
	\
	racial.c \
	realm-arcane.c \
	realm-arcane.h realm-chaos.c realm-chaos.h realm-craft.c realm-craft.h \
	realm-crusade.c realm-crusade.h realm-daemon.c realm-daemon.h realm-death.c \
	realm-death.h realm-hex.c realm-hex.h realm-hissatsu.c realm-hissatsu.h \
	realm-life.c realm-life.h realm-nature.c realm-nature.h realm-song.c \
	realm-song.h realm-sorcery.c realm-sorcery.h realm-trump.c realm-trump.h \
	report.c rooms.c rooms-city.c rooms-city.h rooms-fractal.c rooms-fractal.h \
	rooms.h rooms-normal.c rooms-normal.h rooms-pitnest.c rooms-pitnest.h \
	rooms-special.c rooms-special.h rooms-trap.c rooms-trap.h rooms-vault.c \
	rooms-vault.h save.c scores.c selfinfo.c selfinfo.h shoot.c shoot.h snipe.c \
	\
	rumor.h rumor.c \
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
	sort.c sort.h \
	store.h store.c tables.c term.c term.h trap.c trap.h types.h util.c \
	variable.c wild.h wild.c wizard1.c wizard2.c \
	warning.c warning.h world.c world.h \
	\
	view-mainwindow.c\
	\
	xtra2.c z-config.h \
	z-form.c z-form.h z-rand.c z-rand.h z-term.c z-term.h z-util.c z-util.h \
	z-virt.c z-virt.h 

EXTRA_hengband_SOURCES = \
	angband.ico angband.rc ang_eng.rc maid-x11.c main-mac.c main-win.c \
	main-dos.c main-ibm.c \
	makefile.bcc makefile.std makefile.dos makefile.ibm \
	readdib.c wall.bmp

COMPILE = ./gcc-wrap $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)

install-exec-hook:
if SET_GID
	chgrp "@GAMEGROUP@" "$(DESTDIR)$(bindir)/hengband"
	chmod g+s "$(DESTDIR)$(bindir)/hengband"
endif
