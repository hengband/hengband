
#include "angband.h"
#include "util.h"

#include "dungeon-file.h"
#include "feature.h"
#include "grid.h"
#include "quest.h"
#include "monster.h"
#include "files.h"
#include "player-skill.h"
#include "dungeon.h"
#include "floor.h"
#include "bldg.h"
#include "world.h"
#include "rooms-vault.h"

dungeon_grid letter[255];

/*!
 * @file init1.c
 * @brief ゲームデータ初期化1 / Initialization (part 1) -BEN-
 * @date 2014/01/28
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.\n
 * </pre>
 * @details
 * <pre>
 * This file is used to initialize various variables and arrays for the
 * Angband game.  Note the use of "fd_read()" and "fd_write()" to bypass
 * the common limitation of "read()" and "write()" to only 32767 bytes
 * at a time.
 * Several of the arrays for Angband are built from "template" files in
 * the "lib/file" directory, from which quick-load binary "image" files
 * are constructed whenever they are not present in the "lib/data"
 * directory, or if those files become obsolete, if we are allowed.
 * Warning -- the "ascii" file parsers use a minor hack to collect the
 * name and text information in a single pass.  Thus, the game will not
 * be able to load any template file with more than 20K of names or 60K
 * of text, even though technically, up to 64K should be legal.
 *
 * Note that if "ALLOW_TEMPLATES" is not defined, then a lot of the code
 * in this file is compiled out, and the game will not run unless valid
 * "binary template files" already exist in "lib/data".  Thus, one can
 * compile Angband with ALLOW_TEMPLATES defined, run once to create the
 * "*.raw" files in "lib/data", and then quit, and recompile without
 * defining ALLOW_TEMPLATES, which will both save 20K and prevent people
 * from changing the ascii template files in potentially dangerous ways.
 * ノートとして記録：ALLOW_TEMPLATESが定義されていない場合、
 * このファイルのコードの多くはコンパイル出力され、本ゲームは
 * 正規の「バイナリテンプレートファイル」がlib/dataファイル内に
 * 存在しない限り起動しなくなる。一方ALLOW_TEMPLATESが定義されている
 * ならば１度ゲームが起動するごとに*.rawファイル群が作成され、終了時には
 * ALLOW_TEMPLATEの定義に関係なくリコンパイルされる。これにより20K(バイト？)
 * のデータが保存され、プレイヤーが潜在的に危険な方法でascii文字の
 * テンプレートファイルを変更することを妨げることができる。
 * The code could actually be removed and placed into a "stand-alone"
 * program, but that feels a little silly, especially considering some
 * of the platforms that we currently support.
 * </pre>
 */

#include "angband.h"
#include "dungeon-file.h"
#include "trap.h"
#include "grid.h"
#include "monster.h"
#include "artifact.h"
#include "cmd-activate.h"
#include "term.h"
#include "wild.h"
#include "quest.h"
#include "view-mainwindow.h"



#ifdef ALLOW_TEMPLATES

#include "init.h"


 /*** Helper arrays for parsing ascii template files ***/

 /*!
  * モンスターの打撃手段トークンの定義 /
  * Monster Blow Methods
  */
static concptr r_info_blow_method[] =
{
	"",
	"HIT",
	"TOUCH",
	"PUNCH",
	"KICK",
	"CLAW",
	"BITE",
	"STING",
	"SLASH",
	"BUTT",
	"CRUSH",
	"ENGULF",
	"CHARGE",
	"CRAWL",
	"DROOL",
	"SPIT",
	"EXPLODE",
	"GAZE",
	"WAIL",
	"SPORE",
	"XXX4",
	"BEG",
	"INSULT",
	"MOAN",
	"SHOW",
	"SHOOT",
	NULL
};


/*!
 * モンスターの打撃属性トークンの定義 /
 * Monster Blow Effects
 */
static concptr r_info_blow_effect[] =
{
	"",
	"HURT",
	"POISON",
	"UN_BONUS",
	"UN_POWER",
	"EAT_GOLD",
	"EAT_ITEM",
	"EAT_FOOD",
	"EAT_LITE",
	"ACID",
	"ELEC",
	"FIRE",
	"COLD",
	"BLIND",
	"CONFUSE",
	"TERRIFY",
	"PARALYZE",
	"LOSE_STR",
	"LOSE_INT",
	"LOSE_WIS",
	"LOSE_DEX",
	"LOSE_CON",
	"LOSE_CHR",
	"LOSE_ALL",
	"SHATTER",
	"EXP_10",
	"EXP_20",
	"EXP_40",
	"EXP_80",
	"DISEASE",
	"TIME",
	"EXP_VAMP",
	"DR_MANA",
	"SUPERHURT",
	"INERTIA",
	"STUN",
	NULL
};

/*!
 * 地形属性トークンの定義 /
 * Feature info flags
 */
static concptr f_info_flags[] =
{
	"LOS",
	"PROJECT",
	"MOVE",
	"PLACE",
	"DROP",
	"SECRET",
	"NOTICE",
	"REMEMBER",
	"OPEN",
	"CLOSE",
	"BASH",
	"SPIKE",
	"DISARM",
	"STORE",
	"TUNNEL",
	"MAY_HAVE_GOLD",
	"HAS_GOLD",
	"HAS_ITEM",
	"DOOR",
	"TRAP",
	"STAIRS",
	"GLYPH",
	"LESS",
	"MORE",
	"AVOID_RUN",
	"FLOOR",
	"WALL",
	"PERMANENT",
	"XXX00",
	"XXX01",
	"XXX02",
	"HIT_TRAP",

	"BRIDGE",
	"RIVER",
	"LAKE",
	"BRIDGED",
	"COVERED",
	"GLOW",
	"ENSECRET",
	"WATER",
	"LAVA",
	"SHALLOW",
	"DEEP",
	"POISON_PUDDLE",
	"HURT_ROCK",
	"HURT_FIRE",
	"HURT_COLD",
	"HURT_ACID",
	"COLD_PUDDLE",
	"ACID_PUDDLE",
	"OIL",
	"ELEC_PUDDLE",
	"CAN_CLIMB",
	"CAN_FLY",
	"CAN_SWIM",
	"CAN_PASS",
	"CAN_OOZE",
	"CAN_DIG",
	"HIDE_ITEM",
	"HIDE_SNEAK",
	"HIDE_SWIM",
	"HIDE_DIG",
	"KILL_HUGE",
	"KILL_MOVE",

	"PICK_TRAP",
	"PICK_DOOR",
	"ALLOC",
	"CHEST",
	"DROP_1D2",
	"DROP_2D2",
	"DROP_GOOD",
	"DROP_GREAT",
	"HURT_POIS",
	"HURT_ELEC",
	"HURT_WATER",
	"HURT_BWATER",
	"USE_FEAT",
	"GET_FEAT",
	"GROUND",
	"OUTSIDE",
	"EASY_HIDE",
	"EASY_CLIMB",
	"MUST_CLIMB",
	"TREE",
	"NEED_TREE",
	"BLOOD",
	"DUST",
	"SLIME",
	"PLANT",
	"XXX2",
	"INSTANT",
	"EXPLODE",
	"TIMED",
	"ERUPT",
	"STRIKE",
	"SPREAD",

	"SPECIAL",
	"HURT_DISI",
	"QUEST_ENTER",
	"QUEST_EXIT",
	"QUEST",
	"SHAFT",
	"MOUNTAIN",
	"BLDG",
	"MINOR_GLYPH",
	"PATTERN",
	"TOWN",
	"ENTRANCE",
	"MIRROR",
	"UNPERM",
	"TELEPORTABLE",
	"CONVERT",
	"GLASS",
};


/*!
 * モンスター特性トークンの定義1 /
 * Monster race flags
 */
static concptr r_info_flags1[] =
{
	"UNIQUE",
	"QUESTOR",
	"MALE",
	"FEMALE",
	"CHAR_CLEAR",
	"SHAPECHANGER",
	"ATTR_CLEAR",
	"ATTR_MULTI",
	"FORCE_DEPTH",
	"FORCE_MAXHP",
	"FORCE_SLEEP",
	"FORCE_EXTRA",
	"ATTR_SEMIRAND",
	"FRIENDS",
	"ESCORT",
	"ESCORTS",
	"NEVER_BLOW",
	"NEVER_MOVE",
	"RAND_25",
	"RAND_50",
	"ONLY_GOLD",
	"ONLY_ITEM",
	"DROP_60",
	"DROP_90",
	"DROP_1D2",
	"DROP_2D2",
	"DROP_3D2",
	"DROP_4D2",
	"DROP_GOOD",
	"DROP_GREAT",
	"XXX2",
	"XXX3"
};

/*!
 * モンスター特性トークンの定義2 /
 * Monster race flags
 */
static concptr r_info_flags2[] =
{
	"STUPID",
	"SMART",
	"CAN_SPEAK",
	"REFLECTING",
	"INVISIBLE",
	"COLD_BLOOD",
	"EMPTY_MIND",
	"WEIRD_MIND",
	"MULTIPLY",
	"REGENERATE",
	"CHAR_MULTI",
	"ATTR_ANY",
	"POWERFUL",
	"ELDRITCH_HORROR",
	"AURA_FIRE",
	"AURA_ELEC",
	"OPEN_DOOR",
	"BASH_DOOR",
	"PASS_WALL",
	"KILL_WALL",
	"MOVE_BODY",
	"KILL_BODY",
	"TAKE_ITEM",
	"KILL_ITEM",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"HUMAN",
	"QUANTUM"
};

/*!
 * モンスター特性トークンの定義3 /
 * Monster race flags
 */
static concptr r_info_flags3[] =
{
	"ORC",
	"TROLL",
	"GIANT",
	"DRAGON",
	"DEMON",
	"UNDEAD",
	"EVIL",
	"ANIMAL",
	"AMBERITE",
	"GOOD",
	"AURA_COLD",
	"NONLIVING",
	"HURT_LITE",
	"HURT_ROCK",
	"HURT_FIRE",
	"HURT_COLD",
	"ANGEL",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"NO_FEAR",
	"NO_STUN",
	"NO_CONF",
	"NO_SLEEP"
};

/*!
 * モンスター特性トークンの定義4 /
 * Monster race flags
 */
static concptr r_info_flags4[] =
{
	"SHRIEK",
	"XXX1",
	"DISPEL",
	"ROCKET",
	"SHOOT",
	"XXX2",
	"XXX3",
	"XXX4",
	"BR_ACID",
	"BR_ELEC",
	"BR_FIRE",
	"BR_COLD",
	"BR_POIS",
	"BR_NETH",
	"BR_LITE",
	"BR_DARK",
	"BR_CONF",
	"BR_SOUN",
	"BR_CHAO",
	"BR_DISE",
	"BR_NEXU",
	"BR_TIME",
	"BR_INER",
	"BR_GRAV",
	"BR_SHAR",
	"BR_PLAS",
	"BR_WALL",
	"BR_MANA",
	"BA_NUKE",
	"BR_NUKE",
	"BA_CHAO",
	"BR_DISI",
};

/*!
 * モンスター特性トークン(発動型能力1) /
 * Monster race flags
 */
static concptr r_a_ability_flags1[] =
{
	"BA_ACID",
	"BA_ELEC",
	"BA_FIRE",
	"BA_COLD",
	"BA_POIS",
	"BA_NETH",
	"BA_WATE",
	"BA_MANA",
	"BA_DARK",
	"DRAIN_MANA",
	"MIND_BLAST",
	"BRAIN_SMASH",
	"CAUSE_1",
	"CAUSE_2",
	"CAUSE_3",
	"CAUSE_4",
	"BO_ACID",
	"BO_ELEC",
	"BO_FIRE",
	"BO_COLD",
	"BA_LITE",
	"BO_NETH",
	"BO_WATE",
	"BO_MANA",
	"BO_PLAS",
	"BO_ICEE",
	"MISSILE",
	"SCARE",
	"BLIND",
	"CONF",
	"SLOW",
	"HOLD"
};

/*!
 * モンスター特性トークン(発動型能力2) /
 * Monster race flags
 */
static concptr r_a_ability_flags2[] =
{
	"HASTE",
	"HAND_DOOM",
	"HEAL",
	"INVULNER",
	"BLINK",
	"TPORT",
	"WORLD",
	"SPECIAL",
	"TELE_TO",
	"TELE_AWAY",
	"TELE_LEVEL",
	"PSY_SPEAR",
	"DARKNESS",
	"TRAPS",
	"FORGET",
	"ANIM_DEAD", /* ToDo: Implement ANIM_DEAD */
	"S_KIN",
	"S_CYBER",
	"S_MONSTER",
	"S_MONSTERS",
	"S_ANT",
	"S_SPIDER",
	"S_HOUND",
	"S_HYDRA",
	"S_ANGEL",
	"S_DEMON",
	"S_UNDEAD",
	"S_DRAGON",
	"S_HI_UNDEAD",
	"S_HI_DRAGON",
	"S_AMBERITES",
	"S_UNIQUE"
};

#if 0
/*!
 * モンスター特性トークン(発動型能力3) /
 * Monster race flags
 */
static concptr r_a_ability_flags3[] =
{
	"XXXA3X00",
	"XXXA3X01",
	"XXXA3X02",
	"XXXA3X03",
	"XXXA3X04",
	"XXXA3X05",
	"XXXA3X06",
	"XXXA3X07",
	"XXXA3X08",
	"XXXA3X09",
	"XXXA3X10",
	"XXXA3X11",
	"XXXA3X12",
	"XXXA3X13",
	"XXXA3X14",
	"XXXA3X15",
	"XXXA3X16",
	"XXXA3X17",
	"XXXA3X18",
	"XXXA3X19",
	"XXXA3X20",
	"XXXA3X21",
	"XXXA3X22",
	"XXXA3X23",
	"XXXA3X24",
	"XXXA3X25",
	"XXXA3X26",
	"XXXA3X27",
	"XXXA3X28",
	"XXXA3X29",
	"XXXA3X30",
	"XXXA3X31",
};
#endif

#if 0
/*!
 * モンスター特性トークン(発動型能力4) /
 * Monster race flags
 */
static concptr r_a_ability_flags4[] =
{
	"XXXA4X00",
	"XXXA4X01",
	"XXXA4X02",
	"XXXA4X03",
	"XXXA4X04",
	"XXXA4X05",
	"XXXA4X06",
	"XXXA4X07",
	"XXXA4X08",
	"XXXA4X09",
	"XXXA4X10",
	"XXXA4X11",
	"XXXA4X12",
	"XXXA4X13",
	"XXXA4X14",
	"XXXA4X15",
	"XXXA4X16",
	"XXXA4X17",
	"XXXA4X18",
	"XXXA4X19",
	"XXXA4X20",
	"XXXA4X21",
	"XXXA4X22",
	"XXXA4X23",
	"XXXA4X24",
	"XXXA4X25",
	"XXXA4X26",
	"XXXA4X27",
	"XXXA4X28",
	"XXXA4X29",
	"XXXA4X30",
	"XXXA4X31",
};
#endif


/*!
 * モンスター特性トークンの定義7 /
 * Monster race flags
 */
static concptr r_info_flags7[] =
{
	"AQUATIC",
	"CAN_SWIM",
	"CAN_FLY",
	"FRIENDLY",
	"NAZGUL",
	"UNIQUE2",
	"RIDING",
	"KAGE",
	"HAS_LITE_1",
	"SELF_LITE_1",
	"HAS_LITE_2",
	"SELF_LITE_2",
	"GUARDIAN",
	"CHAMELEON",
	"XXXX4XXX",
	"TANUKI",
	"HAS_DARK_1",
	"SELF_DARK_1",
	"HAS_DARK_2",
	"SELF_DARK_2",
	"XXX7X20",
	"XXX7X21",
	"XXX7X22",
	"XXX7X23",
	"XXX7X24",
	"XXX7X25",
	"XXX7X26",
	"XXX7X27",
	"XXX7X28",
	"XXX7X29",
	"XXX7X30",
	"XXX7X31",
};

/*!
 * モンスター特性トークンの定義8 /
 * Monster race flags
 */
