## Makefile.am -- Process this file with automake to process Makefile.in

AUTOMAKE_OPTIONS = foreign

visual_studio_files = \
	VisualStudio/Hengband.sln \
	VisualStudio/Hengband/Hengband.vcxproj \
	VisualStudio/Hengband/Hengband.vcxproj.filters \
	VisualStudio/Hengband/packages.config

visual_studio_libcurl_files = \
	VisualStudio/Hengband/libcurl/include/curl/curl.h \
	VisualStudio/Hengband/libcurl/include/curl/curlver.h \
	VisualStudio/Hengband/libcurl/include/curl/easy.h \
	VisualStudio/Hengband/libcurl/include/curl/header.h \
	VisualStudio/Hengband/libcurl/include/curl/mprintf.h \
	VisualStudio/Hengband/libcurl/include/curl/multi.h \
	VisualStudio/Hengband/libcurl/include/curl/options.h \
	VisualStudio/Hengband/libcurl/include/curl/stdcheaders.h \
	VisualStudio/Hengband/libcurl/include/curl/system.h \
	VisualStudio/Hengband/libcurl/include/curl/typecheck-gcc.h \
	VisualStudio/Hengband/libcurl/include/curl/urlapi.h \
	VisualStudio/Hengband/libcurl/include/curl/websockets.h \
	VisualStudio/Hengband/libcurl/lib/libcurl_a.lib \
	VisualStudio/Hengband/libcurl/lib/libcurl_a_debug.lib

schema_files = \
	schema/ArtifactDefinitions.schema.json \
	schema/BaseitemDefinitions.schema.json \
	schema/ClassMagicDefinitions.schema.json \
	schema/MonraceDefinitions.schema.json \
	schema/SpellDefinitions.schema.json

EXTRA_DIST = \
	autopick.txt \
	autopick_eng.txt \
	bootstrap \
	readme.md \
	readme_angband \
	readme-eng.md \
	THIRD-PARTY-NOTICES.txt \
	hengband.spec \
	$(visual_studio_files) \
	$(visual_studio_libcurl_files) \
	$(schema_files)

SUBDIRS = src lib
