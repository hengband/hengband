## Makefile.am -- Process this file with automake to process Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = apex bone data edit file help info pref save script user

## lib/xtra はサブモジュールで、無くてもビルドできるようにここで扱う (インストールは HAVE_XTRA のときだけ行う)。
## 配布物には必ず含めるため EXTRA_DIST は条件なしとし、サブモジュールが無いと make dist は失敗する。

xtra_graf_files = \
	xtra/graf/8x8.bmp

xtra_music_files = \
	xtra/music/delete.me \
	xtra/music/music.cfg

xtra_sound_files = \
	xtra/sound/readme.txt \
	xtra/sound/sound.cfg \
	xtra/sound/se_maoudamashii_voice_monster01.wav \
	xtra/sound/apple1-r.wav \
	xtra/sound/decision25r.wav \
	xtra/sound/drop1.wav \
	xtra/sound/enemy-advent1-r.wav \
	xtra/sound/sea-lion2-r.wav \
	xtra/sound/se_maoudamashii_battle07.wav \
	xtra/sound/se_maoudamashii_retro22.wav \
	xtra/sound/se_maoudamashii_se_drink01.wav \
	xtra/sound/sword-clash1-r.wav \
	xtra/sound/sword-gesture3-r.wav \
	xtra/sound/sword-slash3-r.wav

EXTRA_DIST = \
	$(xtra_graf_files) \
	$(xtra_music_files) \
	$(xtra_sound_files)

if SET_GID
if HAVE_XTRA
xtragrafdir = @DEFAULT_LIB_PATH@/xtra/graf

xtragraf_DATA = \
	$(xtra_graf_files)
endif
endif