static concptr r_info_flags8[] =
{
	"WILD_ONLY",
	"WILD_TOWN",
	"XXX8X02",
	"WILD_SHORE",
	"WILD_OCEAN",
	"WILD_WASTE",
	"WILD_WOOD",
	"WILD_VOLCANO",
	"XXX8X08",
	"WILD_MOUNTAIN",
	"WILD_GRASS",
	"XXX8X11",
	"XXX8X12",
	"XXX8X13",
	"XXX8X14",
	"XXX8X15",
	"XXX8X16",
	"XXX8X17",
	"XXX8X18",
	"XXX8X19",
	"XXX8X20",
	"XXX8X21",
	"XXX8X22",
	"XXX8X23",
	"XXX8X24",
	"XXX8X25",
	"XXX8X26",
	"XXX8X27",
	"XXX8X28",
	"XXX8X29",
	"WILD_SWAMP",	/* ToDo: Implement Swamp */
	"WILD_ALL",
};


/*!
 * モンスター特性トークンの定義9 /
 * Monster race flags
 */
static concptr r_info_flags9[] =
{
	"DROP_CORPSE",
	"DROP_SKELETON",
	"EAT_BLIND",
	"EAT_CONF",
	"EAT_MANA",
	"EAT_NEXUS",
	"EAT_BLINK",
	"EAT_SLEEP",
	"EAT_BERSERKER",
	"EAT_ACIDIC",
	"EAT_SPEED",
	"EAT_CURE",
	"EAT_FIRE_RES",
	"EAT_COLD_RES",
	"EAT_ACID_RES",
	"EAT_ELEC_RES",
	"EAT_POIS_RES",
	"EAT_INSANITY",
	"EAT_DRAIN_EXP",
	"EAT_POISONOUS",
	"EAT_GIVE_STR",
	"EAT_GIVE_INT",
	"EAT_GIVE_WIS",
	"EAT_GIVE_DEX",
	"EAT_GIVE_CON",
	"EAT_GIVE_CHR",
	"EAT_LOSE_STR",
	"EAT_LOSE_INT",
	"EAT_LOSE_WIS",
	"EAT_LOSE_DEX",
	"EAT_LOSE_CON",
	"EAT_LOSE_CHR",
	"EAT_DRAIN_MANA",
};


/*!
 * モンスター特性トークンの定義R(耐性) /
 * Monster race flags
 */
static concptr r_info_flagsr[] =
{
	"IM_ACID",
	"IM_ELEC",
	"IM_FIRE",
	"IM_COLD",
	"IM_POIS",
	"RES_LITE",
	"RES_DARK",
	"RES_NETH",
	"RES_WATE",
	"RES_PLAS",
	"RES_SHAR",
	"RES_SOUN",
	"RES_CHAO",
	"RES_NEXU",
	"RES_DISE",
	"RES_WALL",
	"RES_INER",
	"RES_TIME",
	"RES_GRAV",
	"RES_ALL",
	"RES_TELE",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
};


/*!
 * オブジェクト基本特性トークンの定義 /
 * Object flags
 */
static concptr k_info_flags[] =
{
	"STR",
	"INT",
	"WIS",
	"DEX",
	"CON",
	"CHR",
	"MAGIC_MASTERY",
	"FORCE_WEAPON",
	"STEALTH",
	"SEARCH",
	"INFRA",
	"TUNNEL",
	"SPEED",
	"BLOWS",
	"CHAOTIC",
	"VAMPIRIC",
	"SLAY_ANIMAL",
	"SLAY_EVIL",
	"SLAY_UNDEAD",
	"SLAY_DEMON",
	"SLAY_ORC",
	"SLAY_TROLL",
	"SLAY_GIANT",
	"SLAY_DRAGON",
	"KILL_DRAGON",
	"VORPAL",
	"IMPACT",
	"BRAND_POIS",
	"BRAND_ACID",
	"BRAND_ELEC",
	"BRAND_FIRE",
	"BRAND_COLD",

	"SUST_STR",
	"SUST_INT",
	"SUST_WIS",
	"SUST_DEX",
	"SUST_CON",
	"SUST_CHR",
	"RIDING",
	"EASY_SPELL",
	"IM_ACID",
	"IM_ELEC",
	"IM_FIRE",
	"IM_COLD",
	"THROW",
	"REFLECT",
	"FREE_ACT",
	"HOLD_EXP",
	"RES_ACID",
	"RES_ELEC",
	"RES_FIRE",
	"RES_COLD",
	"RES_POIS",
	"RES_FEAR",
	"RES_LITE",
	"RES_DARK",
	"RES_BLIND",
	"RES_CONF",
	"RES_SOUND",
	"RES_SHARDS",
	"RES_NETHER",
	"RES_NEXUS",
	"RES_CHAOS",
	"RES_DISEN",

	"SH_FIRE",
	"SH_ELEC",
	"SLAY_HUMAN",
	"SH_COLD",
	"NO_TELE",
	"NO_MAGIC",
	"DEC_MANA",
	"TY_CURSE",
	"WARNING",
	"HIDE_TYPE",
	"SHOW_MODS",
	"SLAY_GOOD",
	"LEVITATION",
	"LITE",
	"SEE_INVIS",
	"TELEPATHY",
	"SLOW_DIGEST",
	"REGEN",
	"XTRA_MIGHT",
	"XTRA_SHOTS",
	"IGNORE_ACID",
	"IGNORE_ELEC",
	"IGNORE_FIRE",
	"IGNORE_COLD",
	"ACTIVATE",
	"DRAIN_EXP",
	"TELEPORT",
	"AGGRAVATE",
	"BLESSED",
	"XXX3", /* Fake flag for Smith */
	"XXX4", /* Fake flag for Smith */
	"KILL_GOOD",

	"KILL_ANIMAL",
	"KILL_EVIL",
	"KILL_UNDEAD",
	"KILL_DEMON",
	"KILL_ORC",
	"KILL_TROLL",
	"KILL_GIANT",
	"KILL_HUMAN",
	"ESP_ANIMAL",
	"ESP_UNDEAD",
	"ESP_DEMON",
	"ESP_ORC",
	"ESP_TROLL",
	"ESP_GIANT",
	"ESP_DRAGON",
	"ESP_HUMAN",
	"ESP_EVIL",
	"ESP_GOOD",
	"ESP_NONLIVING",
	"ESP_UNIQUE",
	"FULL_NAME",
	"FIXED_FLAVOR",
	"ADD_L_CURSE",
	"ADD_H_CURSE",
	"DRAIN_HP",
	"DRAIN_MANA",

	"LITE_2",
	"LITE_3",
	"LITE_M1",
	"LITE_M2",
	"LITE_M3",
	"LITE_FUEL",

	"CALL_ANIMAL",
	"CALL_DEMON",
	"CALL_DRAGON",
	"CALL_UNDEAD",
	"COWARDICE",
	"LOW_MELEE",
	"LOW_AC",
	"LOW_MAGIC",
	"FAST_DIGEST",
	"SLOW_REGEN",
};

/*!
 * オブジェクト生成特性トークンの定義 /
 * Object flags
 */
static concptr k_info_gen_flags[] =
{
	"INSTA_ART",
	"QUESTITEM",
	"XTRA_POWER",
	"ONE_SUSTAIN",
	"XTRA_RES_OR_POWER",
	"XTRA_H_RES",
	"XTRA_E_RES",
	"XTRA_L_RES",
	"XTRA_D_RES",
	"XTRA_RES",
	"CURSED",
	"HEAVY_CURSE",
	"PERMA_CURSE",
	"RANDOM_CURSE0",
	"RANDOM_CURSE1",
	"RANDOM_CURSE2",
	"XTRA_DICE",
	"POWERFUL",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
	"XXX",
};

/*!
 * ダンジョン特性トークンの定義 /
 * Dungeon flags
 */
static concptr d_info_flags1[] =
{
	"WINNER",
	"MAZE",
	"SMALLEST",
	"BEGINNER",
	"BIG",
	"NO_DOORS",
	"WATER_RIVER",
	"LAVA_RIVER",
	"CURTAIN",
	"GLASS_DOOR",
	"CAVE",
	"CAVERN",
	"ARCADE",
	"LAKE_ACID",
	"LAKE_POISONOUS",
	"XXX",
	"FORGET",
	"LAKE_WATER",
	"LAKE_LAVA",
	"LAKE_RUBBLE",
	"LAKE_TREE",
	"NO_VAULT",
	"ARENA",
	"DESTROY",
	"GLASS_ROOM",
	"NO_CAVE",
	"NO_MAGIC",
	"NO_MELEE",
	"CHAMELEON",
	"DARKNESS",
	"ACID_RIVER",
	"POISONOUS_RIVER"
};


/*!
 * @brief データの可変文字列情報をテキストとして保管する /
 * Add a text to the text-storage and store offset to it.
 * @param offset 文字列保管ポインタからのオフセット
 * @param head テキスト保管ヘッダ情報の構造体参照ポインタ
 * @param buf 保管文字列
 * @param normal_text テキストの正規化を行う
 * @return
 * 無事保管ができたらTRUEを返す。
 * Returns FALSE when there isn't enough space available to store
 * the text.
 */
static bool add_text(u32b *offset, header *head, concptr buf, bool normal_text)
{
	/* Hack -- Verify space */
	if (head->text_size + strlen(buf) + 8 > FAKE_TEXT_SIZE)
		return (FALSE);

	/* New text? */
	if (*offset == 0)
	{
		/* Advance and save the text index */
		*offset = ++head->text_size;
	}

	/* Additional text */
	else if (normal_text)
	{
		/*
		 * If neither the end of the last line nor
		 * the beginning of current line is not a space,
		 * fill up a space as a correct separator of two words.
		 */
		if (head->text_size > 0 &&
#ifdef JP
		(*(head->text_ptr + head->text_size - 1) != ' ') &&
			((head->text_size == 1) || !iskanji(*(head->text_ptr + head->text_size - 2))) &&
			(buf[0] != ' ') && !iskanji(buf[0])
#else
			(*(head->text_ptr + head->text_size - 1) != ' ') &&
			(buf[0] != ' ')
#endif
			)
		{
			/* Append a space */
			*(head->text_ptr + head->text_size) = ' ';

			/* Advance the index */
			head->text_size++;
		}
	}

	/* Append chars to the text */
	strcpy(head->text_ptr + head->text_size, buf);

	/* Advance the index */
	head->text_size += strlen(buf);

	/* Success */
	return (TRUE);
}


/*!
 * @brief データの可変文字列情報を名前として保管する /
 * Add a name to the name-storage and return an offset to it.
 * @param offset 文字列保管ポインタからのオフセット
 * @param head テキスト保管ヘッダ情報の構造体参照ポインタ
 * @param buf 保管文字列
 * @return
 * 無事保管ができたらTRUEを返す。
 * Returns FALSE when there isn't enough space available to store
 * the text.
 */
static bool add_name(u32b *offset, header *head, concptr buf)
{
	/* Hack -- Verify space */
	if (head->name_size + strlen(buf) + 8 > FAKE_NAME_SIZE)
		return (FALSE);

	/* New name? */
	if (*offset == 0)
	{
		/* Advance and save the name index */
		*offset = ++head->name_size;
	}

	/* Append chars to the names */
	strcpy(head->name_ptr + head->name_size, buf);

	/* Advance the index */
	head->name_size += strlen(buf);

	/* Success */
	return (TRUE);
}


/*!
 * @brief データの可変文字列情報をタグとして保管する /
 * Add a tag to the tag-storage and return an offset to it.
 * @param offset 文字列保管ポインタからのオフセット
 * @param head テキスト保管ヘッダ情報の構造体参照ポインタ
 * @param buf 保管文字列
 * @return
 * 無事保管ができたらTRUEを返す。
 * Returns FALSE when there isn't enough space available to store
 * the text.
 */
static bool add_tag(STR_OFFSET *offset, header *head, concptr buf)
{
	u32b i;

	/* Search for an existing (fake) tag */
	for (i = 1; i < head->tag_size; i += strlen(&head->tag_ptr[i]) + 1)
	{
		/* Found it */
		if (streq(&head->tag_ptr[i], buf)) break;
	}

	/* There was no existing tag */
	if (i >= head->tag_size)
	{
		/* Hack -- Verify space */
		if (head->tag_size + strlen(buf) + 8 > FAKE_TAG_SIZE)
			return FALSE;

		/* Append chars to the tags */
		strcpy(head->tag_ptr + head->tag_size, buf);

		/* Point the new tag */
		i = head->tag_size;

		/* Advance the index */
		head->tag_size += strlen(buf) + 1;
	}

	/* Return offset of the tag */
	*offset = (s16b)i;

	/* Success */
	return TRUE;
}


/*** Initialize from ascii template files ***/


/*!
 * @brief パース関数に基づいてデータファイルからデータを読み取る /
 * Initialize an "*_info" array, by parsing an ascii "template" file
 * @param fp 読み取りに使うファイルポインタ
 * @param buf 読み取りに使うバッファ領域
 * @param head ヘッダ構造体
 * @param parse_info_txt_line パース関数
 * @return エラーコード
 */
errr init_info_txt(FILE *fp, char *buf, header *head, parse_info_txt_func parse_info_txt_line)
{
	errr err;

	/* Just before the first record */
	error_idx = -1;

	/* Just before the first line */
	error_line = 0;


	/* Prepare the "fake" stuff */
	head->name_size = 0;
	head->text_size = 0;
	head->tag_size = 0;

	/* Parse */
	while (0 == my_fgets(fp, buf, 1024))
	{
		/* Advance the line number */
		error_line++;

		/* Skip comments and blank lines */
		if (!buf[0] || (buf[0] == '#')) continue;

		/* Verify correct "colon" format */
		if (buf[1] != ':') return (PARSE_ERROR_GENERIC);


		/* Hack -- Process 'V' for "Version" */
		if (buf[0] == 'V')
		{
			/* ignore */
			continue;
		}

		/* Mega Hack -- Calculate Check Sum */
		if (buf[0] != 'N' && buf[0] != 'D')
		{
			int i;
			for (i = 0; buf[i]; i++)
			{
				head->v_extra += (byte)buf[i];
				head->v_extra ^= (1 << (i % 8));
			}
		}

		/* Parse the line */
		if ((err = (*parse_info_txt_line)(buf, head)) != 0)
			return (err);
	}


	/* Complete the "name" and "text" sizes */
	if (head->name_size) head->name_size++;
	if (head->text_size) head->text_size++;

	/* Success */
	return (0);
}


/*!
 * @brief Vault情報(v_info)のパース関数 /
 * Initialize the "v_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_v_info(char *buf, header *head)
{
	int i;
	char *s;

	/* Current entry */
	static vault_type *v_ptr = NULL;

	/* Process 'N' for "New/Number/Name" */
	if (buf[0] == 'N')
	{
		/* Find the colon before the name */
		s = my_strchr(buf + 2, ':');

		/* Verify that colon */
		if (!s) return (1);

		/* Nuke the colon, advance to the name */
		*s++ = '\0';

		/* Paranoia -- require a name */
		if (!*s) return (1);

		/* Get the index */
		i = atoi(buf + 2);

		/* Verify information */
		if (i <= error_idx) return (4);

		/* Verify information */
		if (i >= head->info_num) return (2);

		/* Save the index */
		error_idx = i;

		/* Point at the "info" */
		v_ptr = &v_info[i];

		/* Store the name */
		if (!add_name(&v_ptr->name, head, s)) return (7);
	}

	/* There better be a current v_ptr */
	else if (!v_ptr) return (3);

	/* Process 'D' for "Description" */
	else if (buf[0] == 'D')
	{
		/* Acquire the text */
		s = buf + 2;

		/* Store the text */
		if (!add_text(&v_ptr->text, head, s, FALSE)) return (7);
	}

	/* Process 'X' for "Extra info" (one line only) */
	else if (buf[0] == 'X')
	{
		EFFECT_ID typ, rat, hgt, wid;

		/* Scan for the values */
		if (4 != sscanf(buf + 2, "%d:%d:%d:%d",
			&typ, &rat, &hgt, &wid)) return (1);

		/* Save the values */
		v_ptr->typ = (ROOM_IDX)typ;
		v_ptr->rat = (PROB)rat;
		v_ptr->hgt = (POSITION)hgt;
		v_ptr->wid = (POSITION)wid;
	}

	else	return (6);

	/* Success */
	return (0);
}


