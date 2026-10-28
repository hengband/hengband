## Makefile.am -- Process this file with automake to process Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = apex bone data edit file help info pref save script user

## lib/xtra はサブモジュールで、無くてもビルドできるようにここで扱う (インストールは HAVE_XTRA のときだけ行う)。
## 配布物には必ず含めるため EXTRA_DIST は条件なしとし、サブモジュールが無いと make dist は失敗する。

xtra_graf_files = \
	xtra/graf/8x8.bmp

## BGM (xtra/music/*.mp3) は容量が大きいため配布物に含めず、music.cfg だけを含める。
## mp3 は hengband.xtra から別途入手する。
xtra_music_files = \
	xtra/music/delete.me \
	xtra/music/music.cfg

xtra_sound_files = \
	xtra/sound/readme.txt \
	xtra/sound/sound.cfg \
	xtra/sound/apple1-r.wav \
	xtra/sound/bite.wav \
	xtra/sound/breath.wav \
	xtra/sound/buy.wav \
	xtra/sound/claw.wav \
	xtra/sound/critical.wav \
	xtra/sound/crush.wav \
	xtra/sound/damage_over_time.wav \
	xtra/sound/decision25r.wav \
	xtra/sound/destitem.wav \
	xtra/sound/down.wav \
	xtra/sound/drop1.wav \
	xtra/sound/drop2.wav \
	xtra/sound/eat1.wav \
	xtra/sound/eat2.wav \
	xtra/sound/eat3.wav \
	xtra/sound/enemy-advent1-r.wav \
	xtra/sound/evil.wav \
	xtra/sound/explode.wav \
	xtra/sound/fail.wav \
	xtra/sound/fall.wav \
	xtra/sound/flee.wav \
	xtra/sound/glass.wav \
	xtra/sound/hit1.wav \
	xtra/sound/hit2.wav \
	xtra/sound/hit3.wav \
	xtra/sound/hitwall1.wav \
	xtra/sound/hitwall2.wav \
	xtra/sound/hitwall3.wav \
	xtra/sound/hungry1.wav \
	xtra/sound/hungry2.wav \
	xtra/sound/kill1.wav \
	xtra/sound/kill2.wav \
	xtra/sound/level.wav \
	xtra/sound/miss1.wav \
	xtra/sound/miss2.wav \
	xtra/sound/miss3.wav \
	xtra/sound/moan.wav \
	xtra/sound/n_kill.wav \
	xtra/sound/opendoor1.wav \
	xtra/sound/opendoor2.wav \
	xtra/sound/opendoor3.wav \
	xtra/sound/pain.wav \
	xtra/sound/quaff.wav \
	xtra/sound/reflect.wav \
	xtra/sound/scroll.wav \
	xtra/sound/se_maoudamashii_battle07.wav \
	xtra/sound/se_maoudamashii_retro22.wav \
	xtra/sound/se_maoudamashii_se_drink01.wav \
	xtra/sound/se_maoudamashii_voice_monster01.wav \
	xtra/sound/sea-lion2-r.wav \
	xtra/sound/sell.wav \
	xtra/sound/shoot.wav \
	xtra/sound/shoot_hit1.wav \
	xtra/sound/shoot_hit2.wav \
	xtra/sound/show.wav \
	xtra/sound/shutdoor1.wav \
	xtra/sound/shutdoor2.wav \
	xtra/sound/shutdoor3.wav \
	xtra/sound/slash.wav \
	xtra/sound/slime.wav \
	xtra/sound/stairway1.wav \
	xtra/sound/stairway2.wav \
	xtra/sound/sting.wav \
	xtra/sound/store1.wav \
	xtra/sound/store2.wav \
	xtra/sound/study.wav \
	xtra/sound/summon.wav \
	xtra/sound/sword-clash1-r.wav \
	xtra/sound/sword-gesture3-r.wav \
	xtra/sound/sword-slash3-r.wav \
	xtra/sound/take_off.wav \
	xtra/sound/teleport.wav \
	xtra/sound/terrain_damage.wav \
	xtra/sound/touch.wav \
	xtra/sound/tplevel.wav \
	xtra/sound/tpother.wav \
	xtra/sound/wail.wav \
	xtra/sound/warn.wav \
	xtra/sound/wield.wav \
	xtra/sound/zap.wav

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
