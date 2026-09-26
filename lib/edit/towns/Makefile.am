## Makefile.am -- Process this file with automake to process Makefile.in

angband_files = \
	01_Outpost_Full.jsonc \
	01_Outpost_Lite.jsonc \
	01_Outpost_OnlyAngband.jsonc \
	02_Telmora.jsonc \
	03_Morivant.jsonc \
	04_Angwil.jsonc \
	05_Zul.jsonc \
	06_Rlyeh_Obsoleted.jsonc

EXTRA_DIST = \
	$(angband_files)

if SET_GID
angbanddir = @DEFAULT_LIB_PATH@/edit/towns

angband_DATA = \
	$(angband_files)
endif