/*!
 * @brief 職業技能情報(s_info)のパース関数 /
 * Initialize the "s_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_s_info(char *buf, header *head)
{
	int i;

	/* Current entry */
	static skill_table *s_ptr = NULL;


	/* Process 'N' for "New/Number/Name" */
	if (buf[0] == 'N')
	{
		/* Get the index */
		i = atoi(buf + 2);

		/* Verify information */
		if (i <= error_idx) return (4);

		/* Verify information */
		if (i >= head->info_num) return (2);

		/* Save the index */
		error_idx = i;

		/* Point at the "info" */
		s_ptr = &s_info[i];
	}

	/* There better be a current s_ptr */
	else if (!s_ptr) return (3);

	/* Process 'W' for "Weapon exp" */
	else if (buf[0] == 'W')
	{
		int tval, sval, start, max;
		const s16b exp_conv_table[] =
		{
			WEAPON_EXP_UNSKILLED, WEAPON_EXP_BEGINNER, WEAPON_EXP_SKILLED,
			WEAPON_EXP_EXPERT, WEAPON_EXP_MASTER
		};

		/* Scan for the values */
		if (4 != sscanf(buf + 2, "%d:%d:%d:%d",
			&tval, &sval, &start, &max)) return (1);

		if (start < EXP_LEVEL_UNSKILLED || start > EXP_LEVEL_MASTER
			|| max < EXP_LEVEL_UNSKILLED || max > EXP_LEVEL_MASTER) return (8);

		/* Save the values */
		s_ptr->w_start[tval][sval] = exp_conv_table[start];
		s_ptr->w_max[tval][sval] = exp_conv_table[max];
	}

	/* Process 'S' for "Skill exp" */
	else if (buf[0] == 'S')
	{
		int num, start, max;

		/* Scan for the values */
		if (3 != sscanf(buf + 2, "%d:%d:%d",
			&num, &start, &max)) return (1);

		if (start < WEAPON_EXP_UNSKILLED || start > WEAPON_EXP_MASTER
			|| max < WEAPON_EXP_UNSKILLED || max > WEAPON_EXP_MASTER) return (8);

		/* Save the values */
		s_ptr->s_start[num] = (SUB_EXP)start;
		s_ptr->s_max[num] = (SUB_EXP)max;
	}


	else return (6);

	/* Success */
	return (0);
}


/*!
 * @brief 職業魔法情報(m_info)のパース関数 /
 * Initialize the "m_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_m_info(char *buf, header *head)
{
	int i;

	char *s;

	/* Current entry */
	static player_magic *m_ptr = NULL;

	/* ---Hack--- */
	static int realm, magic_idx = 0, readable = 0;


	/* Process 'N' for "New/Number/Name" */
	if (buf[0] == 'N')
	{
		/* Get the index */
		i = atoi(buf + 2);

		/* Verify information */
		if (i <= error_idx) return (4);

		/* Verify information */
		if (i >= head->info_num) return (2);

		/* Save the index */
		error_idx = i;

		/* Point at the "info" */
		m_ptr = &m_info[i];
	}

	/* There better be a current m_ptr */
	else if (!m_ptr) return (3);

	/* Process 'I' for "Info" (one line only) */
	else if (buf[0] == 'I')
	{
		char *book, *stat;
		int xtra, type, first, weight;

		/* Find the colon before the name */
		s = my_strchr(buf + 2, ':');

		/* Verify that colon */
		if (!s) return (1);

		/* Nuke the colon, advance to the name */
		*s++ = '\0';

		book = buf + 2;

		if (streq(book, "SORCERY")) m_ptr->spell_book = TV_SORCERY_BOOK;
		else if (streq(book, "LIFE")) m_ptr->spell_book = TV_LIFE_BOOK;
		else if (streq(book, "MUSIC")) m_ptr->spell_book = TV_MUSIC_BOOK;
		else if (streq(book, "HISSATSU")) m_ptr->spell_book = TV_HISSATSU_BOOK;
		else if (streq(book, "NONE")) m_ptr->spell_book = 0;
		else return (5);

		stat = s;

		/* Find the colon before the name */
		s = my_strchr(s, ':');

		/* Verify that colon */
		if (!s) return (1);

		/* Nuke the colon, advance to the name */
		*s++ = '\0';

		if (streq(stat, "STR")) m_ptr->spell_stat = A_STR;
		else if (streq(stat, "INT")) m_ptr->spell_stat = A_INT;
		else if (streq(stat, "WIS")) m_ptr->spell_stat = A_WIS;
		else if (streq(stat, "DEX")) m_ptr->spell_stat = A_DEX;
		else if (streq(stat, "CON")) m_ptr->spell_stat = A_CON;
		else if (streq(stat, "CHR")) m_ptr->spell_stat = A_CHR;
		else return (5);


		/* Scan for the values */
		if (4 != sscanf(s, "%x:%d:%d:%d",
			(uint *)&xtra, &type, &first, &weight))	return (1);

		m_ptr->spell_xtra = xtra;
		m_ptr->spell_type = type;
		m_ptr->spell_first = first;
		m_ptr->spell_weight = weight;
	}


	/* Process 'R' for "Realm" (one line only) */
	else if (buf[0] == 'R')
	{
		/* Scan for the values */
		if (2 != sscanf(buf + 2, "%d:%d",
			&realm, &readable)) return (1);

		magic_idx = 0;
	}

	else if (buf[0] == 'T')
	{
		int level, mana, fail, exp;

		if (!readable) return (1);
		/* Scan for the values */
		if (4 != sscanf(buf + 2, "%d:%d:%d:%d",
			&level, &mana, &fail, &exp)) return (1);

		m_ptr->info[realm][magic_idx].slevel = (PLAYER_LEVEL)level;
		m_ptr->info[realm][magic_idx].smana = (MANA_POINT)mana;
		m_ptr->info[realm][magic_idx].sfail = (PERCENTAGE)fail;
		m_ptr->info[realm][magic_idx].sexp = (EXP)exp;
		magic_idx++;
	}


	else return (6);

	/* Success */
	return (0);
}


/*!
 * @brief テキストトークンを走査してフラグを一つ得る(汎用) /
 * Grab one flag from a textual string
 * @param flags ビットフラグを追加する先の参照ポインタ
 * @param names トークン定義配列
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
static errr grab_one_flag(u32b *flags, concptr names[], concptr what)
{
	int i;

	/* Check flags */
	for (i = 0; i < 32; i++)
	{
		if (streq(what, names[i]))
		{
			*flags |= (1L << i);
			return 0;
		}
	}

	return -1;
}


/*!
 * @brief テキストトークンを走査してフラグを一つ得る（地形情報向け） /
 * Grab one flag in an feature_type from a textual string
 * @param f_ptr 地形情報を保管する先の構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
static errr grab_one_feat_flag(feature_type *f_ptr, concptr what)
{
	int i;

	/* Check flags */
	for (i = 0; i < FF_FLAG_MAX; i++)
	{
		if (streq(what, f_info_flags[i]))
		{
			add_flag(f_ptr->flags, i);
			return 0;
		}
	}

	msg_format(_("未知の地形フラグ '%s'。", "Unknown feature flag '%s'."), what);

	/* Error */
	return PARSE_ERROR_GENERIC;
}


/*!
 * @brief テキストトークンを走査してフラグ(ステート)を一つ得る（地形情報向け2） /
 * Grab an action in an feature_type from a textual string
 * @param f_ptr 地形情報を保管する先の構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @param count ステートの保存先ID
 * @return エラーコード
 */
static errr grab_one_feat_action(feature_type *f_ptr, concptr what, int count)
{
	FF_FLAGS_IDX i;

	/* Check flags */
	for (i = 0; i < FF_FLAG_MAX; i++)
	{
		if (streq(what, f_info_flags[i]))
		{
			f_ptr->state[count].action = i;
			return 0;
		}
	}

	msg_format(_("未知の地形アクション '%s'。", "Unknown feature action '%s'."), what);

	/* Error */
	return PARSE_ERROR_GENERIC;
}


/*!
 * @brief 地形情報(f_info)のパース関数 /
 * Initialize the "f_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_f_info(char *buf, header *head)
{
	int i;

	char *s, *t;

	/* Current entry */
	static feature_type *f_ptr = NULL;


	/* Process 'N' for "New/Number/Name" */
	if (buf[0] == 'N')
	{
		/* Find the colon before the name */
		s = my_strchr(buf + 2, ':');

		if (s)
		{
			/* Nuke the colon, advance to the name */
			*s++ = '\0';
		}

		/* Get the index */
		i = atoi(buf + 2);

		/* Verify information */
		if (i <= error_idx) return (4);

		/* Verify information */
		if (i >= head->info_num) return (2);

		/* Save the index */
		error_idx = i;

		/* Point at the "info" */
		f_ptr = &f_info[i];

		/* Tag name is given */
		if (s)
		{
			/* Store the tag */
			if (!add_tag(&f_ptr->tag, head, s)) return (7);
		}

		/* Default "mimic" */
		f_ptr->mimic = (FEAT_IDX)i;

		/* Default "destroyed state" -- if not specified */
		f_ptr->destroyed = (FEAT_IDX)i;

		/* Default "states" */
		for (i = 0; i < MAX_FEAT_STATES; i++) f_ptr->state[i].action = FF_FLAG_MAX;
	}

	/* There better be a current f_ptr */
	else if (!f_ptr) return (3);

#ifdef JP
	else if (buf[0] == 'J')
	{
		/* Store the name */
		if (!add_name(&f_ptr->name, head, buf + 2)) return (7);
	}

	else if (buf[0] == 'E')
	{
		/* Ignore english name */
	}
#else
	else if (buf[0] == 'J')
	{
		/* Ignore Japanese name */
	}

	else if (buf[0] == 'E')
	{
		/* Acquire the Text */
		s = buf + 2;

		/* Store the name */
		if (!add_name(&f_ptr->name, head, s)) return (7);
	}
#endif


	/* Process 'M' for "Mimic" (one line only) */
	else if (buf[0] == 'M')
	{
		STR_OFFSET offset;

		if (!add_tag(&offset, head, buf + 2)) return PARSE_ERROR_OUT_OF_MEMORY;

		/* Record a fake tag index */
		f_ptr->mimic_tag = offset;
	}


	/* Process 'G' for "Graphics" (one line only) */
	else if (buf[0] == 'G')
	{
		int j;
		byte s_attr;
		char char_tmp[F_LIT_MAX];
		if (buf[1] != ':') return (1);
		if (!buf[2]) return (1);
		if (buf[3] != ':') return (1);
		if (!buf[4]) return (1);

		/* Extract the char */
		char_tmp[F_LIT_STANDARD] = buf[2];

		/* Extract the color */
		s_attr = color_char_to_attr(buf[4]);
		if (s_attr > 127) return (1);

		/* Save the standard values */
		f_ptr->d_attr[F_LIT_STANDARD] = s_attr;
		f_ptr->d_char[F_LIT_STANDARD] = char_tmp[F_LIT_STANDARD];

		/* Is this feature supports lighting? */
		if (buf[5] == ':')
		{
			/* G:c:a:LIT (default) */
			apply_default_feat_lighting(f_ptr->d_attr, f_ptr->d_char);

			/* G:c:a:lc:la:dc:da */
			if (!streq(buf + 6, "LIT"))
			{
				char attr_lite_tmp[F_LIT_MAX - F_LIT_NS_BEGIN];

				if ((F_LIT_MAX - F_LIT_NS_BEGIN) * 2 != sscanf(buf + 6, "%c:%c:%c:%c",
					&char_tmp[F_LIT_LITE], &attr_lite_tmp[F_LIT_LITE - F_LIT_NS_BEGIN],
					&char_tmp[F_LIT_DARK], &attr_lite_tmp[F_LIT_DARK - F_LIT_NS_BEGIN])) return 1;
				if (buf[F_LIT_MAX * 4 + 1]) return 1;

				for (j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++)
				{
					switch (attr_lite_tmp[j - F_LIT_NS_BEGIN])
					{
					case '*':
						/* Use default lighting */
						break;
					case '-':
						/* No lighting support */
						f_ptr->d_attr[j] = f_ptr->d_attr[F_LIT_STANDARD];
						break;
					default:
						/* Extract the color */
						f_ptr->d_attr[j] = color_char_to_attr(attr_lite_tmp[j - F_LIT_NS_BEGIN]);
						if (f_ptr->d_attr[j] > 127) return 1;
						break;
					}
					f_ptr->d_char[j] = char_tmp[j];
				}
			}
		}
		else if (!buf[5])
		{
			for (j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++)
			{
				f_ptr->d_attr[j] = s_attr;
				f_ptr->d_char[j] = char_tmp[F_LIT_STANDARD];
			}
		}
		else return 1;
	}

	/* Hack -- Process 'F' for flags */
	else if (buf[0] == 'F')
	{
		/* Parse every entry textually */
		for (s = buf + 2; *s; )
		{
			/* Find the end of this entry */
			for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

			/* Nuke and skip any dividers */
			if (*t)
			{
				*t++ = '\0';
				while (*t == ' ' || *t == '|') t++;
			}

			/* Hack -- Read feature subtype */
			if (1 == sscanf(s, "SUBTYPE_%d", &i))
			{
				/* Extract a "subtype" */
				f_ptr->subtype = (FEAT_SUBTYPE)i;

				/* Start at next entry */
				s = t;

				continue;
			}

			/* Hack -- Read feature power */
			if (1 == sscanf(s, "POWER_%d", &i))
			{
				/* Extract a "power" */
				f_ptr->power = (FEAT_POWER)i;

				/* Start at next entry */
				s = t;
				continue;
			}

			/* Parse this entry */
			if (0 != grab_one_feat_flag(f_ptr, s)) return (PARSE_ERROR_INVALID_FLAG);

			/* Start the next entry */
			s = t;
		}
	}

	/* Process 'W' for "More Info" (one line only) */
	else if (buf[0] == 'W')
	{
		int priority;

		/* Scan for the value */
		if (1 != sscanf(buf + 2, "%d", &priority)) return (PARSE_ERROR_GENERIC);

		/* Save the value */
		f_ptr->priority = (FEAT_PRIORITY)priority;
	}

	/* Process 'K' for "States" (up to four lines + default (which cannot be last)) */
	else if (buf[0] == 'K')
	{
		STR_OFFSET offset;

		/* Find the next empty state slot (if any) */
		for (i = 0; i < MAX_FEAT_STATES; i++) if (f_ptr->state[i].action == FF_FLAG_MAX) break;

		/* Oops, no more slots */
		if (i == MAX_FEAT_STATES) return PARSE_ERROR_GENERIC;

		/* Analyze the first field */
		for (s = t = buf + 2; *t && (*t != ':'); t++) /* loop */;

		/* Terminate the field (if necessary) */
		if (*t == ':') *t++ = '\0';

		/* Is this default entry? */
		if (streq(s, "DESTROYED"))
		{
			if (!add_tag(&offset, head, t)) return PARSE_ERROR_OUT_OF_MEMORY;

			/* Record a fake tag index */
			f_ptr->destroyed_tag = offset;
		}
		else
		{
			/* Reset */
			f_ptr->state[i].action = 0;

			/* Parse this entry */
			if (0 != grab_one_feat_action(f_ptr, s, i)) return PARSE_ERROR_INVALID_FLAG;

			if (!add_tag(&offset, head, t)) return PARSE_ERROR_OUT_OF_MEMORY;

			/* Record a fake tag index */
			f_ptr->state[i].result_tag = offset;
		}
	}

	else return (6);

	/* Success */
	return (0);
}


/*!
 * @brief 地形タグからIDを得る /
 * Convert a fake tag to a real feat index
 * @param str タグ文字列
 * @return 地形ID
 */
s16b f_tag_to_index(concptr str)
{
	u16b i;

	/* Search for real index corresponding to this fake tag */
	for (i = 0; i < f_head.info_num; i++)
	{
		if (streq(f_tag + f_info[i].tag, str))
		{
			/* Return the index */
			return (s16b)i;
		}
	}

	/* Not found */
	return -1;
}


/*!
 * @brief 地形タグからIDを得る /
 * Search for real index corresponding to this fake tag
 * @param feat タグ文字列のオフセット
 * @return 地形ID。該当がないなら-1
 */
