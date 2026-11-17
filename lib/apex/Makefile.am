## Makefile.am -- Process this file with automake to process Makefile.in

angband_files = \
	h_scores.json readme.txt

EXTRA_DIST = \
  $(angband_files)

if SET_GID
angbanddir = @DEFAULT_VAR_PATH@/apex

angband_DATA = \
  $(angband_files)
endif

install-data-hook:
if SET_GID
	touch "$(DESTDIR)$(angbanddir)/scores.json"
	chown -R root.@GAMEGROUP@ "$(DESTDIR)$(angbanddir)"
	chmod -R g+w "$(DESTDIR)$(angbanddir)"
endif
