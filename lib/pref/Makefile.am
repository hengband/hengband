## Makefile.am -- Process this file with automake to process Makefile.in

angband_files = \
	picktype.prf pickpref.prf \
	colors.prf \
	font-ami.prf font-dos.prf font-ibm.prf font-mac.prf \
	font-mon.prf font-win.prf font-x11.prf font-xxx.prf \
	font.prf graf-ami.prf graf-dos.prf graf-gcu.prf \
	graf-ibm.prf graf-mac.prf graf-new.prf graf-win.prf \
	graf-x11.prf graf-xaw.prf graf-xxx.prf graf.prf \
	pref-acn.prf pref-ami.prf pref-emx.prf pref-gcu.prf \
	pref-key.prf pref-mac.prf pref-opt.prf pref-win.prf \
	pref-x11.prf pref.prf proxy.prf spell-xx.prf \
	user-win.prf user.prf xtra-gcu.prf \
	xtra-new.prf xtra-xxx.prf

EXTRA_DIST = \
  $(angband_files)

if SET_GID
angbanddir = @DEFAULT_PATH@pref

angband_DATA = \
  $(angband_files)
endif