static FEAT_IDX search_real_feat(STR_OFFSET feat)
{
	FEAT_IDX i;

	/* Don't convert non-fake tag */
	if (feat <= 0)
	{
		return -1;
	}

	/* Search for real index corresponding to this fake tag */
	for (i = 0; i < f_head.info_num; i++)
	{
		if (feat == f_info[i].tag)
		{
			/* Record real index */
			return i;
		}
	}

	/* Undefined tag */
	msg_format(_("未定義のタグ '%s'。", "%s is undefined."), f_tag + feat);
	return -1;
}


/*!
 * @brief 地形情報の各種タグからIDへ変換して結果を収める /
 * Retouch fake tags of f_info
 * @param head ヘッダ構造体
 * @return なし
 */
void retouch_f_info(header *head)
{
	int i;

	/* Convert fake tags to real feat indices */
	for (i = 0; i < head->info_num; i++)
	{
		feature_type *f_ptr = &f_info[i];
		FEAT_IDX j, k;

		k = search_real_feat(f_ptr->mimic_tag);
		f_ptr->mimic = k < 0 ? f_ptr->mimic : k;
		k = search_real_feat(f_ptr->destroyed_tag);
		f_ptr->destroyed = k < 0 ? f_ptr->destroyed : k;
		for (j = 0; j < MAX_FEAT_STATES; j++)
		{
			k = search_real_feat(f_ptr->state[j].result_tag);
			f_ptr->state[j].result = k < 0 ? f_ptr->state[j].result : k;
		}
	}
}


/*!
 * @brief テキストトークンを走査してフラグを一つ得る(ベースアイテム用) /
 * Grab one flag in an object_kind from a textual string
 * @param k_ptr 保管先のベースアイテム構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
static errr grab_one_kind_flag(object_kind *k_ptr, concptr what)
{
	int i;

	/* Check flags */
	for (i = 0; i < TR_FLAG_MAX; i++)
	{
		if (streq(what, k_info_flags[i]))
		{
			add_flag(k_ptr->flags, i);
			return (0);
		}
	}

	if (grab_one_flag(&k_ptr->gen_flags, k_info_gen_flags, what) == 0)
		return 0;

	msg_format(_("未知のアイテム・フラグ '%s'。", "Unknown object flag '%s'."), what);

	/* Error */
	return (1);
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(発動能力用) /
 * Grab one activation index flag
 * @param what 参照元の文字列ポインタ
 * @return 発動能力ID
 */
static byte grab_one_activation_flag(concptr what)
{
	int i;

	for (i = 0; ; i++)
	{
		if (activation_info[i].flag == NULL) break;

		if (streq(what, activation_info[i].flag))
		{
			return activation_info[i].index;
		}
	}

	i = atoi(what);
	if (i > 0)
	{
		return ((byte)i);
	}

	msg_format(_("未知の発動・フラグ '%s'。", "Unknown activation flag '%s'."), what);

	/* Error */
	return (0);
}


/*!
 * @brief ベースアイテム(k_info)のパース関数 /
 * Initialize the "k_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_k_info(char *buf, header *head)
{
	int i;

	char *s, *t;

	/* Current entry */
	static object_kind *k_ptr = NULL;


	/* Process 'N' for "New/Number/Name" */
	if (buf[0] == 'N')
	{
#ifdef JP
		char *flavor;
#endif

		/* Find the colon before the name */
		s = my_strchr(buf + 2, ':');

		/* Verify that colon */
		if (!s) return (1);

		/* Nuke the colon, advance to the name */
		*s++ = '\0';

		/* Get the index */
		i = atoi(buf + 2);

		/* Verify information */
		if (i <= error_idx) return (4);

		/* Verify information */
		if (i >= head->info_num) return (2);

		/* Save the index */
		error_idx = i;

		/* Point at the "info" */
		k_ptr = &k_info[i];

#ifdef JP
		/* Paranoia -- require a name */
		if (!*s) return (1);

		/* Find the colon before the flavor */
		flavor = my_strchr(s, ':');

		/* Verify that colon */
		if (flavor)
		{
			/* Nuke the colon, advance to the flavor */
			*flavor++ = '\0';

			/* Store the flavor */
			if (!add_name(&k_ptr->flavor_name, head, flavor)) return (7);
		}

		/* Store the name */
		if (!add_name(&k_ptr->name, head, s)) return (7);
#endif
	}

	/* There better be a current k_ptr */
	else if (!k_ptr) return (3);


#ifdef JP
	/* 英語名を読むルーチンを追加 */
	/* 'E' から始まる行は英語名としている */
	else if (buf[0] == 'E')
	{
		/* nothing to do */
	}
#else
	else if (buf[0] == 'E')
	{
		char *flavor;

		/* Acquire the name */
		s = buf + 2;

		/* Find the colon before the flavor */
		flavor = my_strchr(s, ':');

		/* Verify that colon */
		if (flavor)
		{
			/* Nuke the colon, advance to the flavor */
			*flavor++ = '\0';

			/* Store the flavor */
			if (!add_name(&k_ptr->flavor_name, head, flavor)) return (7);
		}

		/* Store the name */
		if (!add_name(&k_ptr->name, head, s)) return (7);
	}
#endif

	/* Process 'D' for "Description" */
	else if (buf[0] == 'D')
	{
#ifdef JP
		if (buf[2] == '$')
			return (0);
		/* Acquire the text */
		s = buf + 2;
#else
		if (buf[2] != '$')
			return (0);
		/* Acquire the text */
		s = buf + 3;
#endif

		/* Store the text */
		if (!add_text(&k_ptr->text, head, s, TRUE)) return (7);
	}

	/* Process 'G' for "Graphics" (one line only) */
	else if (buf[0] == 'G')
	{
		char sym;
		byte tmp;
		if (buf[1] != ':') return (1);
		if (!buf[2]) return (1);
		if (buf[3] != ':') return (1);
		if (!buf[4]) return (1);

		/* Extract the char */
		sym = buf[2];

		/* Extract the attr */
		tmp = color_char_to_attr(buf[4]);
		if (tmp > 127) return (1);

		/* Save the values */
		k_ptr->d_attr = tmp;
		k_ptr->d_char = sym;
	}

	/* Process 'I' for "Info" (one line only) */
	else if (buf[0] == 'I')
	{
		int tval, sval, pval;

		/* Scan for the values */
		if (3 != sscanf(buf + 2, "%d:%d:%d",
			&tval, &sval, &pval)) return (1);

		/* Save the values */
		k_ptr->tval = (OBJECT_TYPE_VALUE)tval;
		k_ptr->sval = (OBJECT_SUBTYPE_VALUE)sval;
		k_ptr->pval = (PARAMETER_VALUE)pval;
	}

	/* Process 'W' for "More Info" (one line only) */
	else if (buf[0] == 'W')
	{
		int level, extra, wgt;
		long cost;

		/* Scan for the values */
		if (4 != sscanf(buf + 2, "%d:%d:%d:%ld",
			&level, &extra, &wgt, &cost)) return (1);

		/* Save the values */
		k_ptr->level = (DEPTH)level;
		k_ptr->extra = (BIT_FLAGS8)extra;
		k_ptr->weight = (WEIGHT)wgt;
		k_ptr->cost = (PRICE)cost;
	}

	/* Process 'A' for "Allocation" (one line only) */
	else if (buf[0] == 'A')
	{

		/* Simply read each number following a colon */
		for (i = 0, s = buf + 1; s && (s[0] == ':') && s[1]; ++i)
		{
			/* Default chance */
			k_ptr->chance[i] = 1;

			/* Store the attack damage index */
			k_ptr->locale[i] = atoi(s + 1);

			/* Find the slash */
			t = my_strchr(s + 1, '/');

			/* Find the next colon */
			s = my_strchr(s + 1, ':');

			/* If the slash is "nearby", use it */
			if (t && (!s || t < s))
			{
				int chance = atoi(t + 1);
				if (chance > 0) k_ptr->chance[i] = (PROB)chance;
			}
		}
	}

	/* Hack -- Process 'P' for "power" and such */
	else if (buf[0] == 'P')
	{
		int ac, hd1, hd2, th, td, ta;

		/* Scan for the values */
		if (6 != sscanf(buf + 2, "%d:%dd%d:%d:%d:%d",
			&ac, &hd1, &hd2, &th, &td, &ta)) return (1);

		k_ptr->ac = (ARMOUR_CLASS)ac;
		k_ptr->dd = (DICE_NUMBER)hd1;
		k_ptr->ds = (DICE_SID)hd2;
		k_ptr->to_h = (HIT_PROB)th;
		k_ptr->to_d = (HIT_POINT)td;
		k_ptr->to_a = (ARMOUR_CLASS)ta;
	}

	/* Hack -- Process 'U' for activation index */
	else if (buf[0] == 'U')
	{
		byte n;
		n = grab_one_activation_flag(buf + 2);
		if (n > 0)
		{
			k_ptr->act_idx = n;
		}
		else
		{
			return (5);
		}
	}

	/* Hack -- Process 'F' for flags */
	else if (buf[0] == 'F')
	{
		/* Parse every entry textually */
		for (s = buf + 2; *s; )
		{
			/* Find the end of this entry */
			for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

			/* Nuke and skip any dividers */
			if (*t)
			{
				*t++ = '\0';
				while (*t == ' ' || *t == '|') t++;
			}

			/* Parse this entry */
			if (0 != grab_one_kind_flag(k_ptr, s)) return (5);

			/* Start the next entry */
			s = t;
		}
	}


	else return (6);


	/* Success */
	return (0);
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(アーティファクト用) /
 * Grab one activation index flag
 * @param a_ptr 保管先のアーティファクト構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーがあった場合1、エラーがない場合0を返す
 */
static errr grab_one_artifact_flag(artifact_type *a_ptr, concptr what)
{
	int i;

	/* Check flags */
	for (i = 0; i < TR_FLAG_MAX; i++)
	{
		if (streq(what, k_info_flags[i]))
		{
			add_flag(a_ptr->flags, i);
			return (0);
		}
	}

	if (grab_one_flag(&a_ptr->gen_flags, k_info_gen_flags, what) == 0)
		return 0;

	msg_format(_("未知の伝説のアイテム・フラグ '%s'。", "Unknown artifact flag '%s'."), what);

	/* Error */
	return (1);
}


/*!
 * @brief 固定アーティファクト情報(a_info)のパース関数 /
 * Initialize the "a_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_a_info(char *buf, header *head)
{
	int i;

	char *s, *t;

	/* Current entry */
	static artifact_type *a_ptr = NULL;


	/* Process 'N' for "New/Number/Name" */
	if (buf[0] == 'N')
	{
		/* Find the colon before the name */
		s = my_strchr(buf + 2, ':');

		/* Verify that colon */
		if (!s) return (1);

		/* Nuke the colon, advance to the name */
		*s++ = '\0';
#ifdef JP
		/* Paranoia -- require a name */
		if (!*s) return (1);
#endif
		/* Get the index */
		i = atoi(buf + 2);

		/* Verify information */
		if (i < error_idx) return (4);

		/* Verify information */
		if (i >= head->info_num) return (2);

		/* Save the index */
		error_idx = i;

		/* Point at the "info" */
		a_ptr = &a_info[i];

		/* Ignore everything */
		add_flag(a_ptr->flags, TR_IGNORE_ACID);
		add_flag(a_ptr->flags, TR_IGNORE_ELEC);
		add_flag(a_ptr->flags, TR_IGNORE_FIRE);
		add_flag(a_ptr->flags, TR_IGNORE_COLD);
#ifdef JP
		/* Store the name */
		if (!add_name(&a_ptr->name, head, s)) return (7);
#endif
	}

	/* There better be a current a_ptr */
	else if (!a_ptr) return (3);


#ifdef JP
	/* 英語名を読むルーチンを追加 */
	/* 'E' から始まる行は英語名としている */
	else if (buf[0] == 'E')
	{
		/* nothing to do */
	}
#else
	else if (buf[0] == 'E')
	{
		/* Acquire the Text */
		s = buf + 2;

		/* Store the name */
		if (!add_name(&a_ptr->name, head, s)) return (7);
	}
#endif

	/* Process 'D' for "Description" */
	else if (buf[0] == 'D')
	{
#ifdef JP
		if (buf[2] == '$')
			return (0);
		/* Acquire the text */
		s = buf + 2;
#else
		if (buf[2] != '$')
			return (0);
		/* Acquire the text */
		s = buf + 3;
#endif

		/* Store the text */
		if (!add_text(&a_ptr->text, head, s, TRUE)) return (7);
	}


	/* Process 'I' for "Info" (one line only) */
	else if (buf[0] == 'I')
	{
		int tval, sval, pval;

		/* Scan for the values */
		if (3 != sscanf(buf + 2, "%d:%d:%d",
			&tval, &sval, &pval)) return (1);

		/* Save the values */
		a_ptr->tval = (OBJECT_TYPE_VALUE)tval;
		a_ptr->sval = (OBJECT_SUBTYPE_VALUE)sval;
		a_ptr->pval = (PARAMETER_VALUE)pval;
	}

	/* Process 'W' for "More Info" (one line only) */
	else if (buf[0] == 'W')
	{
		int level, rarity, wgt;
		long cost;

		/* Scan for the values */
		if (4 != sscanf(buf + 2, "%d:%d:%d:%ld",
			&level, &rarity, &wgt, &cost)) return (1);

		/* Save the values */
		a_ptr->level = (DEPTH)level;
		a_ptr->rarity = (RARITY)rarity;
		a_ptr->weight = (WEIGHT)wgt;
		a_ptr->cost = (PRICE)cost;
	}

	/* Hack -- Process 'P' for "power" and such */
	else if (buf[0] == 'P')
	{
		int ac, hd1, hd2, th, td, ta;

		/* Scan for the values */
		if (6 != sscanf(buf + 2, "%d:%dd%d:%d:%d:%d",
			&ac, &hd1, &hd2, &th, &td, &ta)) return (1);

		a_ptr->ac = (ARMOUR_CLASS)ac;
		a_ptr->dd = (DICE_NUMBER)hd1;
		a_ptr->ds = (DICE_SID)hd2;
		a_ptr->to_h = (HIT_PROB)th;
		a_ptr->to_d = (HIT_POINT)td;
		a_ptr->to_a = (ARMOUR_CLASS)ta;
	}

	/* Hack -- Process 'U' for activation index */
	else if (buf[0] == 'U')
	{
		byte n;
		n = grab_one_activation_flag(buf + 2);
		if (n > 0)
		{
			a_ptr->act_idx = n;
		}
		else
		{
			return (5);
		}
	}

	/* Hack -- Process 'F' for flags */
	else if (buf[0] == 'F')
	{
		/* Parse every entry textually */
		for (s = buf + 2; *s; )
		{
			/* Find the end of this entry */
			for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

			/* Nuke and skip any dividers */
			if (*t)
			{
				*t++ = '\0';
				while ((*t == ' ') || (*t == '|')) t++;
			}

			/* Parse this entry */
			if (0 != grab_one_artifact_flag(a_ptr, s)) return (5);

			/* Start the next entry */
			s = t;
		}
	}


	else return (6);


	/* Success */
	return (0);
}


/*!
 * @brief テキストトークンを走査してフラグを一つ得る(アーティファクト用) /
 * Grab one flag in a ego-item_type from a textual string
 * @param e_ptr 保管先のエゴ構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーがあった場合1、エラーがない場合0を返す
 */
static bool grab_one_ego_item_flag(ego_item_type *e_ptr, concptr what)
{
	int i;

	/* Check flags */
	for (i = 0; i < TR_FLAG_MAX; i++)
	{
		if (streq(what, k_info_flags[i]))
		{
			add_flag(e_ptr->flags, i);
			return (0);
		}
	}

	if (grab_one_flag(&e_ptr->gen_flags, k_info_gen_flags, what) == 0)
		return 0;

	msg_format(_("未知の名のあるアイテム・フラグ '%s'。", "Unknown ego-item flag '%s'."), what);

	/* Error */
	return (1);
}


