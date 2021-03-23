## Makefile.am -- Process this file with automake to process Makefile.in

AUTOMAKE_OPTIONS = foreign

visual_studio_files = \
	Hengband/Hengband.sln \
	Hengband/Hengband/curl/include/curl/curl.h \
	Hengband/Hengband/curl/include/curl/curlver.h \
	Hengband/Hengband/curl/include/curl/easy.h \
	Hengband/Hengband/curl/include/curl/mprintf.h \
	Hengband/Hengband/curl/include/curl/multi.h \
	Hengband/Hengband/curl/include/curl/options.h \
	Hengband/Hengband/curl/include/curl/stdcheaders.h \
	Hengband/Hengband/curl/include/curl/system.h \
	Hengband/Hengband/curl/include/curl/typecheck-gcc.h \
	Hengband/Hengband/curl/include/curl/urlapi.h \
	Hengband/Hengband/Hengband.vcxproj \
	Hengband/Hengband/Hengband.vcxproj.filters \
	Hengband/Hengband/packages.config

EXTRA_DIST = \
	autopick.txt \
	autopick_eng.txt \
	bootstrap \
	readme.md \
	readme_angband \
	readme-eng.md \
	hengband.spec \
	$(visual_studio_files)

SUBDIRS = src lib

# Handle some paths in Hengband/Hengband that have spaces in them and would
# be mishandled by make/automake.
dist-hook:
	(cd $(srcdir)/Hengband/Hengband/curl && tar -cf - "x86 Debug" "x86 Release" ) | (cd $(distdir)/Hengband/Hengband/curl && tar -xf -)
