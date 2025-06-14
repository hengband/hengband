## Makefile.am -- Process this file with automake to process Makefile.in

AUTOMAKE_OPTIONS = foreign

angband_files = \
	a_cursed_j.txt a_high_j.txt a_low_j.txt a_med_j.txt \
	aname_j.txt chainswd_j.txt dead_j.txt \
	death_j.txt elvish_j.txt error_j.txt news_j.txt \
	rumors_j.txt seppuku_j.txt silly_j.txt timefun_j.txt \
	timenorm_j.txt w_cursed_j.txt w_high_j.txt w_low_j.txt \
	w_med_j.txt \
	a_cursed.txt a_high.txt a_low.txt a_med.txt \
	chainswd.txt dead.txt \
	death.txt elvish.txt error.txt news.txt \
	rumors.txt seppuku.txt silly.txt sname.txt timefun.txt \
	timenorm.txt w_cursed.txt w_high.txt w_low.txt\
	w_med.txt

EXTRA_DIST = \
  $(angband_files)

SUBDIRS = books

if SET_GID
angbanddir = @DEFAULT_LIB_PATH@/file

angband_DATA = \
  $(angband_files)
endif