/*!
 * @brief アイテムエゴ情報(e_info)のパース関数 /
 * Initialize the "e_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_e_info(char *buf, header *head)
{
	int i;
	char *s, *t;

	/* Current entry */
	static ego_item_type *e_ptr = NULL;

	/* Just before the first record */
	error_idx = -1;

	/* Just before the first line */
	error_line = -1;

	/* Process 'N' for "New/Number/Name" */
	if (buf[0] == 'N')
	{
		/* Find the colon before the name */
		s = my_strchr(buf + 2, ':');

		/* Verify that colon */
		if (!s) return (1);

		/* Nuke the colon, advance to the name */
		*s++ = '\0';
#ifdef JP
		/* Paranoia -- require a name */
		if (!*s) return (1);
#endif
		/* Get the index */
		i = atoi(buf + 2);

		/* Verify information */
		if (i < error_idx) return (4);

		/* Verify information */
		if (i >= head->info_num) return (2);

		/* Save the index */
		error_idx = i;

		/* Point at the "info" */
		e_ptr = &e_info[i];
#ifdef JP
		/* Store the name */
		if (!add_name(&e_ptr->name, head, s)) return (7);
#endif
	}

	/* There better be a current e_ptr */
	else if (!e_ptr) return (3);


#ifdef JP
	/* 英語名を読むルーチンを追加 */
	/* 'E' から始まる行は英語名 */
	else if (buf[0] == 'E')
	{
		/* nothing to do */
	}
#else
	else if (buf[0] == 'E')
	{
		/* Acquire the Text */
		s = buf + 2;

		/* Store the name */
		if (!add_name(&e_ptr->name, head, s)) return (7);
	}
#endif
#if 0

	/* Process 'D' for "Description" */
	else if (buf[0] == 'D')
	{
		/* Acquire the text */
		s = buf + 2;

		/* Store the text */
		if (!add_text(&e_ptr->text, head, s, TRUE)) return (7);
	}

#endif

	/* Process 'X' for "Xtra" (one line only) */
	else if (buf[0] == 'X')
	{
		int slot, rating;

		/* Scan for the values */
		if (2 != sscanf(buf + 2, "%d:%d",
			&slot, &rating)) return (1);

		/* Save the values */
		e_ptr->slot = (INVENTORY_IDX)slot;
		e_ptr->rating = (PRICE)rating;
	}

	/* Process 'W' for "More Info" (one line only) */
	else if (buf[0] == 'W')
	{
		int level, rarity, pad2;
		long cost;

		/* Scan for the values */
		if (4 != sscanf(buf + 2, "%d:%d:%d:%ld",
			&level, &rarity, &pad2, &cost)) return (1);

		/* Save the values */
		e_ptr->level = level;
		e_ptr->rarity = (RARITY)rarity;
		/* e_ptr->weight = wgt; */
		e_ptr->cost = cost;
	}

	/* Hack -- Process 'C' for "creation" */
	else if (buf[0] == 'C')
	{
		int th, td, ta, pval;

		/* Scan for the values */
		if (4 != sscanf(buf + 2, "%d:%d:%d:%d",
			&th, &td, &ta, &pval)) return (1);

		e_ptr->max_to_h = (HIT_PROB)th;
		e_ptr->max_to_d = (HIT_POINT)td;
		e_ptr->max_to_a = (ARMOUR_CLASS)ta;
		e_ptr->max_pval = (PARAMETER_VALUE)pval;
	}

	/* Hack -- Process 'U' for activation index */
	else if (buf[0] == 'U')
	{
		byte n;
		n = grab_one_activation_flag(buf + 2);
		if (n > 0)
		{
			e_ptr->act_idx = n;
		}
		else
		{
			return (5);
		}
	}

	/* Hack -- Process 'F' for flags */
	else if (buf[0] == 'F')
	{
		/* Parse every entry textually */
		for (s = buf + 2; *s; )
		{
			/* Find the end of this entry */
			for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

			/* Nuke and skip any dividers */
			if (*t)
			{
				*t++ = '\0';
				while ((*t == ' ') || (*t == '|')) t++;
			}

			/* Parse this entry */
			if (0 != grab_one_ego_item_flag(e_ptr, s)) return (5);

			/* Start the next entry */
			s = t;
		}
	}

	else return (6);

	/* Success */
	return (0);
}


/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスター用1) /
 * Grab one (basic) flag in a monster_race from a textual string
 * @param r_ptr 保管先のモンスター種族構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
static errr grab_one_basic_flag(monster_race *r_ptr, concptr what)
{
	if (grab_one_flag(&r_ptr->flags1, r_info_flags1, what) == 0)
		return 0;

	if (grab_one_flag(&r_ptr->flags2, r_info_flags2, what) == 0)
		return 0;

	if (grab_one_flag(&r_ptr->flags3, r_info_flags3, what) == 0)
		return 0;

	if (grab_one_flag(&r_ptr->flags7, r_info_flags7, what) == 0)
		return 0;

	if (grab_one_flag(&r_ptr->flags8, r_info_flags8, what) == 0)
		return 0;

	if (grab_one_flag(&r_ptr->flags9, r_info_flags9, what) == 0)
		return 0;

	if (grab_one_flag(&r_ptr->flagsr, r_info_flagsr, what) == 0)
		return 0;

	msg_format(_("未知のモンスター・フラグ '%s'。", "Unknown monster flag '%s'."), what);

	/* Failure */
	return (1);
}


/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスター用2) /
 * Grab one (spell) flag in a monster_race from a textual string
 * @param r_ptr 保管先のモンスター種族構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
static errr grab_one_spell_flag(monster_race *r_ptr, concptr what)
{
	if (grab_one_flag(&r_ptr->flags4, r_info_flags4, what) == 0)
		return 0;

	if (grab_one_flag(&r_ptr->a_ability_flags1, r_a_ability_flags1, what) == 0)
		return 0;

	if (grab_one_flag(&r_ptr->a_ability_flags2, r_a_ability_flags2, what) == 0)
		return 0;

	msg_format(_("未知のモンスター・フラグ '%s'。", "Unknown monster flag '%s'."), what);

	/* Failure */
	return (1);
}


/*!
 * @brief モンスター種族情報(r_info)のパース関数 /
 * Initialize the "r_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_r_info(char *buf, header *head)
{
	int i;
	char *s, *t;

	/* Current entry */
	static monster_race *r_ptr = NULL;


	/* Process 'N' for "New/Number/Name" */
	if (buf[0] == 'N')
	{
		/* Find the colon before the name */
		s = my_strchr(buf + 2, ':');

		/* Verify that colon */
		if (!s) return (1);

		/* Nuke the colon, advance to the name */
		*s++ = '\0';
#ifdef JP
		/* Paranoia -- require a name */
		if (!*s) return (1);
#endif
		/* Get the index */
		i = atoi(buf + 2);

		/* Verify information */
		if (i < error_idx) return (4);

		/* Verify information */
		if (i >= head->info_num) return (2);

		/* Save the index */
		error_idx = i;

		/* Point at the "info" */
		r_ptr = &r_info[i];
#ifdef JP
		/* Store the name */
		if (!add_name(&r_ptr->name, head, s)) return (7);
#endif
	}

	/* There better be a current r_ptr */
	else if (!r_ptr) return (3);


#ifdef JP
	/* 英語名を読むルーチンを追加 */
	/* 'E' から始まる行は英語名 */
	else if (buf[0] == 'E')
	{
		/* Acquire the Text */
		s = buf + 2;

		/* Store the name */
		if (!add_name(&r_ptr->E_name, head, s)) return (7);
	}
#else
	else if (buf[0] == 'E')
	{
		/* Acquire the Text */
		s = buf + 2;

		/* Store the name */
		if (!add_name(&r_ptr->name, head, s)) return (7);
	}
#endif
	/* Process 'D' for "Description" */
	else if (buf[0] == 'D')
	{
#ifdef JP
		if (buf[2] == '$')
			return (0);
		/* Acquire the text */
		s = buf + 2;
#else
		if (buf[2] != '$')
			return (0);
		/* Acquire the text */
		s = buf + 3;
#endif

		/* Store the text */
		if (!add_text(&r_ptr->text, head, s, TRUE)) return (7);
	}

	/* Process 'G' for "Graphics" (one line only) */
	else if (buf[0] == 'G')
	{
		char sym;
		byte tmp;
		if (buf[1] != ':') return (1);
		if (!buf[2]) return (1);
		if (buf[3] != ':') return (1);
		if (!buf[4]) return (1);

		/* Extract the char */
		sym = buf[2];

		/* Extract the attr */
		tmp = color_char_to_attr(buf[4]);
		if (tmp > 127) return (1);

		/* Save the values */
		r_ptr->d_char = sym;
		r_ptr->d_attr = tmp;
	}

	/* Process 'I' for "Info" (one line only) */
	else if (buf[0] == 'I')
	{
		int spd, hp1, hp2, aaf, ac, slp;

		/* Scan for the other values */
		if (6 != sscanf(buf + 2, "%d:%dd%d:%d:%d:%d",
			&spd, &hp1, &hp2, &aaf, &ac, &slp)) return (1);

		/* Save the values */
		r_ptr->speed = (SPEED)spd;
		r_ptr->hdice = (DICE_NUMBER)MAX(hp1, 1);
		r_ptr->hside = (DICE_SID)MAX(hp2, 1);
		r_ptr->aaf = (POSITION)aaf;
		r_ptr->ac = (ARMOUR_CLASS)ac;
		r_ptr->sleep = (SLEEP_DEGREE)slp;
	}

	/* Process 'W' for "More Info" (one line only) */
	else if (buf[0] == 'W')
	{
		int lev, rar, pad;
		long exp;
		long nextexp;
		int nextmon;

		/* Scan for the values */
		if (6 != sscanf(buf + 2, "%d:%d:%d:%ld:%ld:%d",
			&lev, &rar, &pad, &exp, &nextexp, &nextmon)) return (1);

		/* Save the values */
		r_ptr->level = (DEPTH)lev;
		r_ptr->rarity = (RARITY)rar;
		r_ptr->extra = (BIT_FLAGS16)pad;
		r_ptr->mexp = (EXP)exp;
		r_ptr->next_exp = (EXP)nextexp;
		r_ptr->next_r_idx = (IDX)nextmon;
	}

	/* Process 'R' for "Reinforcement" (up to six lines) */
	else if (buf[0] == 'R')
	{
		int id, ds, dd;
		/* Find the next empty blow slot (if any) */
		for (i = 0; i < A_MAX; i++) if (r_ptr->reinforce_id[i] == 0) break;

		/* Oops, no more slots */
		if (i == 6) return (1);

		/* Scan for the values */
		if (3 != sscanf(buf + 2, "%d:%dd%d", &id, &dd, &ds)) return (1);
		r_ptr->reinforce_id[i] = (MONRACE_IDX)id;
		r_ptr->reinforce_dd[i] = (DICE_NUMBER)dd;
		r_ptr->reinforce_ds[i] = (DICE_SID)ds;
	}

	/* Process 'B' for "Blows" (up to four lines) */
	else if (buf[0] == 'B')
	{
		int n1, n2;

		/* Find the next empty blow slot (if any) */
		for (i = 0; i < 4; i++) if (!r_ptr->blow[i].method) break;

		/* Oops, no more slots */
		if (i == 4) return (1);

		/* Analyze the first field */
		for (s = t = buf + 2; *t && (*t != ':'); t++) /* loop */;

		/* Terminate the field (if necessary) */
		if (*t == ':') *t++ = '\0';

		/* Analyze the method */
		for (n1 = 0; r_info_blow_method[n1]; n1++)
		{
			if (streq(s, r_info_blow_method[n1])) break;
		}

		/* Invalid method */
		if (!r_info_blow_method[n1]) return (1);

		/* Analyze the second field */
		for (s = t; *t && (*t != ':'); t++) /* loop */;

		/* Terminate the field (if necessary) */
		if (*t == ':') *t++ = '\0';

		/* Analyze effect */
		for (n2 = 0; r_info_blow_effect[n2]; n2++)
		{
			if (streq(s, r_info_blow_effect[n2])) break;
		}

		/* Invalid effect */
		if (!r_info_blow_effect[n2]) return (1);

		/* Analyze the third field */
		for (s = t; *t && (*t != 'd'); t++) /* loop */;

		/* Terminate the field (if necessary) */
		if (*t == 'd') *t++ = '\0';

		/* Save the method */
		r_ptr->blow[i].method = (BLOW_METHOD)n1;

		/* Save the effect */
		r_ptr->blow[i].effect = (BLOW_EFFECT)n2;

		/* Extract the damage dice and sides */
		r_ptr->blow[i].d_dice = atoi(s);
		r_ptr->blow[i].d_side = atoi(t);
	}

	/* Process 'F' for "Basic Flags" (multiple lines) */
	else if (buf[0] == 'F')
	{
		/* Parse every entry */
		for (s = buf + 2; *s; )
		{
			/* Find the end of this entry */
			for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

			/* Nuke and skip any dividers */
			if (*t)
			{
				*t++ = '\0';
				while (*t == ' ' || *t == '|') t++;
			}

			/* Parse this entry */
			if (0 != grab_one_basic_flag(r_ptr, s)) return (5);

			/* Start the next entry */
			s = t;
		}
	}

	/* Process 'S' for "Spell Flags" (multiple lines) */
	else if (buf[0] == 'S')
	{
		/* Parse every entry */
		for (s = buf + 2; *s; )
		{
			/* Find the end of this entry */
			for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

			/* Nuke and skip any dividers */
			if (*t)
			{
				*t++ = '\0';
				while ((*t == ' ') || (*t == '|')) t++;
			}

			/* Hack -- Read spell frequency */
			if (1 == sscanf(s, "1_IN_%d", &i))
			{
				/* Extract a "frequency" */
				r_ptr->freq_spell = 100 / i;

				/* Start at next entry */
				s = t;
				continue;
			}

			/* Parse this entry */
			if (0 != grab_one_spell_flag(r_ptr, s)) return (5);

			/* Start the next entry */
			s = t;
		}
	}

	/* Process 'A' for "Artifact Flags" (multiple lines) */
	else if (buf[0] == 'A')
	{
		int id, per, rarity;

		/* Find the next empty blow slot (if any) */
		for (i = 0; i < 4; i++) if (!r_ptr->artifact_id[i]) break;

		/* Oops, no more slots */
		if (i == 4) return (1);

		if (3 != sscanf(buf + 2, "%d:%d:%d", &id, &rarity, &per)) return (1);
		r_ptr->artifact_id[i] = (ARTIFACT_IDX)id;
		r_ptr->artifact_rarity[i] = (RARITY)rarity;
		r_ptr->artifact_percent[i] = (PERCENTAGE)per;
	}

	/* Process 'V' for "Arena power value ratio" */
	else if (buf[0] == 'V')
	{
		int val;
		if (3 != sscanf(buf + 2, "%d", &val)) return (1);
		r_ptr->arena_ratio = (PERCENTAGE)val;
	}

	else return (6);


	/* Success */
	return (0);
}


/*!
 * @brief テキストトークンを走査してフラグを一つ得る(ダンジョン用) /
 * Grab one flag for a dungeon type from a textual string
 * @param d_ptr 保管先のダンジョン構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
static errr grab_one_dungeon_flag(dungeon_type *d_ptr, concptr what)
{
	if (grab_one_flag(&d_ptr->flags1, d_info_flags1, what) == 0)
		return 0;

	msg_format(_("未知のダンジョン・フラグ '%s'。", "Unknown dungeon type flag '%s'."), what);

	/* Failure */
	return (1);
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスターのダンジョン出現条件用1) /
 * Grab one (basic) flag in a monster_race from a textual string
 * @param d_ptr 保管先のダンジョン構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
static errr grab_one_basic_monster_flag(dungeon_type *d_ptr, concptr what)
{
	if (grab_one_flag(&d_ptr->mflags1, r_info_flags1, what) == 0)
		return 0;

	if (grab_one_flag(&d_ptr->mflags2, r_info_flags2, what) == 0)
		return 0;

	if (grab_one_flag(&d_ptr->mflags3, r_info_flags3, what) == 0)
		return 0;

	if (grab_one_flag(&d_ptr->mflags7, r_info_flags7, what) == 0)
		return 0;

	if (grab_one_flag(&d_ptr->mflags8, r_info_flags8, what) == 0)
		return 0;

	if (grab_one_flag(&d_ptr->mflags9, r_info_flags9, what) == 0)
		return 0;

	if (grab_one_flag(&d_ptr->mflagsr, r_info_flagsr, what) == 0)
		return 0;

	msg_format(_("未知のモンスター・フラグ '%s'。", "Unknown monster flag '%s'."), what);
	/* Failure */
	return (1);
}


