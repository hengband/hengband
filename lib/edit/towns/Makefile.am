## Makefile.am -- Process this file with automake to process Makefile.in

angband_files = \
	01_Outpost_Full.txt \
	01_Outpost_Lite.txt \
	01_Outpost_OnlyAngband.txt \
	02_Telmora.txt \
	03_Morivant.txt \
	04_Angwil.txt \
	05_Zul.txt \
	06_Rlyeh_Obsoleted.txt

EXTRA_DIST = \
	$(angband_files)

if SET_GID
angbanddir = @DEFAULT_LIB_PATH@/edit/towns

angband_DATA = \
	$(angband_files)
endif
