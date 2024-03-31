## Makefile.am -- Process this file with automake to process Makefile.in

AUTOMAKE_OPTIONS = foreign

visual_studio_files = \
	Hengband/Hengband.sln \
	Hengband/Hengband/curl/include/curl/curl.h \
	Hengband/Hengband/curl/include/curl/curlver.h \
	Hengband/Hengband/curl/include/curl/easy.h \
	Hengband/Hengband/curl/include/curl/header.h \
	Hengband/Hengband/curl/include/curl/mprintf.h \
	Hengband/Hengband/curl/include/curl/multi.h \
	Hengband/Hengband/curl/include/curl/options.h \
	Hengband/Hengband/curl/include/curl/stdcheaders.h \
	Hengband/Hengband/curl/include/curl/system.h \
	Hengband/Hengband/curl/include/curl/typecheck-gcc.h \
	Hengband/Hengband/curl/include/curl/urlapi.h \
	Hengband/Hengband/curl/include/curl/websockets.h \
	Hengband/Hengband/curl/lib/libcurl_a.lib \
	Hengband/Hengband/curl/lib/libcurl_a_debug.lib \
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
	THIRD-PARTY-NOTICES.txt \
	hengband.spec \
	$(visual_studio_files)

SUBDIRS = src lib