/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスターのダンジョン出現条件用2) /
 * Grab one (spell) flag in a monster_race from a textual string
 * @param d_ptr 保管先のダンジョン構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
static errr grab_one_spell_monster_flag(dungeon_type *d_ptr, concptr what)
{
	if (grab_one_flag(&d_ptr->mflags4, r_info_flags4, what) == 0)
		return 0;

	if (grab_one_flag(&d_ptr->m_a_ability_flags1, r_a_ability_flags1, what) == 0)
		return 0;

	if (grab_one_flag(&d_ptr->m_a_ability_flags2, r_a_ability_flags2, what) == 0)
		return 0;

	msg_format(_("未知のモンスター・フラグ '%s'。", "Unknown monster flag '%s'."), what);

	/* Failure */
	return (1);
}

/*!
 * @brief ダンジョン情報(d_info)のパース関数 /
 * Initialize the "d_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_d_info(char *buf, header *head)
{
	int i;
	char *s, *t;

	/* Current entry */
	static dungeon_type *d_ptr = NULL;

	/* Process 'N' for "New/Number/Name" */
	if (buf[0] == 'N')
	{
		/* Find the colon before the name */
		s = my_strchr(buf + 2, ':');

		/* Verify that colon */
		if (!s) return (1);

		/* Nuke the colon, advance to the name */
		*s++ = '\0';
#ifdef JP
		/* Paranoia -- require a name */
		if (!*s) return (1);
#endif
		/* Get the index */
		i = atoi(buf + 2);

		/* Verify information */
		if (i < error_idx) return (4);

		/* Verify information */
		if (i >= head->info_num) return (2);

		/* Save the index */
		error_idx = i;

		/* Point at the "info" */
		d_ptr = &d_info[i];
#ifdef JP
		/* Store the name */
		if (!add_name(&d_ptr->name, head, s)) return (7);
#endif
	}

#ifdef JP
	else if (buf[0] == 'E') return (0);
#else
	else if (buf[0] == 'E')
	{
		/* Acquire the Text */
		s = buf + 2;

		/* Store the name */
		if (!add_name(&d_ptr->name, head, s)) return (7);
	}
#endif

	/* Process 'D' for "Description */
	else if (buf[0] == 'D')
	{
#ifdef JP
		if (buf[2] == '$')
			return (0);
		/* Acquire the text */
		s = buf + 2;
#else
		if (buf[2] != '$')
			return (0);
		/* Acquire the text */
		s = buf + 3;
#endif

		/* Store the text */
		if (!add_text(&d_ptr->text, head, s, TRUE)) return (7);
	}

	/* Process 'W' for "More Info" (one line only) */
	else if (buf[0] == 'W')
	{
		int min_lev, max_lev;
		int min_plev, mode;
		int min_alloc, max_chance;
		int obj_good, obj_great;
		int pit, nest;

		/* Scan for the values */
		if (10 != sscanf(buf + 2, "%d:%d:%d:%d:%d:%d:%d:%d:%x:%x",
			&min_lev, &max_lev, &min_plev, &mode, &min_alloc, &max_chance, &obj_good, &obj_great, (unsigned int *)&pit, (unsigned int *)&nest)) return (1);

		/* Save the values */
		d_ptr->mindepth = (DEPTH)min_lev;
		d_ptr->maxdepth = (DEPTH)max_lev;
		d_ptr->min_plev = (PLAYER_LEVEL)min_plev;
		d_ptr->mode = (BIT_FLAGS8)mode;
		d_ptr->min_m_alloc_level = min_alloc;
		d_ptr->max_m_alloc_chance = max_chance;
		d_ptr->obj_good = obj_good;
		d_ptr->obj_great = obj_great;
		d_ptr->pit = (BIT_FLAGS16)pit;
		d_ptr->nest = (BIT_FLAGS16)nest;
	}

	/* Process 'P' for "Place Info" */
	else if (buf[0] == 'P')
	{
		int dy, dx;

		/* Scan for the values */
		if (2 != sscanf(buf + 2, "%d:%d", &dy, &dx)) return (1);

		/* Save the values */
		d_ptr->dy = dy;
		d_ptr->dx = dx;
	}

	/* Process 'L' for "fLoor type" (one line only) */
	else if (buf[0] == 'L')
	{
		char *zz[16];

		/* Scan for the values */
		if (tokenize(buf + 2, DUNGEON_FEAT_PROB_NUM * 2 + 1, zz, 0) != (DUNGEON_FEAT_PROB_NUM * 2 + 1)) return (1);

		/* Save the values */
		for (i = 0; i < DUNGEON_FEAT_PROB_NUM; i++)
		{
			d_ptr->floor[i].feat = f_tag_to_index(zz[i * 2]);
			if (d_ptr->floor[i].feat < 0) return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;

			d_ptr->floor[i].percent = (PERCENTAGE)atoi(zz[i * 2 + 1]);
		}
		d_ptr->tunnel_percent = atoi(zz[DUNGEON_FEAT_PROB_NUM * 2]);
	}

	/* Process 'A' for "wAll type" (one line only) */
	else if (buf[0] == 'A')
	{
		char *zz[16];

		/* Scan for the values */
		if (tokenize(buf + 2, DUNGEON_FEAT_PROB_NUM * 2 + 4, zz, 0) != (DUNGEON_FEAT_PROB_NUM * 2 + 4)) return (1);

		/* Save the values */
		for (i = 0; i < DUNGEON_FEAT_PROB_NUM; i++)
		{
			d_ptr->fill[i].feat = f_tag_to_index(zz[i * 2]);
			if (d_ptr->fill[i].feat < 0) return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;

			d_ptr->fill[i].percent = (PERCENTAGE)atoi(zz[i * 2 + 1]);
		}

		d_ptr->outer_wall = f_tag_to_index(zz[DUNGEON_FEAT_PROB_NUM * 2]);
		if (d_ptr->outer_wall < 0) return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;

		d_ptr->inner_wall = f_tag_to_index(zz[DUNGEON_FEAT_PROB_NUM * 2 + 1]);
		if (d_ptr->inner_wall < 0) return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;

		d_ptr->stream1 = f_tag_to_index(zz[DUNGEON_FEAT_PROB_NUM * 2 + 2]);
		if (d_ptr->stream1 < 0) return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;

		d_ptr->stream2 = f_tag_to_index(zz[DUNGEON_FEAT_PROB_NUM * 2 + 3]);
		if (d_ptr->stream2 < 0) return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
	}

	/* Process 'F' for "Dungeon Flags" (multiple lines) */
	else if (buf[0] == 'F')
	{
		int artif = 0, monst = 0;

		/* Parse every entry */
		for (s = buf + 2; *s; )
		{
			/* Find the end of this entry */
			for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

			/* Nuke and skip any dividers */
			if (*t)
			{
				*t++ = '\0';
				while (*t == ' ' || *t == '|') t++;
			}

			/* Hack -- Read Final Artifact */
			if (1 == sscanf(s, "FINAL_ARTIFACT_%d", &artif))
			{
				/* Extract a "Final Artifact" */
				d_ptr->final_artifact = (ARTIFACT_IDX)artif;

				/* Start at next entry */
				s = t;
				continue;
			}

			/* Hack -- Read Final Object */
			if (1 == sscanf(s, "FINAL_OBJECT_%d", &artif))
			{
				/* Extract a "Final Artifact" */
				d_ptr->final_object = (KIND_OBJECT_IDX)artif;

				/* Start at next entry */
				s = t;
				continue;
			}

			/* Hack -- Read Artifact Guardian */
			if (1 == sscanf(s, "FINAL_GUARDIAN_%d", &monst))
			{
				/* Extract a "Artifact Guardian" */
				d_ptr->final_guardian = (MONRACE_IDX)monst;

				/* Start at next entry */
				s = t;
				continue;
			}

			/* Hack -- Read Special Percentage */
			if (1 == sscanf(s, "MONSTER_DIV_%d", &monst))
			{
				/* Extract a "Special %" */
				d_ptr->special_div = (PROB)monst;

				/* Start at next entry */
				s = t;
				continue;
			}

			/* Parse this entry */
			if (0 != grab_one_dungeon_flag(d_ptr, s)) return (5);

			/* Start the next entry */
			s = t;
		}
	}

	/* Process 'M' for "Basic Flags" (multiple lines) */
	else if (buf[0] == 'M')
	{
		/* Parse every entry */
		for (s = buf + 2; *s; )
		{
			/* Find the end of this entry */
			for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

			/* Nuke and skip any dividers */
			if (*t)
			{
				*t++ = '\0';
				while (*t == ' ' || *t == '|') t++;
			}

			/* Hack -- Read monster symbols */
			if (!strncmp(s, "R_CHAR_", 7))
			{
				/* Skip "R_CHAR_" */
				s += 7;

				/* Read a string */
				strncpy(d_ptr->r_char, s, sizeof(d_ptr->r_char));

				/* Start at next entry */
				s = t;
				continue;
			}

			/* Parse this entry */
			if (0 != grab_one_basic_monster_flag(d_ptr, s)) return (5);

			/* Start the next entry */
			s = t;
		}
	}

	/* Process 'S' for "Spell Flags" (multiple lines) */
	else if (buf[0] == 'S')
	{
		/* Parse every entry */
		for (s = buf + 2; *s; )
		{
			/* Find the end of this entry */
			for (t = s; *t && (*t != ' ') && (*t != '|'); ++t) /* loop */;

			/* Nuke and skip any dividers */
			if (*t)
			{
				*t++ = '\0';
				while ((*t == ' ') || (*t == '|')) t++;
			}

			/* Hack -- Read spell frequency */
			if (1 == sscanf(s, "1_IN_%d", &i))
			{
				/* Start at next entry */
				s = t;
				continue;
			}

			/* Parse this entry */
			if (0 != grab_one_spell_monster_flag(d_ptr, s)) return (5);

			/* Start the next entry */
			s = t;
		}
	}

	else return (6);

	/* Success */
	return (0);
}


#else	/* ALLOW_TEMPLATES */

#ifdef MACINTOSH
static int i = 0;
#endif

#endif	/* ALLOW_TEMPLATES */


/*!
 * @brief 地形情報の「F:」情報をパースする
 * Process "F:<letter>:<terrain>:<cave_info>:<monster>:<object>:<ego>:<artifact>:<trap>:<special>" -- info for dungeon grid
 * @param buf 解析文字列
 * @return エラーコード
 */
static errr parse_line_feature(char *buf)
{
	int num;
	char *zz[9];

	if (init_flags & INIT_ONLY_BUILDINGS) return (0);

	/* Tokenize the line */
	if ((num = tokenize(buf + 2, 9, zz, 0)) > 1)
	{
		/* Letter to assign */
		int index = zz[0][0];

		/* Reset the info for the letter */
		letter[index].feature = feat_none;
		letter[index].monster = 0;
		letter[index].object = 0;
		letter[index].ego = 0;
		letter[index].artifact = 0;
		letter[index].trap = feat_none;
		letter[index].cave_info = 0;
		letter[index].special = 0;
		letter[index].random = RANDOM_NONE;

		switch (num)
		{
			/* Special */
		case 9:
			letter[index].special = (s16b)atoi(zz[8]);
			/* Fall through */
		/* Trap */
		case 8:
			if ((zz[7][0] == '*') && !zz[7][1])
			{
				letter[index].random |= RANDOM_TRAP;
			}
			else
			{
				letter[index].trap = f_tag_to_index(zz[7]);
				if (letter[index].trap < 0) return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
			}
			/* Fall through */
		/* Artifact */
		case 7:
			if (zz[6][0] == '*')
			{
				letter[index].random |= RANDOM_ARTIFACT;
				if (zz[6][1]) letter[index].artifact = (IDX)atoi(zz[6] + 1);
			}
			else if (zz[6][0] == '!')
			{
				if (p_ptr->inside_quest)
				{
					letter[index].artifact = quest[p_ptr->inside_quest].k_idx;
				}
			}
			else
			{
				letter[index].artifact = (IDX)atoi(zz[6]);
			}
			/* Fall through */
		/* Ego-item */
		case 6:
			if (zz[5][0] == '*')
			{
				letter[index].random |= RANDOM_EGO;
				if (zz[5][1]) letter[index].ego = (IDX)atoi(zz[5] + 1);
			}
			else
			{
				letter[index].ego = (IDX)atoi(zz[5]);
			}
			/* Fall through */
		/* Object */
		case 5:
			if (zz[4][0] == '*')
			{
				letter[index].random |= RANDOM_OBJECT;
				if (zz[4][1]) letter[index].object = (IDX)atoi(zz[4] + 1);
			}
			else if (zz[4][0] == '!')
			{
				if (p_ptr->inside_quest)
				{
					ARTIFACT_IDX a_idx = quest[p_ptr->inside_quest].k_idx;
					if (a_idx)
					{
						artifact_type *a_ptr = &a_info[a_idx];
						if (!(a_ptr->gen_flags & TRG_INSTA_ART))
						{
							letter[index].object = lookup_kind(a_ptr->tval, a_ptr->sval);
						}
					}
				}
			}
			else
			{
				letter[index].object = (IDX)atoi(zz[4]);
			}
			/* Fall through */
		/* Monster */
		case 4:
			if (zz[3][0] == '*')
			{
				letter[index].random |= RANDOM_MONSTER;
				if (zz[3][1]) letter[index].monster = (IDX)atoi(zz[3] + 1);
			}
			else if (zz[3][0] == 'c')
			{
				if (!zz[3][1]) return PARSE_ERROR_GENERIC;
				letter[index].monster = -atoi(zz[3] + 1);
			}
			else
			{
				letter[index].monster = (IDX)atoi(zz[3]);
			}
			/* Fall through */
		/* Cave info */
		case 3:
			letter[index].cave_info = atoi(zz[2]);
			/* Fall through */
		/* Feature */
		case 2:
			if ((zz[1][0] == '*') && !zz[1][1])
			{
				letter[index].random |= RANDOM_FEATURE;
			}
			else
			{
				letter[index].feature = f_tag_to_index(zz[1]);
				if (letter[index].feature < 0) return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
			}
			break;
		}

		return (0);
	}

	return (1);
}


/*!
 * @brief 地形情報の「B:」情報をパースする
 * Process "B:<Index>:<Command>:..." -- Building definition
 * @param buf 解析文字列
 * @return エラーコード
 */
