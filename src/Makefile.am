## Makefile.am -- Process this file with automake to process Makefile.in

AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = hengband

hengband_SOURCES = \
	z-util.c z-virt.c z-form.c z-rand.c z-term.c \
	variable.c tables.c util.c cave.c \
	kajitips.h object1.c object2.c monster1.c monster2.c \
	xtra1.c xtra2.c spells1.c spells2.c \
	melee1.c melee2.c save.c files.c floors.c\
	cmd1.c cmd2.c cmd3.c cmd4.c cmd5.c cmd6.c do-spell.c\
	store.c birth.c load.c \
	wizard1.c wizard2.c grid.c streams.c rooms.c \
	generate.c dungeon.c init1.c init2.c \
	effects.c racial.c inet.c report.c snipe.c hex.c \
	artifact.c autopick.c mutation.c flavor.c spells3.c \
	mspells1.c mspells2.c scores.c mind.c mane.c hissatsu.c \
	bldg.c obj_kind.c wild.c avatar.c japanese.c mspells3.c \
	main-cap.c main-gcu.c main-x11.c main-xaw.c main.c chuukei.c \
	angband.h defines.h externs.h generate.h grid.h \
	h-basic.h h-config.h h-define.h h-system.h h-type.h \
	mindtips.h readdib.h rooms.h streams.h init.h \
	types.h z-config.h z-form.h z-rand.h z-term.h z-util.h z-virt.h

EXTRA_hengband_SOURCES = \
	angband.ico angband.rc ang_eng.rc maid-x11.c main-mac.c main-win.c \
	main-dos.c main-ibm.c \
	makefile.bcc makefile.std makefile.dos makefile.ibm \
	readdib.c wall.bmp

install-exec-hook:
if SET_GID
	chgrp "@GAMEGROUP@" "$(DESTDIR)$(bindir)/hengband"
	chmod g+s "$(DESTDIR)$(bindir)/hengband"
endif