static errr parse_line_building(char *buf)
{
	int i;
	char *zz[1000];
	int index;
	char *s;

#ifdef JP
	if (buf[2] == '$')
		return 0;
	s = buf + 2;
#else
	if (buf[2] != '$')
		return 0;
	s = buf + 3;
#endif
	/* Get the building number */
	index = atoi(s);

	/* Find the colon after the building number */
	s = my_strchr(s, ':');

	/* Verify that colon */
	if (!s) return (1);

	/* Nuke the colon, advance to the sub-index */
	*s++ = '\0';

	/* Paranoia -- require a sub-index */
	if (!*s) return (1);

	/* Building definition sub-index */
	switch (s[0])
	{
		/* Building name, owner, race */
	case 'N':
	{
		if (tokenize(s + 2, 3, zz, 0) == 3)
		{
			/* Name of the building */
			strcpy(building[index].name, zz[0]);

			/* Name of the owner */
			strcpy(building[index].owner_name, zz[1]);

			/* Race of the owner */
			strcpy(building[index].owner_race, zz[2]);

			break;
		}

		return (PARSE_ERROR_TOO_FEW_ARGUMENTS);
	}

	/* Building Action */
	case 'A':
	{
		if (tokenize(s + 2, 8, zz, 0) >= 7)
		{
			/* Index of the action */
			int action_index = atoi(zz[0]);

			/* Name of the action */
			strcpy(building[index].act_names[action_index], zz[1]);

			/* Cost of the action for members */
			building[index].member_costs[action_index] = (PRICE)atoi(zz[2]);

			/* Cost of the action for non-members */
			building[index].other_costs[action_index] = (PRICE)atoi(zz[3]);

			/* Letter assigned to the action */
			building[index].letters[action_index] = zz[4][0];

			/* Action code */
			building[index].actions[action_index] = (BACT_IDX)atoi(zz[5]);

			/* Action restriction */
			building[index].action_restr[action_index] = (BACT_RESTRICT_IDX)atoi(zz[6]);

			break;
		}

		return (PARSE_ERROR_TOO_FEW_ARGUMENTS);
	}

	/* Building Classes */
	case 'C':
	{
		int n;
		n = tokenize(s + 2, MAX_CLASS, zz, 0);
		for (i = 0; i < MAX_CLASS; i++)
		{
			building[index].member_class[i] = ((i < n) ? (CLASS_IDX)atoi(zz[i]) : 1);
		}
		break;
	}

	/* Building Races */
	case 'R':
	{
		int n;
		n = tokenize(s + 2, MAX_RACES, zz, 0);
		for (i = 0; i < MAX_RACES; i++)
		{
			building[index].member_race[i] = ((i < n) ? (RACE_IDX)atoi(zz[i]) : 1);
		}
		break;
	}

	/* Building Realms */
	case 'M':
	{
		int n;
		n = tokenize(s + 2, MAX_MAGIC, zz, 0);
		for (i = 0; i < MAX_MAGIC; i++)
		{
			building[index].member_realm[i + 1] = ((i < n) ? (REALM_IDX)atoi(zz[i]) : 1);
		}
		break;
	}

	case 'Z':
	{
		/* Ignore scripts */
		break;
	}

	default:
	{
		return (PARSE_ERROR_UNDEFINED_DIRECTIVE);
	}
	}

	return (0);
}


/*!
 * @brief フロアの所定のマスにオブジェクトを配置する
 * Place the object j_ptr to a grid
 * @param j_ptr オブジェクト構造体の参照ポインタ
 * @param y 配置先Y座標
 * @param x 配置先X座標
 * @return エラーコード
 */
static void drop_here(object_type *j_ptr, POSITION y, POSITION x)
{
	grid_type *g_ptr = &current_floor_ptr->grid_array[y][x];
	object_type *o_ptr;

	OBJECT_IDX o_idx = o_pop();

	/* Access new object */
	o_ptr = &current_floor_ptr->o_list[o_idx];

	/* Structure copy */
	object_copy(o_ptr, j_ptr);

	/* Locate */
	o_ptr->iy = y;
	o_ptr->ix = x;

	/* No monster */
	o_ptr->held_m_idx = 0;

	/* Build a stack */
	o_ptr->next_o_idx = g_ptr->o_idx;

	g_ptr->o_idx = o_idx;
}

/*!
 * @brief クエスト用固定ダンジョンをフロアに生成する
 * Parse a sub-file of the "extra info"
 * @param buf 文字列
 * @param ymin 詳細不明
 * @param xmin 詳細不明
 * @param ymax 詳細不明
 * @param xmax 詳細不明
 * @param y 詳細不明
 * @param x 詳細不明
 * @return エラーコード
 */
static errr process_dungeon_file_aux(char *buf, int ymin, int xmin, int ymax, int xmax, int *y, int *x)
{
	int i;
	char *zz[33];

	/* Skip "empty" lines */
	if (!buf[0]) return (0);

	/* Skip "blank" lines */
	if (iswspace(buf[0])) return (0);

	/* Skip comments */
	if (buf[0] == '#') return (0);

	/* Require "?:*" format */
	if (buf[1] != ':') return (1);


	/* Process "%:<fname>" */
	if (buf[0] == '%')
	{
		/* Attempt to Process the given file */
		return (process_dungeon_file(buf + 2, ymin, xmin, ymax, xmax));
	}

	/* Process "F:<letter>:<terrain>:<cave_info>:<monster>:<object>:<ego>:<artifact>:<trap>:<special>" -- info for dungeon grid */
	if (buf[0] == 'F')
	{
		return parse_line_feature(buf);
	}

	/* Process "D:<dungeon>" -- info for the current_floor_ptr->grid_array grids */
	else if (buf[0] == 'D')
	{
		object_type object_type_body;

		/* Acquire the text */
		char *s = buf + 2;

		/* Length of the text */
		int len = strlen(s);

		if (init_flags & INIT_ONLY_BUILDINGS) return (0);

		for (*x = xmin, i = 0; ((*x < xmax) && (i < len)); (*x)++, s++, i++)
		{
			grid_type *g_ptr = &current_floor_ptr->grid_array[*y][*x];

			int idx = s[0];

			OBJECT_IDX object_index = letter[idx].object;
			MONSTER_IDX monster_index = letter[idx].monster;
			int random = letter[idx].random;
			ARTIFACT_IDX artifact_index = letter[idx].artifact;

			/* Lay down a floor */
			g_ptr->feat = conv_dungeon_feat(letter[idx].feature);

			/* Only the features */
			if (init_flags & INIT_ONLY_FEATURES) continue;

			/* Cave info */
			g_ptr->info = letter[idx].cave_info;

			/* Create a monster */
			if (random & RANDOM_MONSTER)
			{
				current_floor_ptr->monster_level = current_floor_ptr->base_level + monster_index;

				place_monster(*y, *x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));

				current_floor_ptr->monster_level = current_floor_ptr->base_level;
			}
			else if (monster_index)
			{
				int old_cur_num, old_max_num;
				bool clone = FALSE;

				if (monster_index < 0)
				{
					monster_index = -monster_index;
					clone = TRUE;
				}
				old_cur_num = r_info[monster_index].cur_num;
				old_max_num = r_info[monster_index].max_num;

				/* Make alive again */
				if (r_info[monster_index].flags1 & RF1_UNIQUE)
				{
					r_info[monster_index].cur_num = 0;
					r_info[monster_index].max_num = 1;
				}

				/* Make alive again */
				/* Hack -- Non-unique Nazguls are semi-unique */
				else if (r_info[monster_index].flags7 & RF7_NAZGUL)
				{
					if (r_info[monster_index].cur_num == r_info[monster_index].max_num)
					{
						r_info[monster_index].max_num++;
					}
				}

				/* Place it */
				place_monster_aux(0, *y, *x, monster_index, (PM_ALLOW_SLEEP | PM_NO_KAGE));
				if (clone)
				{
					/* clone */
					current_floor_ptr->m_list[hack_m_idx_ii].smart |= SM_CLONED;

					/* Make alive again for real unique monster */
					r_info[monster_index].cur_num = old_cur_num;
					r_info[monster_index].max_num = old_max_num;
				}
			}

			/* Object (and possible trap) */
			if ((random & RANDOM_OBJECT) && (random & RANDOM_TRAP))
			{
				current_floor_ptr->object_level = current_floor_ptr->base_level + object_index;

				/*
				 * Random trap and random treasure defined
				 * 25% chance for trap and 75% chance for object
				 */
				if (randint0(100) < 75)
				{
					place_object(*y, *x, 0L);
				}
				else
				{
					place_trap(*y, *x);
				}

				current_floor_ptr->object_level = current_floor_ptr->base_level;
			}
			else if (random & RANDOM_OBJECT)
			{
				current_floor_ptr->object_level = current_floor_ptr->base_level + object_index;

				/* Create an out of deep object */
				if (randint0(100) < 75)
					place_object(*y, *x, 0L);
				else if (randint0(100) < 80)
					place_object(*y, *x, AM_GOOD);
				else
					place_object(*y, *x, AM_GOOD | AM_GREAT);

				current_floor_ptr->object_level = current_floor_ptr->base_level;
			}
			/* Random trap */
			else if (random & RANDOM_TRAP)
			{
				place_trap(*y, *x);
			}
			/* Hidden trap (or door) */
			else if (letter[idx].trap)
			{
				g_ptr->mimic = g_ptr->feat;
				g_ptr->feat = conv_dungeon_feat(letter[idx].trap);
			}
			else if (object_index)
			{
				object_type *o_ptr = &object_type_body;
				object_prep(o_ptr, object_index);

				if (o_ptr->tval == TV_GOLD)
				{
					coin_type = object_index - OBJ_GOLD_LIST;
					make_gold(o_ptr);
					coin_type = 0;
				}

				/* Apply magic (no messages, no artifacts) */
				apply_magic(o_ptr, current_floor_ptr->base_level, AM_NO_FIXED_ART | AM_GOOD);

				drop_here(o_ptr, *y, *x);
			}

			/* Artifact */
			if (artifact_index)
			{
				if (a_info[artifact_index].cur_num)
				{
					KIND_OBJECT_IDX k_idx = lookup_kind(TV_SCROLL, SV_SCROLL_ACQUIREMENT);
					object_type forge;
					object_type *q_ptr = &forge;

					object_prep(q_ptr, k_idx);
					drop_here(q_ptr, *y, *x);
				}
				else
				{
					if (create_named_art(artifact_index, *y, *x))
						a_info[artifact_index].cur_num = 1;
				}
			}

			/* Terrain special */
			g_ptr->special = letter[idx].special;
		}

		(*y)++;

		return (0);
	}

	/* Process "Q:<number>:<command>:... -- quest info */
	else if (buf[0] == 'Q')
	{
		int num;
		quest_type *q_ptr;
#ifdef JP
		if (buf[2] == '$')
			return 0;
		num = tokenize(buf + 2, 33, zz, 0);
#else
		if (buf[2] != '$')
			return 0;
		num = tokenize(buf + 3, 33, zz, 0);
#endif

		/* Have we enough parameters? */
		if (num < 3) return (PARSE_ERROR_TOO_FEW_ARGUMENTS);

		/* Get the quest */
		q_ptr = &(quest[atoi(zz[0])]);

		/* Process "Q:<q_index>:Q:<type>:<num_mon>:<cur_num>:<max_num>:<level>:<r_idx>:<k_idx>:<flags>" -- quest info */
		if (zz[1][0] == 'Q')
		{
			if (init_flags & INIT_ASSIGN)
			{
				monster_race *r_ptr;
				artifact_type *a_ptr;

				if (num < 9) return (PARSE_ERROR_TOO_FEW_ARGUMENTS);

				q_ptr->type = (QUEST_TYPE)atoi(zz[2]);
				q_ptr->num_mon = (MONSTER_NUMBER)atoi(zz[3]);
				q_ptr->cur_num = (MONSTER_NUMBER)atoi(zz[4]);
				q_ptr->max_num = (MONSTER_NUMBER)atoi(zz[5]);
				q_ptr->level = (DEPTH)atoi(zz[6]);
				q_ptr->r_idx = (IDX)atoi(zz[7]);
				q_ptr->k_idx = (IDX)atoi(zz[8]);
				q_ptr->dungeon = (DUNGEON_IDX)atoi(zz[9]);

				if (num > 10) q_ptr->flags = atoi(zz[10]);

				r_ptr = &r_info[q_ptr->r_idx];
				if (r_ptr->flags1 & RF1_UNIQUE)
					r_ptr->flags1 |= RF1_QUESTOR;

				a_ptr = &a_info[q_ptr->k_idx];
				a_ptr->gen_flags |= TRG_QUESTITEM;
			}
			return (0);
		}

		else if (zz[1][0] == 'R')
		{
			if (init_flags & INIT_ASSIGN)
			{
				int count = 0;
				IDX idx, reward_idx = 0;

				for (idx = 2; idx < num; idx++)
				{
					IDX a_idx = (IDX)atoi(zz[idx]);
					if (a_idx < 1) continue;
					if (a_info[a_idx].cur_num > 0) continue;
					count++;
					if (one_in_(count)) reward_idx = a_idx;
				}

				if (reward_idx)
				{
					/* Set quest's rewarding artifact */
					q_ptr->k_idx = reward_idx;
					a_info[reward_idx].gen_flags |= TRG_QUESTITEM;
				}
				else
				{
					/* Change a quest type to KILL_ALL when all artifact of reward list are got */
					q_ptr->type = QUEST_TYPE_KILL_ALL;
				}
			}

			return (0);
		}

		/* Process "Q:<q_index>:N:<name>" -- quest name */
		else if (zz[1][0] == 'N')
		{
			if (init_flags & (INIT_ASSIGN | INIT_SHOW_TEXT | INIT_NAME_ONLY))
			{
				strcpy(q_ptr->name, zz[2]);
			}

			return (0);
		}

		/* Process "Q:<q_index>:T:<text>" -- quest description line */
		else if (zz[1][0] == 'T')
		{
			if (init_flags & INIT_SHOW_TEXT)
			{
				strcpy(quest_text[quest_text_line], zz[2]);
				quest_text_line++;
			}

			return (0);
		}
	}

	/* Process "W:<command>: ..." -- info for the wilderness */
	else if (buf[0] == 'W')
	{
		return parse_line_wilderness(buf, ymin, xmin, ymax, xmax, y, x);
	}

	/* Process "P:<y>:<x>" -- player position */
	else if (buf[0] == 'P')
	{
		if (init_flags & INIT_CREATE_DUNGEON)
		{
			if (tokenize(buf + 2, 2, zz, 0) == 2)
			{
				int panels_x, panels_y;

				/* Hack - Set the dungeon size */
				panels_y = (*y / SCREEN_HGT);
				if (*y % SCREEN_HGT) panels_y++;
				current_floor_ptr->height = panels_y * SCREEN_HGT;

				panels_x = (*x / SCREEN_WID);
				if (*x % SCREEN_WID) panels_x++;
				current_floor_ptr->width = panels_x * SCREEN_WID;

				/* Assume illegal panel */
				panel_row_min = current_floor_ptr->height;
				panel_col_min = current_floor_ptr->width;

				/* Place player in a quest level */
				if (p_ptr->inside_quest)
				{
					POSITION py, px;

					/* Delete the monster (if any) */
					delete_monster(p_ptr->y, p_ptr->x);

					py = atoi(zz[0]);
					px = atoi(zz[1]);

					p_ptr->y = py;
					p_ptr->x = px;
				}
				/* Place player in the town */
				else if (!p_ptr->oldpx && !p_ptr->oldpy)
				{
					p_ptr->oldpy = atoi(zz[0]);
					p_ptr->oldpx = atoi(zz[1]);
				}
			}
		}

		return (0);
	}

	/* Process "B:<Index>:<Command>:..." -- Building definition */
	else if (buf[0] == 'B')
	{
		return parse_line_building(buf);
	}

	/* Process "M:<type>:<maximum>" -- set maximum values */
	else if (buf[0] == 'M')
	{
		if (tokenize(buf + 2, 2, zz, 0) == 2)
		{
			/* Maximum towns */
			if (zz[0][0] == 'T')
			{
				max_towns = (TOWN_IDX)atoi(zz[1]);
			}

			/* Maximum quests */
			else if (zz[0][0] == 'Q')
			{
				max_q_idx = (QUEST_IDX)atoi(zz[1]);
			}

			/* Maximum r_idx */
			else if (zz[0][0] == 'R')
			{
				max_r_idx = (IDX)atoi(zz[1]);
			}

			/* Maximum k_idx */
			else if (zz[0][0] == 'K')
			{
				max_k_idx = (IDX)atoi(zz[1]);
			}

			/* Maximum v_idx */
			else if (zz[0][0] == 'V')
			{
				max_v_idx = (IDX)atoi(zz[1]);
			}

			/* Maximum f_idx */
			else if (zz[0][0] == 'F')
			{
				max_f_idx = (IDX)atoi(zz[1]);
			}

			/* Maximum a_idx */
			else if (zz[0][0] == 'A')
			{
				max_a_idx = (IDX)atoi(zz[1]);
			}

			/* Maximum e_idx */
			else if (zz[0][0] == 'E')
			{
				max_e_idx = (IDX)atoi(zz[1]);
			}

			/* Maximum d_idx */
			else if (zz[0][0] == 'D')
			{
				max_d_idx = (IDX)atoi(zz[1]);
			}

			/* Maximum o_idx */
			else if (zz[0][0] == 'O')
			{
				current_floor_ptr->max_o_idx = (IDX)atoi(zz[1]);
			}

			/* Maximum m_idx */
			else if (zz[0][0] == 'M')
			{
				current_floor_ptr->max_m_idx = (IDX)atoi(zz[1]);
			}

			/* Wilderness size */
			else if (zz[0][0] == 'W')
			{
				/* Maximum wild_x_size */
				if (zz[0][1] == 'X')
					current_world_ptr->max_wild_x = atoi(zz[1]);
				/* Maximum wild_y_size */
				if (zz[0][1] == 'Y')
					current_world_ptr->max_wild_y = atoi(zz[1]);
			}

			return (0);
		}
	}


	/* Failure */
	return (1);
}


static char tmp[8];
static concptr variant = "ZANGBAND";


/*!
 * @brief クエスト用固定ダンジョン生成時の分岐処理
 * Helper function for "process_dungeon_file()"
 * @param sp
 * @param fp
 * @return エラーコード
 */
static concptr process_dungeon_file_expr(char **sp, char *fp)
{
	concptr v;

	char *b;
	char *s;

	char b1 = '[';
	char b2 = ']';

	char f = ' ';

	/* Initial */
	s = (*sp);

	/* Skip spaces */
	while (iswspace(*s)) s++;

	/* Save start */
	b = s;

	/* Default */
	v = "?o?o?";

	/* Analyze */
	if (*s == b1)
	{
		concptr p;
		concptr t;

		/* Skip b1 */
		s++;

		/* First */
		t = process_dungeon_file_expr(&s, &f);

		if (!*t)
		{
			/* Nothing */
		}

		/* Function: IOR */
		else if (streq(t, "IOR"))
		{
			v = "0";
			while (*s && (f != b2))
			{
				t = process_dungeon_file_expr(&s, &f);
				if (*t && !streq(t, "0")) v = "1";
			}
		}

		/* Function: AND */
		else if (streq(t, "AND"))
		{
			v = "1";
			while (*s && (f != b2))
			{
				t = process_dungeon_file_expr(&s, &f);
				if (*t && streq(t, "0")) v = "0";
			}
		}

		/* Function: NOT */
		else if (streq(t, "NOT"))
		{
			v = "1";
			while (*s && (f != b2))
			{
				t = process_dungeon_file_expr(&s, &f);
				if (*t && streq(t, "1")) v = "0";
			}
		}

		/* Function: EQU */
		else if (streq(t, "EQU"))
		{
			v = "0";
			if (*s && (f != b2))
			{
				t = process_dungeon_file_expr(&s, &f);
			}
			while (*s && (f != b2))
			{
				p = process_dungeon_file_expr(&s, &f);
				if (streq(t, p)) v = "1";
			}
		}

		/* Function: LEQ */
		else if (streq(t, "LEQ"))
		{
			v = "1";
			if (*s && (f != b2))
			{
				t = process_dungeon_file_expr(&s, &f);
			}
			while (*s && (f != b2))
			{
				p = t;
				t = process_dungeon_file_expr(&s, &f);
				if (*t && atoi(p) > atoi(t)) v = "0";
			}
		}

		/* Function: GEQ */
		else if (streq(t, "GEQ"))
		{
			v = "1";
			if (*s && (f != b2))
			{
				t = process_dungeon_file_expr(&s, &f);
			}
			while (*s && (f != b2))
			{
				p = t;
				t = process_dungeon_file_expr(&s, &f);

				/* Compare two numbers instead of string */
				if (*t && atoi(p) < atoi(t)) v = "0";
			}
		}

		else
		{
			while (*s && (f != b2))
			{
				t = process_dungeon_file_expr(&s, &f);
			}
		}

		/* Verify ending */
		if (f != b2) v = "?x?x?";

		/* Extract final and Terminate */
		if ((f = *s) != '\0') *s++ = '\0';
	}

	/* Other */
	else
	{
		/* Accept all printables except spaces and brackets */
#ifdef JP
		while (iskanji(*s) || (isprint(*s) && !my_strchr(" []", *s)))
		{
			if (iskanji(*s)) s++;
			s++;
		}
#else
		while (isprint(*s) && !my_strchr(" []", *s)) ++s;
#endif

		/* Extract final and Terminate */
		if ((f = *s) != '\0') *s++ = '\0';

		/* Variable */
		if (*b == '$')
		{
			/* System */
			if (streq(b + 1, "SYS"))
			{
				v = ANGBAND_SYS;
			}

			/* Graphics */
			else if (streq(b + 1, "GRAF"))
			{
				v = ANGBAND_GRAF;
			}

			else if (streq(b + 1, "MONOCHROME"))
			{
				if (arg_monochrome)
					v = "ON";
				else
					v = "OFF";
			}

			/* Race */
			else if (streq(b + 1, "RACE"))
			{
				v = _(rp_ptr->E_title, rp_ptr->title);
			}

			/* Class */
			else if (streq(b + 1, "CLASS"))
			{
				v = _(cp_ptr->E_title, cp_ptr->title);
			}

			/* First realm */
			else if (streq(b + 1, "REALM1"))
			{
				v = _(E_realm_names[p_ptr->realm1], realm_names[p_ptr->realm1]);
			}

			/* Second realm */
			else if (streq(b + 1, "REALM2"))
			{
				v = _(E_realm_names[p_ptr->realm2], realm_names[p_ptr->realm2]);
			}

			/* Player name */
			else if (streq(b + 1, "PLAYER"))
			{
				static char tmp_player_name[32];
				char *pn, *tpn;
				for (pn = p_ptr->name, tpn = tmp_player_name; *pn; pn++, tpn++)
				{
#ifdef JP
					if (iskanji(*pn))
					{
						*(tpn++) = *(pn++);
						*tpn = *pn;
						continue;
					}
#endif
					*tpn = my_strchr(" []", *pn) ? '_' : *pn;
				}
				*tpn = '\0';
				v = tmp_player_name;
			}

			/* Town */
			else if (streq(b + 1, "TOWN"))
			{
				sprintf(tmp, "%d", p_ptr->town_num);
				v = tmp;
			}

			/* Level */
			else if (streq(b + 1, "LEVEL"))
			{
				sprintf(tmp, "%d", p_ptr->lev);
				v = tmp;
			}

			/* Current quest number */
			else if (streq(b + 1, "QUEST_NUMBER"))
			{
				sprintf(tmp, "%d", p_ptr->inside_quest);
				v = tmp;
			}

			/* Number of last quest */
			else if (streq(b + 1, "LEAVING_QUEST"))
			{
				sprintf(tmp, "%d", leaving_quest);
				v = tmp;
			}

			/* Quest type */
			else if (prefix(b + 1, "QUEST_TYPE"))
			{
				/* "QUEST_TYPE" uses a special parameter to determine the type of the quest */
				sprintf(tmp, "%d", quest[atoi(b + 11)].type);
				v = tmp;
			}

			/* Quest status */
			else if (prefix(b + 1, "QUEST"))
			{
				/* "QUEST" uses a special parameter to determine the number of the quest */
				sprintf(tmp, "%d", quest[atoi(b + 6)].status);
				v = tmp;
			}

			/* Random */
			else if (prefix(b + 1, "RANDOM"))
			{
				/* "RANDOM" uses a special parameter to determine the number of the quest */
				sprintf(tmp, "%d", (int)(current_world_ptr->seed_town%atoi(b + 7)));
				v = tmp;
			}

			/* Variant name */
			else if (streq(b + 1, "VARIANT"))
			{
				v = variant;
			}

			/* Wilderness */
			else if (streq(b + 1, "WILDERNESS"))
			{
				if (vanilla_town)
					sprintf(tmp, "NONE");
				else if (lite_town)
					sprintf(tmp, "LITE");
				else
					sprintf(tmp, "NORMAL");
				v = tmp;
			}
		}

		/* Constant */
		else
		{
			v = b;
		}
	}

	/* Save */
	(*fp) = f;
	(*sp) = s;
	return (v);
}

#if 0
void write_r_info_txt(void)
{
	int i, j, z, fc, bc;
	int dlen;

	concptr flags[32 * 10];

	u32b f_ptr[10];
	concptr *n_ptr[10];

	monster_race *r_ptr;

	monster_blow *b_ptr;

	FILE *fff = fopen("output.txt", "wt");

	concptr desc;

	BIT_FLAGS mode = -1;

	if (!fff) return;

	fprintf(fff, "# File: r_info.txt (autogenerated)\n\n");

	fprintf(fff, "# Version stamp (required)\n\n");

	/* Write Version */
	fprintf(fff, "V:%d.%d.%d\n\n\n", r_head->v_major, r_head->v_minor, r_head->v_patch);

	/* Write a note */
	fprintf(fff, "##### The Player #####\n\n");

	for (z = -1; z < alloc_race_size; z++)
	{
		/* Output the monsters in order */
		i = (z >= 0) ? alloc_race_table[z].index : 0;

		/* Acquire the monster */
		r_ptr = &r_info[i];

		/* Ignore empty monsters */
		if (!strlen(r_name + r_ptr->name)) continue;

		/* Ignore useless monsters */
		if (i && !r_ptr->speed) continue;

		/* Write a note if necessary */
		if (i && (!r_ptr->level != !mode))
		{
			/* Note the town */
			if (!r_ptr->level)
			{
				fprintf(fff, "\n##### Town monsters #####\n\n");
			}
			/* Note the dungeon */
			else
			{
				fprintf(fff, "\n##### Normal monsters #####\n\n");
			}

			/* Record the change */
			mode = r_ptr->level;
		}

		/* Acquire the flags */
		f_ptr[0] = r_ptr->flags1; n_ptr[0] = r_info_flags1;
		f_ptr[1] = r_ptr->flags2; n_ptr[1] = r_info_flags2;
		f_ptr[2] = r_ptr->flags3; n_ptr[2] = r_info_flags3;
		f_ptr[3] = r_ptr->flags4; n_ptr[3] = r_info_flags4;
		f_ptr[4] = r_ptr->a_ability_flags1; n_ptr[4] = r_a_ability_flags1;
		f_ptr[5] = r_ptr->a_ability_flags2; n_ptr[5] = r_a_ability_flags2;
		f_ptr[6] = r_ptr->flags7; n_ptr[6] = r_info_flags7;
		f_ptr[7] = r_ptr->flags8; n_ptr[7] = r_info_flags8;
		f_ptr[8] = r_ptr->flags9; n_ptr[8] = r_info_flags9;
		f_ptr[9] = r_ptr->flagsr; n_ptr[9] = r_info_flagsr;

		/* Write New/Number/Name */
		fprintf(fff, "N:%d:%s\n", z + 1, r_name + r_ptr->name);

		/* Write Graphic */
		fprintf(fff, "G:%c:%c\n", r_ptr->d_char, color_char[r_ptr->d_attr]);

		/* Write Information */
		fprintf(fff, "I:%d:%dd%d:%d:%d:%d\n", r_ptr->speed, r_ptr->hdice, r_ptr->hside,
			r_ptr->aaf, r_ptr->ac, r_ptr->sleep);

		/* Write more information */
		fprintf(fff, "W:%d:%d:%d:%ld\n", r_ptr->level, r_ptr->rarity, r_ptr->extra, r_ptr->mexp);

		/* Write Blows */
		for (j = 0; j < 4; j++)
		{
			b_ptr = &(r_ptr->blow[j]);

			/* Stop when done */
			if (!b_ptr->method) break;

			/* Write the blows */
			fprintf(fff, "B:%s:%s:%dd%d\n", r_info_blow_method[b_ptr->method],
				r_info_blow_effect[b_ptr->effect],
				b_ptr->d_dice, b_ptr->d_side);
		}
		for (fc = 0, j = 0; j < 32 * 3; j++)
		{
			/* Check this flag */
			if (f_ptr[j / 32] & (1L << (j % 32))) flags[fc++] = n_ptr[j / 32][j % 32];
		}

		/* Extract the extra flags */
		for (j = 32 * 6; j < 32 * 10; j++)
		{
			/* Check this flag */
			if (f_ptr[j / 32] & (1L << (j % 32))) flags[fc++] = n_ptr[j / 32][j % 32];
		}

		/* Write the flags */
		for (j = 0; j < fc;)
		{
			char buf[160];

			/* Start the line */
			sprintf(buf, "F:");

			for (bc = 0; (bc < 60) && (j < fc); j++)
			{
				char t[80];

				/* Format the flag */
				sprintf(t, "%s%s", flags[j], (j < fc - 1) ? " | " : "");

				/* Add it to the buffer */
				strcat(buf, t);

				/* Note the length */
				bc += strlen(t);
			}

			/* Done with this line; write it */
			fprintf(fff, "%s\n", buf);
		}

		/* Write Spells if applicable */
		if (r_ptr->freq_spell)
		{
			/* Write the frequency */
			fprintf(fff, "S:1_IN_%d | \n", 100 / r_ptr->freq_spell);

			/* Extract the spell flags */
			for (fc = 0, j = 96; j < 192; j++)
			{
				/* Check this flag */
				if (f_ptr[j / 32] & (1L << (j % 32))) flags[fc++] = n_ptr[j / 32][j % 32];
			}

			/* Write the flags */
			for (j = 0; j < fc;)
			{
				char buf[160], *t;

				/* Start the line */
				sprintf(buf, "S:");

				for (bc = 0, t = buf + 2; (bc < 60) && (j < fc); j++)
				{
					int tlen;

					/* Format the flag */
					sprintf(t, "%s%s", flags[j], (j < fc - 1) ? " | " : "");

					tlen = strlen(t);

					/* Note the length */
					bc += tlen;

					/* Advance */
					t += tlen;
				}

				/* Done with this line; write it */
				fprintf(fff, "%s\n", buf);
			}
		}

		/* Acquire the description */
		desc = r_text + r_ptr->text;
		dlen = strlen(desc);

		/* Write Description */
		for (j = 0; j < dlen;)
		{
			char buf[160], *t;

			/* Start the line */
			sprintf(buf, "D:");

			for (bc = 0, t = buf + 2; ((bc < 60) || !iswspace(desc[j])) && (j < dlen); j++, bc++, t++)
			{
				*t = desc[j];
			}

			/* Terminate it */
			*t = '\0';

			/* Done with this line; write it */
			fprintf(fff, "%s\n", buf);
		}

		/* Space between entries */
		fprintf(fff, "\n");
	}

	fclose(fff);
}

#endif


/*!
 * @brief クエスト用固定ダンジョン生成時のメインルーチン
 * Helper function for "process_dungeon_file()"
 * @param name ファイル名
 * @param ymin 詳細不明
 * @param xmin 詳細不明
 * @param ymax 詳細不明
 * @param xmax 詳細不明
 * @return エラーコード
 */
errr process_dungeon_file(concptr name, int ymin, int xmin, int ymax, int xmax)
{
	FILE *fp;
	char buf[1024];
	int num = -1;
	errr err = 0;
	bool bypass = FALSE;
	int x = xmin, y = ymin;


	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_EDIT, name);

	/* Open the file */
	fp = my_fopen(buf, "r");

	/* No such file */
	if (!fp) return (-1);


	/* Process the file */
	while (0 == my_fgets(fp, buf, sizeof(buf)))
	{
		/* Count lines */
		num++;


		/* Skip "empty" lines */
		if (!buf[0]) continue;

		/* Skip "blank" lines */
		if (iswspace(buf[0])) continue;

		/* Skip comments */
		if (buf[0] == '#') continue;


		/* Process "?:<expr>" */
		if ((buf[0] == '?') && (buf[1] == ':'))
		{
			char f;
			concptr v;
			char *s;

			/* Start */
			s = buf + 2;

			/* Parse the expr */
			v = process_dungeon_file_expr(&s, &f);

			/* Set flag */
			bypass = (streq(v, "0") ? TRUE : FALSE);
			continue;
		}

		/* Apply conditionals */
		if (bypass) continue;

		/* Process the line */
		err = process_dungeon_file_aux(buf, ymin, xmin, ymax, xmax, &y, &x);

		if (err) break;
	}

	/* Errors */
	if (err)
	{
		concptr oops;

		/* Error string */
		oops = (((err > 0) && (err < PARSE_ERROR_MAX)) ? err_str[err] : "unknown");

		msg_format("Error %d (%s) at line %d of '%s'.", err, oops, num, name);
		msg_format(_("'%s'を解析中。", "Parsing '%s'."), buf);

		msg_print(NULL);
	}

	my_fclose(fp);
	return (err);
}


