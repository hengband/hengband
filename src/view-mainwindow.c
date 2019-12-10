/*!
 * @file xtra1.c
 * @brief プレイヤーのステータス処理 / status
 * @date 2018/09/25
 * @author
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research, and\n
 * not for profit purposes provided that this copyright and statement are\n
 * included in all such copies.\n
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "angband.h"
#include "util.h"
#include "autopick.h"
#include "term.h"

#include "bldg.h"
#include "core.h"
#include "files.h"
#include "world.h"
#include "quest.h"
#include "artifact.h"
#include "avatar.h"
#include "player-status.h"
#include "player-class.h"
#include "player-race.h"
#include "player-effects.h"
#include "player-inventory.h"
#include "monster.h"
#include "view-mainwindow.h"

#include "spells.h"
#include "realm-hex.h"
#include "realm-song.h"

#include "object-flavor.h"

#include "grid.h"
#include "dungeon.h"
#include "floor.h"
#include "floor-town.h"
#include "feature.h"
#include "objectkind.h"
#include "targeting.h"

 /*
  * Not using graphical tiles for this feature?
  */
#define IS_ASCII_GRAPHICS(A) (!((A) & 0x80))


static int feat_priority; /*!< マップ縮小表示時に表示すべき地形の優先度を保管する */
static byte display_autopick; /*!< 自動拾い状態の設定フラグ */
static int match_autopick;
static object_type *autopick_obj; /*!< 各種自動拾い処理時に使うオブジェクトポインタ */

/*
 * Dungeon size info
 */
POSITION panel_row_min, panel_row_max;
POSITION panel_col_min, panel_col_max;
POSITION panel_col_prt, panel_row_prt;

 /*
  * Some screen locations for various display routines
  * Currently, row 8 and 15 are the only "blank" rows.
  * That leaves a "border" around the "stat" values.
  */

#define ROW_RACE                1
#define COL_RACE                0       /* <race name> */

  /*#define ROW_CLASS               2 */
  /*#define COL_CLASS               0 */      /* <class name> */

#define ROW_TITLE               2
#define COL_TITLE               0       /* <title> or <mode> */

/*#define ROW_SEIKAKU		4 */
/*#define COL_SEIKAKU      	0*/	/* <seikaku> */

#define ROW_DAY                 21
#define COL_DAY                 0       /* day */

#define ROW_DUNGEON             22
#define COL_DUNGEON             0       /* dungeon */

#define ROW_LEVEL               3
#define COL_LEVEL               0       /* "LEVEL xxxxxx" */

#define ROW_EXP                 4
#define COL_EXP                 0       /* "EXP xxxxxxxx" */

#define ROW_GOLD                5
#define COL_GOLD                0       /* "AU xxxxxxxxx" */

#define ROW_EQUIPPY             6
#define COL_EQUIPPY             0       /* equippy chars */

#define ROW_STAT                7
#define COL_STAT                0       /* "xxx   xxxxxx" */

#define ROW_AC                  13
#define COL_AC                  0       /* "Cur AC xxxxx" */

#define ROW_HPMP                14
#define COL_HPMP                0

#define ROW_CURHP               14
#define COL_CURHP               0       /* "Cur HP xxxxx" */

#define ROW_CURSP               15
#define COL_CURSP               0       /* "Cur SP xxxxx" */

#define ROW_RIDING_INFO         16
#define COL_RIDING_INFO         0       /* "xxxxxxxxxxxx" */

#define ROW_INFO                17
#define COL_INFO                0       /* "xxxxxxxxxxxx" */

#define ROW_CUT                 18
#define COL_CUT                 0       /* <cut> */

#define ROW_STUN                19
#define COL_STUN                0       /* <stun> */

#define ROW_HUNGRY              20
#define COL_HUNGRY              0       /* "Weak" / "Hungry" / "Full" / "Gorged" */

#define ROW_STATE               20
#define COL_STATE                7      /* <state> */

#define ROW_SPEED               (-1)
#define COL_SPEED               (-24)      /* "Slow (-NN)" or "Fast (+NN)" */

#define ROW_STUDY               (-1)
#define COL_STUDY               (-13)      /* "Study" */

#define ROW_DEPTH               (-1)
#define COL_DEPTH               (-8)      /* "Lev NNN" / "NNNN ft" */

#define ROW_STATBAR             (-1)
#define COL_STATBAR              0
#define MAX_COL_STATBAR         (-26)



/*!
 * @brief 画面左の能力値表示を行うために指定位置から13キャラ分を空白消去後指定のメッセージを明るい青で描画する /
 * Print character info at given row, column in a 13 char field
 * @param info 表示文字列
 * @param row 描画列
 * @param col 描画行
 * @return なし
 */
static void prt_field(concptr info, TERM_LEN row, TERM_LEN col)
{
	/* Dump 13 spaces to clear */
	c_put_str(TERM_WHITE, "             ", row, col);

	/* Dump the info itself */
	c_put_str(TERM_L_BLUE, info, row, col);
}

/*!
 * @brief ゲーム時刻を表示する /
 * Print time
 * @return なし
 */
void prt_time(void)
{
	int day, hour, min;

	/* Dump 13 spaces to clear */
	c_put_str(TERM_WHITE, "             ", ROW_DAY, COL_DAY);

	extract_day_hour_min(&day, &hour, &min);

	/* Dump the info itself */
	if (day < 1000) c_put_str(TERM_WHITE, format(_("%2d日目", "Day%3d"), day), ROW_DAY, COL_DAY);
	else c_put_str(TERM_WHITE, _("***日目", "Day***"), ROW_DAY, COL_DAY);

	c_put_str(TERM_WHITE, format("%2d:%02d", hour, min), ROW_DAY, COL_DAY+7);
}

/*!
 * @brief 現在のマップ名を返す /
 * @return マップ名の文字列参照ポインタ
 */
concptr map_name(void)
{
	if (p_ptr->current_floor_ptr->inside_quest && is_fixed_quest_idx(p_ptr->current_floor_ptr->inside_quest)
	    && (quest[p_ptr->current_floor_ptr->inside_quest].flags & QUEST_FLAG_PRESET))
		return _("クエスト", "Quest");
	else if (p_ptr->wild_mode)
		return _("地上", "Surface");
	else if (p_ptr->current_floor_ptr->inside_arena)
		return _("アリーナ", "Arena");
	else if (p_ptr->phase_out)
		return _("闘技場", "Monster Arena");
	else if (!p_ptr->current_floor_ptr->dun_level && p_ptr->town_num)
		return town_info[p_ptr->town_num].name;
	else
		return d_name+d_info[p_ptr->dungeon_idx].name;
}

/*!
 * @brief 現在のマップ名を描画する / Print dungeon
 * @return なし
 */
static void prt_dungeon(void)
{
	concptr dungeon_name;
	TERM_LEN col;

	/* Dump 13 spaces to clear */
	c_put_str(TERM_WHITE, "             ", ROW_DUNGEON, COL_DUNGEON);

	dungeon_name = map_name();

	col = COL_DUNGEON + 6 - strlen(dungeon_name)/2;
	if (col < 0) col = 0;

	/* Dump the info itself */
	c_put_str(TERM_L_UMBER, format("%s",dungeon_name),
		  ROW_DUNGEON, col);
}


/*!
 * @brief プレイヤー能力値を描画する / Print character stat in given row, column
 * @param stat 描画するステータスのID
 * @return なし
 */
static void prt_stat(int stat)
{
	GAME_TEXT tmp[32];

	/* Display "injured" stat */
	if (p_ptr->stat_cur[stat] < p_ptr->stat_max[stat])
	{
		put_str(stat_names_reduced[stat], ROW_STAT + stat, 0);
		cnv_stat(p_ptr->stat_use[stat], tmp);
		c_put_str(TERM_YELLOW, tmp, ROW_STAT + stat, COL_STAT + 6);
	}

	/* Display "healthy" stat */
	else
	{
		put_str(stat_names[stat], ROW_STAT + stat, 0);
		cnv_stat(p_ptr->stat_use[stat], tmp);
		c_put_str(TERM_L_GREEN, tmp, ROW_STAT + stat, COL_STAT + 6);
	}

	/* Indicate natural maximum */
	if (p_ptr->stat_max[stat] == p_ptr->stat_max_max[stat])
	{
#ifdef JP
		/* 日本語にかぶらないように表示位置を変更 */
		put_str("!", ROW_STAT + stat, 5);
#else
		put_str("!", ROW_STAT + stat, 3);
#endif

	}
}


/*
 * 画面下部に表示する状態表示定義ID / Data structure for status bar
 */
#define BAR_TSUYOSHI 0      /*!< 下部ステータス表示: オクレ兄さん状態 */
#define BAR_HALLUCINATION 1 /*!< 下部ステータス表示: 幻覚 */
#define BAR_BLINDNESS 2     /*!< 下部ステータス表示: 盲目 */
#define BAR_PARALYZE 3      /*!< 下部ステータス表示: 麻痺 */
#define BAR_CONFUSE 4       /*!< 下部ステータス表示: 混乱 */
#define BAR_POISONED 5      /*!< 下部ステータス表示: 毒 */
#define BAR_AFRAID 6        /*!< 下部ステータス表示: 恐怖 */
#define BAR_LEVITATE 7      /*!< 下部ステータス表示: 浮遊 */
#define BAR_REFLECTION 8    /*!< 下部ステータス表示: 反射 */
#define BAR_PASSWALL 9      /*!< 下部ステータス表示: 壁抜け */
#define BAR_WRAITH 10       /*!< 下部ステータス表示: 幽体化 */
#define BAR_PROTEVIL 11     /*!< 下部ステータス表示: 対邪悪結界 */
#define BAR_KAWARIMI 12     /*!< 下部ステータス表示: 変わり身 */
#define BAR_MAGICDEFENSE 13 /*!< 下部ステータス表示: 魔法の鎧 */
#define BAR_EXPAND 14       /*!< 下部ステータス表示: 横伸び */
#define BAR_STONESKIN 15    /*!< 下部ステータス表示: 石肌化 */
#define BAR_MULTISHADOW 16  /*!< 下部ステータス表示: 影分身 */
#define BAR_REGMAGIC 17     /*!< 下部ステータス表示: 魔法防御 */
#define BAR_ULTIMATE 18     /*!< 下部ステータス表示: 究極の耐性 */
#define BAR_INVULN 19       /*!< 下部ステータス表示: 無敵化 */
#define BAR_IMMACID 20      /*!< 下部ステータス表示: 酸免疫 */
#define BAR_RESACID 21      /*!< 下部ステータス表示: 酸耐性 */
#define BAR_IMMELEC 22      /*!< 下部ステータス表示: 電撃免疫 */
#define BAR_RESELEC 23      /*!< 下部ステータス表示: 電撃耐性 */
#define BAR_IMMFIRE 24      /*!< 下部ステータス表示: 火炎免疫 */
#define BAR_RESFIRE 25      /*!< 下部ステータス表示: 火炎耐性 */
#define BAR_IMMCOLD 26      /*!< 下部ステータス表示: 冷気免疫 */
#define BAR_RESCOLD 27      /*!< 下部ステータス表示: 冷気耐性 */
#define BAR_RESPOIS 28      /*!< 下部ステータス表示: 毒耐性 */
#define BAR_RESNETH 29      /*!< 下部ステータス表示: 地獄耐性 */
#define BAR_RESTIME 30      /*!< 下部ステータス表示: 時間逆転耐性 */
#define BAR_DUSTROBE 31     /*!< 下部ステータス表示: 破片オーラ */
#define BAR_SHFIRE 32       /*!< 下部ステータス表示: 火炎オーラ */
#define BAR_TOUKI 33        /*!< 下部ステータス表示: 闘気 */
#define BAR_SHHOLY 34       /*!< 下部ステータス表示: 聖なるオーラ */
#define BAR_EYEEYE 35       /*!< 下部ステータス表示: 目には目を */
#define BAR_BLESSED 36      /*!< 下部ステータス表示: 祝福 */
#define BAR_HEROISM 37      /*!< 下部ステータス表示: 士気高揚 */
#define BAR_BERSERK 38      /*!< 下部ステータス表示: 狂戦士化 */
#define BAR_ATTKFIRE 39     /*!< 下部ステータス表示: 焼棄スレイ */
#define BAR_ATTKCOLD 40     /*!< 下部ステータス表示: 冷凍スレイ */
#define BAR_ATTKELEC 41     /*!< 下部ステータス表示: 電撃スレイ */
#define BAR_ATTKACID 42     /*!< 下部ステータス表示: 溶解スレイ */
#define BAR_ATTKPOIS 43     /*!< 下部ステータス表示: 毒殺スレイ */
#define BAR_ATTKCONF 44     /*!< 下部ステータス表示: 混乱打撃 */
#define BAR_SENSEUNSEEN 45  /*!< 下部ステータス表示: 透明視 */
#define BAR_TELEPATHY 46    /*!< 下部ステータス表示: テレパシー */
#define BAR_REGENERATION 47 /*!< 下部ステータス表示: 急回復 */
#define BAR_INFRAVISION 48  /*!< 下部ステータス表示: 赤外線視力 */
#define BAR_STEALTH 49      /*!< 下部ステータス表示: 隠密 */
#define BAR_SUPERSTEALTH 50 /*!< 下部ステータス表示: 超隠密 */
#define BAR_RECALL 51       /*!< 下部ステータス表示: 帰還待ち */
#define BAR_ALTER 52        /*!< 下部ステータス表示: 現実変容待ち */
#define BAR_SHCOLD 53       /*!< 下部ステータス表示: 冷気オーラ */
#define BAR_SHELEC 54       /*!< 下部ステータス表示: 電撃オーラ */
#define BAR_SHSHADOW 55     /*!< 下部ステータス表示: 影のオーラ */
#define BAR_MIGHT 56        /*!< 下部ステータス表示: 腕力強化 */
#define BAR_BUILD 57        /*!< 下部ステータス表示: 肉体強化 */
#define BAR_ANTIMULTI 58    /*!< 下部ステータス表示: 反増殖 */
#define BAR_ANTITELE 59     /*!< 下部ステータス表示: 反テレポート */
#define BAR_ANTIMAGIC 60    /*!< 下部ステータス表示: 反魔法 */
#define BAR_PATIENCE 61     /*!< 下部ステータス表示: 我慢 */
#define BAR_REVENGE 62      /*!< 下部ステータス表示: 宣告 */
#define BAR_RUNESWORD 63    /*!< 下部ステータス表示: 魔剣化 */
#define BAR_VAMPILIC 64     /*!< 下部ステータス表示: 吸血 */
#define BAR_CURE 65         /*!< 下部ステータス表示: 回復 */
#define BAR_ESP_EVIL 66     /*!< 下部ステータス表示: 邪悪感知 */

static struct {
	TERM_COLOR attr;
	concptr sstr;
	concptr lstr;
} bar[]
#ifdef JP
= {
	{TERM_YELLOW, "つ", "つよし"},
	{TERM_VIOLET, "幻", "幻覚"},
	{TERM_L_DARK, "盲", "盲目"},
	{TERM_RED, "痺", "麻痺"},
	{TERM_VIOLET, "乱", "混乱"},
	{TERM_GREEN, "毒", "毒"},
	{TERM_BLUE, "恐", "恐怖"},
	{TERM_L_BLUE, "浮", "浮遊"},
	{TERM_SLATE, "反", "反射"},
	{TERM_SLATE, "壁", "壁抜け"},
	{TERM_L_DARK, "幽", "幽体"},
	{TERM_SLATE, "邪", "防邪"},
	{TERM_VIOLET, "変", "変わり身"},
	{TERM_YELLOW, "魔", "魔法鎧"},
	{TERM_L_UMBER, "伸", "伸び"},
	{TERM_WHITE, "石", "石肌"},
	{TERM_L_BLUE, "分", "分身"},
	{TERM_SLATE, "防", "魔法防御"},
	{TERM_YELLOW, "究", "究極"},
	{TERM_YELLOW, "無", "無敵"},
	{TERM_L_GREEN, "酸", "酸免疫"},
	{TERM_GREEN, "酸", "耐酸"},
	{TERM_L_BLUE, "電", "電免疫"},
	{TERM_BLUE, "電", "耐電"},
	{TERM_L_RED, "火", "火免疫"},
	{TERM_RED, "火", "耐火"},
	{TERM_WHITE, "冷", "冷免疫"},
	{TERM_SLATE, "冷", "耐冷"},
	{TERM_GREEN, "毒", "耐毒"},
	{TERM_L_DARK, "獄", "耐地獄"},
	{TERM_L_BLUE, "時", "耐時間"},
	{TERM_L_DARK, "鏡", "鏡オーラ"},
	{TERM_L_RED, "オ", "火オーラ"},
	{TERM_WHITE, "闘", "闘気"},
	{TERM_WHITE, "聖", "聖オーラ"},
	{TERM_VIOLET, "目", "目には目"},
	{TERM_WHITE, "祝", "祝福"},
	{TERM_WHITE, "勇", "勇"},
	{TERM_RED, "狂", "狂乱"},
	{TERM_L_RED, "火", "魔剣火"},
	{TERM_WHITE, "冷", "魔剣冷"},
	{TERM_L_BLUE, "電", "魔剣電"},
	{TERM_SLATE, "酸", "魔剣酸"},
	{TERM_L_GREEN, "毒", "魔剣毒"},
	{TERM_RED, "乱", "混乱打撃"},
	{TERM_L_BLUE, "視", "透明視"},
	{TERM_ORANGE, "テ", "テレパシ"},
	{TERM_L_BLUE, "回", "回復"},
	{TERM_L_RED, "赤", "赤外"},
	{TERM_UMBER, "隠", "隠密"},
	{TERM_YELLOW, "隠", "超隠密"},
	{TERM_WHITE, "帰", "帰還"},
	{TERM_WHITE, "現", "現実変容"},
	{TERM_WHITE, "オ", "氷オーラ"},
	{TERM_BLUE, "オ", "電オーラ"},
	{TERM_L_DARK, "オ", "影オーラ"},
	{TERM_YELLOW, "腕", "腕力強化"},
	{TERM_RED, "肉", "肉体強化"},
	{TERM_L_DARK, "殖", "反増殖"},
	{TERM_ORANGE, "テ", "反テレポ"},
	{TERM_RED, "魔", "反魔法"},
	{TERM_SLATE, "我", "我慢"},
	{TERM_SLATE, "宣", "宣告"},
	{TERM_L_DARK, "剣", "魔剣化"},
	{TERM_RED, "吸", "吸血打撃"},
	{TERM_WHITE, "回", "回復"},
	{TERM_L_DARK, "感", "邪悪感知"},
	{0, NULL, NULL}
};
#else
= {
	{TERM_YELLOW, "Ts", "Tsuyoshi"},
	{TERM_VIOLET, "Ha", "Halluc"},
	{TERM_L_DARK, "Bl", "Blind"},
	{TERM_RED, "Pa", "Paralyzed"},
	{TERM_VIOLET, "Cf", "Confused"},
	{TERM_GREEN, "Po", "Poisoned"},
	{TERM_BLUE, "Af", "Afraid"},
	{TERM_L_BLUE, "Lv", "Levit"},
	{TERM_SLATE, "Rf", "Reflect"},
	{TERM_SLATE, "Pw", "PassWall"},
	{TERM_L_DARK, "Wr", "Wraith"},
	{TERM_SLATE, "Ev", "PrtEvl"},
	{TERM_VIOLET, "Kw", "Kawarimi"},
	{TERM_YELLOW, "Md", "MgcArm"},
	{TERM_L_UMBER, "Eh", "Expand"},
	{TERM_WHITE, "Ss", "StnSkn"},
	{TERM_L_BLUE, "Ms", "MltShdw"},
	{TERM_SLATE, "Rm", "ResMag"},
	{TERM_YELLOW, "Ul", "Ultima"},
	{TERM_YELLOW, "Iv", "Invuln"},
	{TERM_L_GREEN, "IAc", "ImmAcid"},
	{TERM_GREEN, "Ac", "Acid"},
	{TERM_L_BLUE, "IEl", "ImmElec"},
	{TERM_BLUE, "El", "Elec"},
	{TERM_L_RED, "IFi", "ImmFire"},
	{TERM_RED, "Fi", "Fire"},
	{TERM_WHITE, "ICo", "ImmCold"},
	{TERM_SLATE, "Co", "Cold"},
	{TERM_GREEN, "Po", "Pois"},
	{TERM_L_DARK, "Nt", "Nthr"},
	{TERM_L_BLUE, "Ti", "Time"},
	{TERM_L_DARK, "Mr", "Mirr"},
	{TERM_L_RED, "SFi", "SFire"},
	{TERM_WHITE, "Fo", "Force"},
	{TERM_WHITE, "Ho", "Holy"},
	{TERM_VIOLET, "Ee", "EyeEye"},
	{TERM_WHITE, "Bs", "Bless"},
	{TERM_WHITE, "He", "Hero"},
	{TERM_RED, "Br", "Berserk"},
	{TERM_L_RED, "BFi", "BFire"},
	{TERM_WHITE, "BCo", "BCold"},
	{TERM_L_BLUE, "BEl", "BElec"},
	{TERM_SLATE, "BAc", "BAcid"},
	{TERM_L_GREEN, "BPo", "BPois"},
	{TERM_RED, "TCf", "TchCnf"},
	{TERM_L_BLUE, "Se", "SInv"},
	{TERM_ORANGE, "Te", "Telepa"},
	{TERM_L_BLUE, "Rg", "Regen"},
	{TERM_L_RED, "If", "Infr"},
	{TERM_UMBER, "Sl", "Stealth"},
	{TERM_YELLOW, "Stlt", "Stealth"},
	{TERM_WHITE, "Rc", "Recall"},
	{TERM_WHITE, "Al", "Alter"},
	{TERM_WHITE, "SCo", "SCold"},
	{TERM_BLUE, "SEl", "SElec"},
	{TERM_L_DARK, "SSh", "SShadow"},
	{TERM_YELLOW, "EMi", "ExMight"},
	{TERM_RED, "Bu", "BuildUp"},
	{TERM_L_DARK, "AMl", "AntiMulti"},
	{TERM_ORANGE, "AT", "AntiTele"},
	{TERM_RED, "AM", "AntiMagic"},
	{TERM_SLATE, "Pa", "Patience"},
	{TERM_SLATE, "Rv", "Revenge"},
	{TERM_L_DARK, "Rs", "RuneSword"},
	{TERM_RED, "Vm", "Vampiric"},
	{TERM_WHITE, "Cu", "Cure"},
	{TERM_L_DARK, "ET", "EvilTele"},
	{0, NULL, NULL}
};
#endif

/*!
 * @brief 32ビット変数配列の指定位置のビットフラグを1にする。
 * @param FLG フラグ位置(ビット)
 * @return なし
 */
#define ADD_FLG(FLG) (bar_flags[FLG / 32] |= (1L << (FLG % 32)))

/*!
 * @brief 32ビット変数配列の指定位置のビットフラグが1かどうかを返す。
 * @param FLG フラグ位置(ビット)
 * @return 1ならば0以外を返す
 */
#define IS_FLG(FLG) (bar_flags[FLG / 32] & (1L << (FLG % 32)))


/*!
 * @brief 下部に状態表示を行う / Show status bar
 * @return なし
 */
static void prt_status(void)
{
	BIT_FLAGS bar_flags[3];
	TERM_LEN wid, hgt, row_statbar, max_col_statbar;
	int i;
	TERM_LEN col = 0, num = 0;
	int space = 2;

	Term_get_size(&wid, &hgt);
	row_statbar = hgt + ROW_STATBAR;
	max_col_statbar = wid + MAX_COL_STATBAR;

	Term_erase(0, row_statbar, max_col_statbar);

	bar_flags[0] = bar_flags[1] = bar_flags[2] = 0L;

	/* Tsuyoshi  */
	if (p_ptr->tsuyoshi) ADD_FLG(BAR_TSUYOSHI);

	/* Hallucinating */
	if (p_ptr->image) ADD_FLG(BAR_HALLUCINATION);

	/* Blindness */
	if (p_ptr->blind) ADD_FLG(BAR_BLINDNESS);

	/* Paralysis */
	if (p_ptr->paralyzed) ADD_FLG(BAR_PARALYZE);

	/* Confusion */
	if (p_ptr->confused) ADD_FLG(BAR_CONFUSE);

	/* Posioned */
	if (p_ptr->poisoned) ADD_FLG(BAR_POISONED);

	/* Times see-invisible */
	if (p_ptr->tim_invis) ADD_FLG(BAR_SENSEUNSEEN);

	/* Timed esp */
	if (IS_TIM_ESP()) ADD_FLG(BAR_TELEPATHY);

	/* Timed regenerate */
	if (p_ptr->tim_regen) ADD_FLG(BAR_REGENERATION);

	/* Timed infra-vision */
	if (p_ptr->tim_infra) ADD_FLG(BAR_INFRAVISION);

	/* Protection from evil */
	if (p_ptr->protevil) ADD_FLG(BAR_PROTEVIL);

	/* Invulnerability */
	if (IS_INVULN()) ADD_FLG(BAR_INVULN);

	/* Wraith form */
	if (p_ptr->wraith_form) ADD_FLG(BAR_WRAITH);

	/* Kabenuke */
	if (p_ptr->kabenuke) ADD_FLG(BAR_PASSWALL);

	if (p_ptr->tim_reflect) ADD_FLG(BAR_REFLECTION);

	/* Heroism */
	if (IS_HERO()) ADD_FLG(BAR_HEROISM);

	/* Super Heroism / berserk */
	if (p_ptr->shero) ADD_FLG(BAR_BERSERK);

	/* Blessed */
	if (IS_BLESSED()) ADD_FLG(BAR_BLESSED);

	/* Shield */
	if (p_ptr->magicdef) ADD_FLG(BAR_MAGICDEFENSE);

	if (p_ptr->tsubureru) ADD_FLG(BAR_EXPAND);

	if (p_ptr->shield) ADD_FLG(BAR_STONESKIN);
	
	if (p_ptr->special_defense & NINJA_KAWARIMI) ADD_FLG(BAR_KAWARIMI);

	/* Oppose Acid */
	if (p_ptr->special_defense & DEFENSE_ACID) ADD_FLG(BAR_IMMACID);
	if (IS_OPPOSE_ACID()) ADD_FLG(BAR_RESACID);

	/* Oppose Lightning */
	if (p_ptr->special_defense & DEFENSE_ELEC) ADD_FLG(BAR_IMMELEC);
	if (IS_OPPOSE_ELEC()) ADD_FLG(BAR_RESELEC);

	/* Oppose Fire */
	if (p_ptr->special_defense & DEFENSE_FIRE) ADD_FLG(BAR_IMMFIRE);
	if (IS_OPPOSE_FIRE()) ADD_FLG(BAR_RESFIRE);

	/* Oppose Cold */
	if (p_ptr->special_defense & DEFENSE_COLD) ADD_FLG(BAR_IMMCOLD);
	if (IS_OPPOSE_COLD()) ADD_FLG(BAR_RESCOLD);

	/* Oppose Poison */
	if (IS_OPPOSE_POIS()) ADD_FLG(BAR_RESPOIS);

	/* Word of Recall */
	if (p_ptr->word_recall) ADD_FLG(BAR_RECALL);

	/* Alter realiry */
	if (p_ptr->alter_reality) ADD_FLG(BAR_ALTER);

	/* Afraid */
	if (p_ptr->afraid) ADD_FLG(BAR_AFRAID);

	/* Resist time */
	if (p_ptr->tim_res_time) ADD_FLG(BAR_RESTIME);

	if (p_ptr->multishadow) ADD_FLG(BAR_MULTISHADOW);

	/* Confusing Hands */
	if (p_ptr->special_attack & ATTACK_CONFUSE) ADD_FLG(BAR_ATTKCONF);

	if (p_ptr->resist_magic) ADD_FLG(BAR_REGMAGIC);

	/* Ultimate-resistance */
	if (p_ptr->ult_res) ADD_FLG(BAR_ULTIMATE);

	/* tim levitation */
	if (p_ptr->tim_levitation) ADD_FLG(BAR_LEVITATE);

	if (p_ptr->tim_res_nether) ADD_FLG(BAR_RESNETH);

	if (p_ptr->dustrobe) ADD_FLG(BAR_DUSTROBE);

	/* Mahouken */
	if (p_ptr->special_attack & ATTACK_FIRE) ADD_FLG(BAR_ATTKFIRE);
	if (p_ptr->special_attack & ATTACK_COLD) ADD_FLG(BAR_ATTKCOLD);
	if (p_ptr->special_attack & ATTACK_ELEC) ADD_FLG(BAR_ATTKELEC);
	if (p_ptr->special_attack & ATTACK_ACID) ADD_FLG(BAR_ATTKACID);
	if (p_ptr->special_attack & ATTACK_POIS) ADD_FLG(BAR_ATTKPOIS);
	if (p_ptr->special_defense & NINJA_S_STEALTH) ADD_FLG(BAR_SUPERSTEALTH);

	if (p_ptr->tim_sh_fire) ADD_FLG(BAR_SHFIRE);

	/* tim stealth */
	if (IS_TIM_STEALTH()) ADD_FLG(BAR_STEALTH);

	if (p_ptr->tim_sh_touki) ADD_FLG(BAR_TOUKI);

	/* Holy aura */
	if (p_ptr->tim_sh_holy) ADD_FLG(BAR_SHHOLY);

	/* An Eye for an Eye */
	if (p_ptr->tim_eyeeye) ADD_FLG(BAR_EYEEYE);

	/* Hex spells */
	if (p_ptr->realm1 == REALM_HEX)
	{
		if (hex_spelling(HEX_BLESS)) ADD_FLG(BAR_BLESSED);
		if (hex_spelling(HEX_DEMON_AURA)) { ADD_FLG(BAR_SHFIRE); ADD_FLG(BAR_REGENERATION); }
		if (hex_spelling(HEX_XTRA_MIGHT)) ADD_FLG(BAR_MIGHT);
		if (hex_spelling(HEX_DETECT_EVIL)) ADD_FLG(BAR_ESP_EVIL);
		if (hex_spelling(HEX_ICE_ARMOR)) ADD_FLG(BAR_SHCOLD);
		if (hex_spelling(HEX_RUNESWORD)) ADD_FLG(BAR_RUNESWORD);
		if (hex_spelling(HEX_BUILDING)) ADD_FLG(BAR_BUILD);
		if (hex_spelling(HEX_ANTI_TELE)) ADD_FLG(BAR_ANTITELE);
		if (hex_spelling(HEX_SHOCK_CLOAK)) ADD_FLG(BAR_SHELEC);
		if (hex_spelling(HEX_SHADOW_CLOAK)) ADD_FLG(BAR_SHSHADOW);
		if (hex_spelling(HEX_CONFUSION)) ADD_FLG(BAR_ATTKCONF);
		if (hex_spelling(HEX_EYE_FOR_EYE)) ADD_FLG(BAR_EYEEYE);
		if (hex_spelling(HEX_ANTI_MULTI)) ADD_FLG(BAR_ANTIMULTI);
		if (hex_spelling(HEX_VAMP_BLADE)) ADD_FLG(BAR_VAMPILIC);
		if (hex_spelling(HEX_ANTI_MAGIC)) ADD_FLG(BAR_ANTIMAGIC);
		if (hex_spelling(HEX_CURE_LIGHT) ||
			hex_spelling(HEX_CURE_SERIOUS) ||
			hex_spelling(HEX_CURE_CRITICAL)) ADD_FLG(BAR_CURE);

		if (HEX_REVENGE_TURN(p_ptr))
		{
			if (HEX_REVENGE_TYPE(p_ptr) == 1) ADD_FLG(BAR_PATIENCE);
			if (HEX_REVENGE_TYPE(p_ptr) == 2) ADD_FLG(BAR_REVENGE);
		}
	}

	/* Calcurate length */
	for (i = 0; bar[i].sstr; i++)
	{
		if (IS_FLG(i))
		{
			col += strlen(bar[i].lstr) + 1;
			num++;
		}
	}

	/* If there are not excess spaces for long strings, use short one */
	if (col - 1 > max_col_statbar)
	{
		space = 0;
		col = 0;

		for (i = 0; bar[i].sstr; i++)
		{
			if (IS_FLG(i))
			{
				col += strlen(bar[i].sstr);
			}
		}

		/* If there are excess spaces for short string, use more */
		if (col - 1 <= max_col_statbar - (num-1))
		{
			space = 1;
			col += num - 1;
		}
	}


	/* Centering display column */
	col = (max_col_statbar - col) / 2;

	/* Display status bar */
	for (i = 0; bar[i].sstr; i++)
	{
		if (IS_FLG(i))
		{
			concptr str;
			if (space == 2) str = bar[i].lstr;
			else str = bar[i].sstr;

			c_put_str(bar[i].attr, str, row_statbar, col);
			col += strlen(str);
			if (space > 0) col++;
			if (col > max_col_statbar) break;
		}
	}
}


/*!
 * @brief プレイヤーの称号を表示する / Prints "title", including "wizard" or "winner" as needed.
 * @return なし
 */
static void prt_title(void)
{
	concptr p = "";
	GAME_TEXT str[14];

	if (current_world_ptr->wizard)
	{
		p = _("[ウィザード]", "[=-WIZARD-=]");
	}
	else if (p_ptr->total_winner || (p_ptr->lev > PY_MAX_LEVEL))
	{
		if (p_ptr->arena_number > MAX_ARENA_MONS + 2)
		{
			p = _("*真・勝利者*", "*TRUEWINNER*");
		}
		else
		{
			p = _("***勝利者***", "***WINNER***");
		}
	}

	/* Normal */
	else
	{
		my_strcpy(str, player_title[p_ptr->pclass][(p_ptr->lev - 1) / 5], sizeof(str));
		p = str;
	}

	prt_field(p, ROW_TITLE, COL_TITLE);
}


/*!
 * @brief プレイヤーのレベルを表示する / Prints level
 * @return なし
 */
static void prt_level(void)
{
	char tmp[32];

	sprintf(tmp, _("%5d", "%6d"), p_ptr->lev);

	if (p_ptr->lev >= p_ptr->max_plv)
	{
		put_str(_("レベル ", "LEVEL "), ROW_LEVEL, 0);
		c_put_str(TERM_L_GREEN, tmp, ROW_LEVEL, COL_LEVEL + 7);
	}
	else
	{
		put_str(_("xレベル", "Level "), ROW_LEVEL, 0);
		c_put_str(TERM_YELLOW, tmp, ROW_LEVEL, COL_LEVEL + 7);
	}
}


/*!
 * @brief プレイヤーの経験値を表示する / Display the experience
 * @return なし
 */
static void prt_exp(void)
{
	char out_val[32];

	if ((!exp_need)||(p_ptr->prace == RACE_ANDROID))
	{
		(void)sprintf(out_val, "%8ld", (long)p_ptr->exp);
	}
	else
	{
		if (p_ptr->lev >= PY_MAX_LEVEL)
		{
			(void)sprintf(out_val, "********");
		}
		else
		{
			(void)sprintf(out_val, "%8ld", (long)(player_exp [p_ptr->lev - 1] * p_ptr->expfact / 100L) - p_ptr->exp);
		}
	}

	if (p_ptr->exp >= p_ptr->max_exp)
	{
		if (p_ptr->prace == RACE_ANDROID) put_str(_("強化 ", "Cst "), ROW_EXP, 0);
		else put_str(_("経験 ", "EXP "), ROW_EXP, 0);
		c_put_str(TERM_L_GREEN, out_val, ROW_EXP, COL_EXP + 4);
	}
	else
	{
		put_str(_("x経験", "Exp "), ROW_EXP, 0);
		c_put_str(TERM_YELLOW, out_val, ROW_EXP, COL_EXP + 4);
	}
}

/*!
 * @brief プレイヤーの所持金を表示する / Prints current gold
 * @return なし
 */
static void prt_gold(void)
{
	char tmp[32];
	put_str(_("＄ ", "AU "), ROW_GOLD, COL_GOLD);
	sprintf(tmp, "%9ld", (long)p_ptr->au);
	c_put_str(TERM_L_GREEN, tmp, ROW_GOLD, COL_GOLD + 3);
}


/*!
 * @brief プレイヤーのACを表示する / Prints current AC
 * @return なし
 */
static void prt_ac(void)
{
	char tmp[32];

#ifdef JP
/* AC の表示方式を変更している */
	put_str(" ＡＣ(     )", ROW_AC, COL_AC);
	sprintf(tmp, "%5d", p_ptr->dis_ac + p_ptr->dis_to_a);
	c_put_str(TERM_L_GREEN, tmp, ROW_AC, COL_AC + 6);
#else
	put_str("Cur AC ", ROW_AC, COL_AC);
	sprintf(tmp, "%5d", p_ptr->dis_ac + p_ptr->dis_to_a);
	c_put_str(TERM_L_GREEN, tmp, ROW_AC, COL_AC + 7);
#endif

}


/*!
 * @brief プレイヤーのHPを表示する / Prints Cur/Max hit points
 * @return なし
 */
static void prt_hp(void)
{
	/* ヒットポイントの表示方法を変更 */
	char tmp[32];
  
	TERM_COLOR color;
  
	/* タイトル */
	put_str("HP", ROW_CURHP, COL_CURHP);

	/* 現在のヒットポイント */
	sprintf(tmp, "%4ld", (long int)p_ptr->chp);

	if (p_ptr->chp >= p_ptr->mhp)
	{
		color = TERM_L_GREEN;
	}
	else if (p_ptr->chp > (p_ptr->mhp * hitpoint_warn) / 10)
	{
		color = TERM_YELLOW;
	}
	else
	{
		color = TERM_RED;
	}

	c_put_str(color, tmp, ROW_CURHP, COL_CURHP+3);

	/* 区切り */
	put_str( "/", ROW_CURHP, COL_CURHP + 7 );

	/* 最大ヒットポイント */
	sprintf(tmp, "%4ld", (long int)p_ptr->mhp);
	color = TERM_L_GREEN;

	c_put_str(color, tmp, ROW_CURHP, COL_CURHP + 8 );
}


/*!
 * @brief プレイヤーのMPを表示する / Prints players max/cur spell points
 * @return なし
 */
static void prt_sp(void)
{
/* マジックポイントの表示方法を変更している */
	char tmp[32];
	byte color;


	/* Do not show mana unless it matters */
	if (!mp_ptr->spell_book) return;

	/* タイトル */
	put_str(_("MP", "SP"), ROW_CURSP, COL_CURSP);

	/* 現在のマジックポイント */
	sprintf(tmp, "%4ld", (long int)p_ptr->csp);

	if (p_ptr->csp >= p_ptr->msp)
	{
		color = TERM_L_GREEN;
	}
	else if (p_ptr->csp > (p_ptr->msp * mana_warn) / 10)
	{
		color = TERM_YELLOW;
	}
	else
	{
		color = TERM_RED;
	}

	c_put_str(color, tmp, ROW_CURSP, COL_CURSP+3);

	/* 区切り */
	put_str( "/", ROW_CURSP, COL_CURSP + 7 );

	/* 最大マジックポイント */
	sprintf(tmp, "%4ld", (long int)p_ptr->msp);
	color = TERM_L_GREEN;

	c_put_str(color, tmp, ROW_CURSP, COL_CURSP + 8);
}


/*!
 * @brief 現在のフロアの深さを表示する / Prints depth in stat area
 * @return なし
 */
static void prt_depth(void)
{
	char depths[32];
	TERM_LEN wid, hgt, row_depth, col_depth;
	TERM_COLOR attr = TERM_WHITE;

	Term_get_size(&wid, &hgt);
	col_depth = wid + COL_DEPTH;
	row_depth = hgt + ROW_DEPTH;

	if (!p_ptr->current_floor_ptr->dun_level)
	{
		strcpy(depths, _("地上", "Surf."));
	}
	else if (p_ptr->current_floor_ptr->inside_quest && !p_ptr->dungeon_idx)
	{
		strcpy(depths, _("地上", "Quest"));
	}
	else
	{
		if (depth_in_feet) (void)sprintf(depths, _("%d ft", "%d ft"), (int)p_ptr->current_floor_ptr->dun_level * 50);
		else (void)sprintf(depths, _("%d 階", "Lev %d"), (int)p_ptr->current_floor_ptr->dun_level);

		/* Get color of level based on feeling  -JSV- */
		switch (p_ptr->feeling)
		{
		case  0: attr = TERM_SLATE;   break; /* Unknown */
		case  1: attr = TERM_L_BLUE;  break; /* Special */
		case  2: attr = TERM_VIOLET;  break; /* Horrible visions */
		case  3: attr = TERM_RED;     break; /* Very dangerous */
		case  4: attr = TERM_L_RED;   break; /* Very bad feeling */
		case  5: attr = TERM_ORANGE;  break; /* Bad feeling */
		case  6: attr = TERM_YELLOW;  break; /* Nervous */
		case  7: attr = TERM_L_UMBER; break; /* Luck is turning */
		case  8: attr = TERM_L_WHITE; break; /* Don't like */
		case  9: attr = TERM_WHITE;   break; /* Reasonably safe */
		case 10: attr = TERM_WHITE;   break; /* Boring place */
		}
	}

	/* Right-Adjust the "depth", and clear old values */
	c_prt(attr, format("%7s", depths), row_depth, col_depth);
}


/*!
 * @brief プレイヤーの空腹状態を表示する / Prints status of hunger
 * @return なし
 */
static void prt_hunger(void)
{
	if(current_world_ptr->wizard && p_ptr->current_floor_ptr->inside_arena) return;

	/* Fainting / Starving */
	if (p_ptr->food < PY_FOOD_FAINT)
	{
		c_put_str(TERM_RED, _("衰弱  ", "Weak  "), ROW_HUNGRY, COL_HUNGRY);
	}

	/* Weak */
	else if (p_ptr->food < PY_FOOD_WEAK)
	{
		c_put_str(TERM_ORANGE, _("衰弱  ", "Weak  "), ROW_HUNGRY, COL_HUNGRY);
	}

	/* Hungry */
	else if (p_ptr->food < PY_FOOD_ALERT)
	{
		c_put_str(TERM_YELLOW, _("空腹  ", "Hungry"), ROW_HUNGRY, COL_HUNGRY);
	}

	/* Normal */
	else if (p_ptr->food < PY_FOOD_FULL)
	{
		c_put_str(TERM_L_GREEN, "      ", ROW_HUNGRY, COL_HUNGRY);
	}

	/* Full */
	else if (p_ptr->food < PY_FOOD_MAX)
	{
		c_put_str(TERM_L_GREEN, _("満腹  ", "Full  "), ROW_HUNGRY, COL_HUNGRY);
	}

	/* Gorged */
	else
	{
		c_put_str(TERM_GREEN, _("食過ぎ", "Gorged"), ROW_HUNGRY, COL_HUNGRY);
	}
}


/*!
 * @brief プレイヤーの行動状態を表示する / Prints Searching, Resting, Paralysis, or 'count' status
 * @return なし
 * @details
 * Display is always exactly 10 characters wide (see below)
 * This function was a major bottleneck when resting, so a lot of
 * the text formatting code was optimized in place below.
 */
static void prt_state(void)
{
	TERM_COLOR attr = TERM_WHITE;
	GAME_TEXT text[16];

	/* Repeating */
	if (command_rep)
	{
		if (command_rep > 999)
		{
			(void)sprintf(text, "%2d00", command_rep / 100);
		}
		else
		{
			(void)sprintf(text, "  %2d", command_rep);
		}
	}

	/* Action */
	else
	{
		switch(p_ptr->action)
		{
			case ACTION_SEARCH:
			{
				strcpy(text, _("探索", "Sear"));
				break;
			}
			case ACTION_REST:
				/* Start with "Rest" */
				strcpy(text, _("    ", "    "));

				if (p_ptr->resting > 0)
				{
					sprintf(text, "%4d", p_ptr->resting);
				}
				else if (p_ptr->resting == COMMAND_ARG_REST_FULL_HEALING)
				{
					text[0] = text[1] = text[2] = text[3] = '*';
				}
				else if (p_ptr->resting == COMMAND_ARG_REST_UNTIL_DONE)
				{
					text[0] = text[1] = text[2] = text[3] = '&';
				}
				break;

			case ACTION_LEARN:
			{
				strcpy(text, _("学習", "lear"));
				if (p_ptr->new_mane) attr = TERM_L_RED;
				break;
			}
			case ACTION_FISH:
			{
				strcpy(text, _("釣り", "fish"));
				break;
			}
			case ACTION_KAMAE:
			{
				int i;
				for (i = 0; i < MAX_KAMAE; i++)
					if (p_ptr->special_defense & (KAMAE_GENBU << i)) break;
				switch (i)
				{
					case 0: attr = TERM_GREEN;break;
					case 1: attr = TERM_WHITE;break;
					case 2: attr = TERM_L_BLUE;break;
					case 3: attr = TERM_L_RED;break;
				}
				strcpy(text, kamae_shurui[i].desc);
				break;
			}
			case ACTION_KATA:
			{
				int i;
				for (i = 0; i < MAX_KATA; i++)
					if (p_ptr->special_defense & (KATA_IAI << i)) break;
				strcpy(text, kata_shurui[i].desc);
				break;
			}
			case ACTION_SING:
			{
				strcpy(text, _("歌  ", "Sing"));
				break;
			}
			case ACTION_HAYAGAKE:
			{
				strcpy(text, _("速駆", "Fast"));
				break;
			}
			case ACTION_SPELL:
			{
				strcpy(text, _("詠唱", "Spel"));
				break;
			}
			default:
			{
				strcpy(text, "    ");
				break;
			}
		}
	}

	/* Display the info (or blanks) */
	c_put_str(attr, format("%5.5s",text), ROW_STATE, COL_STATE);
}


/*!
 * @brief プレイヤーの行動速度を表示する / Prints the speed of a character.			-CJS-
 * @return なし
 */
static void prt_speed(void)
{
	int i = p_ptr->pspeed;
	bool is_fast = IS_FAST();

	TERM_COLOR attr = TERM_WHITE;
	char buf[32] = "";
	TERM_LEN wid, hgt, row_speed, col_speed;

	Term_get_size(&wid, &hgt);
	col_speed = wid + COL_SPEED;
	row_speed = hgt + ROW_SPEED;

	/* Hack -- Visually "undo" the Search Mode Slowdown */
	if (p_ptr->action == ACTION_SEARCH && !p_ptr->lightspeed) i += 10;

	/* Fast */
	if (i > 110)
	{
		if (p_ptr->riding)
		{
			monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[p_ptr->riding];
			if (MON_FAST(m_ptr) && !MON_SLOW(m_ptr)) attr = TERM_L_BLUE;
			else if (MON_SLOW(m_ptr) && !MON_FAST(m_ptr)) attr = TERM_VIOLET;
			else attr = TERM_GREEN;
		}
		else if ((is_fast && !p_ptr->slow) || p_ptr->lightspeed) attr = TERM_YELLOW;
		else if (p_ptr->slow && !is_fast) attr = TERM_VIOLET;
		else attr = TERM_L_GREEN;
		sprintf(buf, "%s(+%d)", (p_ptr->riding ? _("乗馬", "Ride") : _("加速", "Fast")), (i - 110));
	}

	/* Slow */
	else if (i < 110)
	{
		if (p_ptr->riding)
		{
			monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[p_ptr->riding];
			if (MON_FAST(m_ptr) && !MON_SLOW(m_ptr)) attr = TERM_L_BLUE;
			else if (MON_SLOW(m_ptr) && !MON_FAST(m_ptr)) attr = TERM_VIOLET;
			else attr = TERM_RED;
		}
		else if (is_fast && !p_ptr->slow) attr = TERM_YELLOW;
		else if (p_ptr->slow && !is_fast) attr = TERM_VIOLET;
		else attr = TERM_L_UMBER;
		sprintf(buf, "%s(-%d)", (p_ptr->riding ? _("乗馬", "Ride") : _("減速", "Slow")), (110 - i));
	}
	else if (p_ptr->riding)
	{
		attr = TERM_GREEN;
		strcpy(buf, _("乗馬中", "Riding"));
	}

	/* Display the speed */
	c_put_str(attr, format("%-9s", buf), row_speed, col_speed);
}


/*!
 * @brief プレイヤーの呪文学習可能状態を表示する
 * @return なし
 */
static void prt_study(void)
{
	TERM_LEN wid, hgt, row_study, col_study;

	Term_get_size(&wid, &hgt);
	col_study = wid + COL_STUDY;
	row_study = hgt + ROW_STUDY;

	if (p_ptr->new_spells)
	{
		put_str(_("学習", "Stud"), row_study, col_study);
	}
	else
	{
		put_str("    ", row_study, col_study);
	}
}


/*!
 * @brief プレイヤーのものまね可能状態を表示する
 * @return なし
 */
static void prt_imitation(void)
{
	TERM_LEN wid, hgt, row_study, col_study;

	Term_get_size(&wid, &hgt);
	col_study = wid + COL_STUDY;
	row_study = hgt + ROW_STUDY;

	if (p_ptr->pclass == CLASS_IMITATOR)
	{
		if (p_ptr->mane_num)
		{
			TERM_COLOR attr;
			if (p_ptr->new_mane) attr = TERM_L_RED;
			else attr = TERM_WHITE;
			c_put_str(attr, _("まね", "Imit"), row_study, col_study);
		}
		else
		{
			put_str("    ", row_study, col_study);
		}
	}
}

/*!
 * @brief プレイヤーの負傷状態を表示する
 * @return なし
 */
static void prt_cut(void)
{
	int c = p_ptr->cut;

	if (c > 1000)
	{
		c_put_str(TERM_L_RED, _("致命傷      ", "Mortal wound"), ROW_CUT, COL_CUT);
	}
	else if (c > 200)
	{
		c_put_str(TERM_RED, _("ひどい深手  ", "Deep gash   "), ROW_CUT, COL_CUT);
	}
	else if (c > 100)
	{
		c_put_str(TERM_RED, _("重傷        ", "Severe cut  "), ROW_CUT, COL_CUT);
	}
	else if (c > 50)
	{
		c_put_str(TERM_ORANGE, _("大変な傷    ", "Nasty cut   "), ROW_CUT, COL_CUT);
	}
	else if (c > 25)
	{
		c_put_str(TERM_ORANGE, _("ひどい傷    ", "Bad cut     "), ROW_CUT, COL_CUT);
	}
	else if (c > 10)
	{
		c_put_str(TERM_YELLOW, _("軽傷        ", "Light cut   "), ROW_CUT, COL_CUT);
	}
	else if (c)
	{
		c_put_str(TERM_YELLOW, _("かすり傷    ", "Graze       "), ROW_CUT, COL_CUT);
	}
	else
	{
		put_str("            ", ROW_CUT, COL_CUT);
	}
}


/*!
 * @brief プレイヤーの朦朧状態を表示する
 * @return なし
 */
static void prt_stun(void)
{
	int s = p_ptr->stun;

	if (s > 100)
	{
		c_put_str(TERM_RED, _("意識不明瞭  ", "Knocked out "), ROW_STUN, COL_STUN);
	}
	else if (s > 50)
	{
		c_put_str(TERM_ORANGE, _("ひどく朦朧  ", "Heavy stun  "), ROW_STUN, COL_STUN);
	}
	else if (s)
	{
		c_put_str(TERM_ORANGE, _("朦朧        ", "Stun        "), ROW_STUN, COL_STUN);
	}
	else
	{
		put_str("            ", ROW_STUN, COL_STUN);
	}
}



/*!
 * @brief モンスターの体力ゲージを表示する
 * @param riding TRUEならば騎乗中のモンスターの体力、FALSEならターゲットモンスターの体力を表示する。表示位置は固定。
 * @return なし
 * @details
 * <pre>
 * Redraw the "monster health bar"	-DRS-
 * Rather extensive modifications by	-BEN-
 *
 * The "monster health bar" provides visual feedback on the "health"
 * of the monster currently being "tracked".  There are several ways
 * to "track" a monster, including targetting it, attacking it, and
 * affecting it (and nobody else) with a ranged attack.
 *
 * Display the monster health bar (affectionately known as the
 * "health-o-meter").  Clear health bar if nothing is being tracked.
 * Auto-track current target monster when bored.  Note that the
 * health-bar stops tracking any monster that "disappears".
 * </pre>
 */
static void health_redraw(bool riding)
{
	s16b health_who;
	int row, col;
	monster_type *m_ptr;

	if (riding)
	{
		health_who = p_ptr->riding;
		row = ROW_RIDING_INFO;
		col = COL_RIDING_INFO;
	}
	else
	{
		health_who = p_ptr->health_who;
		row = ROW_INFO;
		col = COL_INFO;
	}

	m_ptr = &p_ptr->current_floor_ptr->m_list[health_who];

	if (current_world_ptr->wizard && p_ptr->phase_out)
	{
		row = ROW_INFO - 2;
		col = COL_INFO + 2;

		Term_putstr(col - 2, row, 12, TERM_WHITE, "      /     ");
		Term_putstr(col - 2, row + 1, 12, TERM_WHITE, "      /     ");
		Term_putstr(col - 2, row + 2, 12, TERM_WHITE, "      /     ");
		Term_putstr(col - 2, row + 3, 12, TERM_WHITE, "      /     ");

		if(p_ptr->current_floor_ptr->m_list[1].r_idx)
		{
			Term_putstr(col - 2, row, 2, r_info[p_ptr->current_floor_ptr->m_list[1].r_idx].x_attr, format("%c", r_info[p_ptr->current_floor_ptr->m_list[1].r_idx].x_char));
			Term_putstr(col - 1, row, 5, TERM_WHITE, format("%5d", p_ptr->current_floor_ptr->m_list[1].hp));
			Term_putstr(col + 5, row, 6, TERM_WHITE, format("%5d", p_ptr->current_floor_ptr->m_list[1].max_maxhp));
		}

		if(p_ptr->current_floor_ptr->m_list[2].r_idx)
		{
			Term_putstr(col - 2, row + 1, 2, r_info[p_ptr->current_floor_ptr->m_list[2].r_idx].x_attr, format("%c", r_info[p_ptr->current_floor_ptr->m_list[2].r_idx].x_char));
			Term_putstr(col - 1, row + 1, 5, TERM_WHITE, format("%5d", p_ptr->current_floor_ptr->m_list[2].hp));
			Term_putstr(col + 5, row + 1, 6, TERM_WHITE, format("%5d", p_ptr->current_floor_ptr->m_list[2].max_maxhp));
		}

		if(p_ptr->current_floor_ptr->m_list[3].r_idx)
		{
			Term_putstr(col - 2, row + 2, 2, r_info[p_ptr->current_floor_ptr->m_list[3].r_idx].x_attr, format("%c", r_info[p_ptr->current_floor_ptr->m_list[3].r_idx].x_char));
			Term_putstr(col - 1, row + 2, 5, TERM_WHITE, format("%5d", p_ptr->current_floor_ptr->m_list[3].hp));
			Term_putstr(col + 5, row + 2, 6, TERM_WHITE, format("%5d", p_ptr->current_floor_ptr->m_list[3].max_maxhp));
		}

		if(p_ptr->current_floor_ptr->m_list[4].r_idx)
		{
			Term_putstr(col - 2, row + 3, 2, r_info[p_ptr->current_floor_ptr->m_list[4].r_idx].x_attr, format("%c", r_info[p_ptr->current_floor_ptr->m_list[4].r_idx].x_char));
			Term_putstr(col - 1, row + 3, 5, TERM_WHITE, format("%5d", p_ptr->current_floor_ptr->m_list[4].hp));
			Term_putstr(col + 5, row + 3, 6, TERM_WHITE, format("%5d", p_ptr->current_floor_ptr->m_list[4].max_maxhp));
		}
	}
	else
	{

		/* Not tracking */
		if (!health_who)
		{
			/* Erase the health bar */
			Term_erase(col, row, 12);
		}

		/* Tracking an unseen monster */
		else if (!m_ptr->ml)
		{
			/* Indicate that the monster health is "unknown" */
			Term_putstr(col, row, 12, TERM_WHITE, "[----------]");
		}

		/* Tracking a hallucinatory monster */
		else if (p_ptr->image)
		{
			/* Indicate that the monster health is "unknown" */
			Term_putstr(col, row, 12, TERM_WHITE, "[----------]");
		}

		/* Tracking a dead monster (???) */
		else if (m_ptr->hp < 0)
		{
			/* Indicate that the monster health is "unknown" */
			Term_putstr(col, row, 12, TERM_WHITE, "[----------]");
		}

		/* Tracking a visible monster */
		else
		{
			/* Extract the "percent" of health */
			int pct = m_ptr->maxhp > 0 ? 100L * m_ptr->hp / m_ptr->maxhp : 0;
			int pct2 = m_ptr->maxhp > 0 ? 100L * m_ptr->hp / m_ptr->max_maxhp: 0;

			/* Convert percent into "health" */
			int len = (pct2 < 10) ? 1 : (pct2 < 90) ? (pct2 / 10 + 1) : 10;

			/* Default to almost dead */
			TERM_COLOR attr = TERM_RED;

			/* Invulnerable */
			if (MON_INVULNER(m_ptr)) attr = TERM_WHITE;

			/* Asleep */
			else if (MON_CSLEEP(m_ptr)) attr = TERM_BLUE;

			/* Afraid */
			else if (MON_MONFEAR(m_ptr)) attr = TERM_VIOLET;

			/* Healthy */
			else if (pct >= 100) attr = TERM_L_GREEN;

			/* Somewhat Wounded */
			else if (pct >= 60) attr = TERM_YELLOW;

			/* Wounded */
			else if (pct >= 25) attr = TERM_ORANGE;

			/* Badly wounded */
			else if (pct >= 10) attr = TERM_L_RED;

			/* Default to "unknown" */
			Term_putstr(col, row, 12, TERM_WHITE, "[----------]");

			/* Dump the current "health" (use '*' symbols) */
			Term_putstr(col + 1, row, len, attr, "**********");
		}
	}
}



/*!
 * @brief プレイヤーのステータスを一括表示する（左側部分） / Display basic info (mostly left of map)
 * @return なし
 */
static void prt_frame_basic(void)
{
	int i;
	if (p_ptr->mimic_form)
		prt_field(mimic_info[p_ptr->mimic_form].title, ROW_RACE, COL_RACE);
	else
	{
		char str[14];
		my_strcpy(str, rp_ptr->title, sizeof(str));
		prt_field(str, ROW_RACE, COL_RACE);
	}

	prt_title();
	prt_level();
	prt_exp();
	for (i = 0; i < A_MAX; i++) prt_stat(i);
	prt_ac();
	prt_hp();
	prt_sp();
	prt_gold();
	prt_depth();
	health_redraw(FALSE);
	health_redraw(TRUE);
}


/*!
 * @brief プレイヤーのステータスを一括表示する（下部分） / Display extra info (mostly below map)
 * @return なし
 */
static void prt_frame_extra(void)
{
	prt_cut();
	prt_stun();
	prt_hunger();
	prt_state();
	prt_speed();
	prt_study();
	prt_imitation();
	prt_status();
}


/*!
 * @brief サブウィンドウに所持品一覧を表示する / Hack -- display p_ptr->inventory_list in sub-windows
 * @return なし
 */
static void fix_inven(void)
{
	int j;

	/* Scan windows */
	for (j = 0; j < 8; j++)
	{
		term *old = Term;

		/* No window */
		if (!angband_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] & (PW_INVEN))) continue;

		/* Activate */
		Term_activate(angband_term[j]);

		/* Display p_ptr->inventory_list */
		display_inven(item_tester_tval);
		Term_fresh();
		Term_activate(old);
	}
}


/*!
 * @brief モンスターの現在数を一行で表現する / Print monster info in line
 * @param x 表示列
 * @param y 表示行
 * @param m_ptr 思い出を表示するモンスター情報の参照ポインタ
 * @param n_same モンスターの数の現在数
 * @details
 * <pre>
 * nnn X LV name
 *  nnn : number or unique(U) or wanted unique(W)
 *  X   : symbol of monster
 *  LV  : monster lv if known
 *  name: name of monster
 * @return なし
 * </pre>
 */
static void print_monster_line(TERM_LEN x, TERM_LEN y, monster_type* m_ptr, int n_same){
	char buf[256];
	int i;
	MONRACE_IDX r_idx = m_ptr->ap_r_idx;
	monster_race* r_ptr = &r_info[r_idx];
 
	Term_gotoxy(x, y);
	if(!r_ptr)return;
	//Number of 'U'nique
	if(r_ptr->flags1&RF1_UNIQUE){//unique
		bool is_kubi = FALSE;
		for(i=0;i<MAX_KUBI;i++){
			if(current_world_ptr->bounty_r_idx[i] == r_idx){
				is_kubi = TRUE;
				break;
			}
		}
		Term_addstr(-1, TERM_WHITE, is_kubi?"  W":"  U");
	}else{
		sprintf(buf, "%3d", n_same);
		Term_addstr(-1, TERM_WHITE, buf);
	}
	//symbol
	Term_addstr(-1, TERM_WHITE, " ");
	//Term_add_bigch(r_ptr->d_attr, r_ptr->d_char);
	//Term_addstr(-1, TERM_WHITE, "/");
	Term_add_bigch(r_ptr->x_attr, r_ptr->x_char);
	//LV
	if (r_ptr->r_tkills && !(m_ptr->mflag2 & MFLAG2_KAGE)){
		sprintf(buf, " %2d", (int)r_ptr->level);
	}else{
		strcpy(buf, " ??");
	}
	Term_addstr(-1, TERM_WHITE, buf);
	//name
	sprintf(buf, " %s ", r_name+r_ptr->name);
	Term_addstr(-1, TERM_WHITE, buf);
 
	//Term_addstr(-1, TERM_WHITE, look_mon_desc(m_ptr, 0));
}

/*!
 * @brief モンスターの出現リストを表示する / Print monster info in line
 * @param x 表示列
 * @param y 表示行
 * @param max_lines 最大何行描画するか
 */
void print_monster_list(TERM_LEN x, TERM_LEN y, TERM_LEN max_lines){
	TERM_LEN line = y;
	monster_type* last_mons = NULL;
	monster_type* m_ptr = NULL;
	int n_same = 0;
	int i;

	for(i=0;i<tmp_pos.n;i++){
		grid_type* g_ptr = &p_ptr->current_floor_ptr->grid_array[tmp_pos.y[i]][tmp_pos.x[i]];
		if(!g_ptr->m_idx || !p_ptr->current_floor_ptr->m_list[g_ptr->m_idx].ml)continue;//no mons or cannot look
		m_ptr = &p_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
		if(is_pet(m_ptr))continue;//pet
		if(!m_ptr->r_idx)continue;//dead?
		{
			/*
			MONRACE_IDX r_idx = m_ptr->ap_r_idx;
			monster_race* r_ptr = &r_info[r_idx];
			concptr name = (r_name + r_ptr->name);
			concptr ename = (r_name + r_ptr->name);
			//ミミック類や「それ」等は、一覧に出てはいけない
			if(r_ptr->flags1&RF1_CHAR_CLEAR)continue;
			if((r_ptr->flags1&RF1_NEVER_MOVE)&&(r_ptr->flags2&RF2_CHAR_MULTI))continue;
			//『ヌル』は、一覧に出てはいけない
			if((strcmp(name, "生ける虚無『ヌル』")==0)||
			   (strcmp(ename, "Null the Living Void")==0))continue;
			//"金無垢の指輪"は、一覧に出てはいけない
			if((strcmp(name, "金無垢の指輪")==0)||
				(strcmp(ename, "Plain Gold Ring")==0))continue;
			*/
		}

		//ソート済みなので同じモンスターは連続する．これを利用して同じモンスターをカウント，まとめて表示する．
		if(!last_mons){//先頭モンスター
			last_mons = m_ptr;
			n_same = 1;
			continue;
		}
		//same race?
		if(last_mons->ap_r_idx == m_ptr->ap_r_idx){
			n_same++;
			continue;//表示処理を次に回す
		}
		//print last mons info
		print_monster_line(x, line++, last_mons, n_same);
		n_same = 1;
		last_mons = m_ptr;
		if(line-y-1==max_lines){//残り1行
			break;
		}
	}
	if(line-y-1==max_lines && i!=tmp_pos.n){
		Term_gotoxy(x, line);
		Term_addstr(-1, TERM_WHITE, "-- and more --");
	}else{
		if(last_mons)print_monster_line(x, line++, last_mons, n_same);
	}
}

/*!
 * @brief 出現中モンスターのリストをサブウィンドウに表示する / Hack -- display monster list in sub-windows
 * @return なし
 */
static void fix_monster_list(void)
{
	int j;
	int w, h;

	/* Scan windows */
	for (j = 0; j < 8; j++)
	{
		term *old = Term;

		/* No window */
		if (!angband_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] & (PW_MONSTER_LIST))) continue;

		/* Activate */
		Term_activate(angband_term[j]);
		Term_get_size(&w, &h);

		Term_clear();

		target_set_prepare_look();//モンスター一覧を生成，ソート
		print_monster_list(0, 0, h);
		Term_fresh();
		Term_activate(old);
	}
}



/*!
 * @brief 現在の装備品をサブウィンドウに表示する / 
 * Hack -- display equipment in sub-windows
 * @return なし
 */
static void fix_equip(void)
{
	int j;

	/* Scan windows */
	for (j = 0; j < 8; j++)
	{
		term *old = Term;

		/* No window */
		if (!angband_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] & (PW_EQUIP))) continue;

		/* Activate */
		Term_activate(angband_term[j]);

		/* Display equipment */
		display_equip(item_tester_tval);
		Term_fresh();
		Term_activate(old);
	}
}


/*!
 * @brief 現在の習得済魔法をサブウィンドウに表示する / 
 * Hack -- display spells in sub-windows
 * @return なし
 */
static void fix_spell(void)
{
	int j;

	/* Scan windows */
	for (j = 0; j < 8; j++)
	{
		term *old = Term;

		/* No window */
		if (!angband_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] & (PW_SPELL))) continue;

		/* Activate */
		Term_activate(angband_term[j]);

		/* Display spell list */
		display_spell_list(p_ptr);
		Term_fresh();
		Term_activate(old);
	}
}


/*!
 * @brief 現在のプレイヤーステータスをサブウィンドウに表示する / 
 * Hack -- display character in sub-windows
 * @return なし
 */
static void fix_player(void)
{
	int j;

	/* Scan windows */
	for (j = 0; j < 8; j++)
	{
		term *old = Term;

		/* No window */
		if (!angband_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] & (PW_PLAYER))) continue;

		/* Activate */
		Term_activate(angband_term[j]);

		update_playtime();
		display_player(p_ptr, 0);
		Term_fresh();
		Term_activate(old);
	}
}

/*!
 * @brief ゲームメッセージ履歴をサブウィンドウに表示する / 
 * Hack -- display recent messages in sub-windows
 * Adjust for width and split messages
 * @return なし
 */
static void fix_message(void)
{
	int j, i;
	TERM_LEN w, h;
	TERM_LEN x, y;

	/* Scan windows */
	for (j = 0; j < 8; j++)
	{
		term *old = Term;

		/* No window */
		if (!angband_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] & (PW_MESSAGE))) continue;

		/* Activate */
		Term_activate(angband_term[j]);

		Term_get_size(&w, &h);

		/* Dump messages */
		for (i = 0; i < h; i++)
		{
			/* Dump the message on the appropriate line */
			Term_putstr(0, (h - 1) - i, -1, (byte)((i < now_message) ? TERM_WHITE : TERM_SLATE), message_str((s16b)i));

			/* Cursor */
			Term_locate(&x, &y);

			/* Clear to end of line */
			Term_erase(x, y, 255);
		}
		Term_fresh();
		Term_activate(old);
	}
}


/*!
 * @brief 簡易マップをサブウィンドウに表示する / 
 * Hack -- display overhead view in sub-windows
 * Adjust for width and split messages
 * @return なし
 * @details
 * Note that the "player" symbol does NOT appear on the map.
 */
static void fix_overhead(void)
{
	int j;
	int cy, cx;

	/* Scan windows */
	for (j = 0; j < 8; j++)
	{
		term *old = Term;
		TERM_LEN wid, hgt;

		/* No window */
		if (!angband_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] & (PW_OVERHEAD))) continue;

		/* Activate */
		Term_activate(angband_term[j]);

		/* Full map in too small window is useless  */
		Term_get_size(&wid, &hgt);
		if (wid > COL_MAP + 2 && hgt > ROW_MAP + 2)
		{

			display_map(p_ptr->current_floor_ptr, &cy, &cx);
			Term_fresh();
		}
		Term_activate(old);
	}
}

static void display_dungeon(void)
{
	TERM_LEN x, y;
	TERM_COLOR a;
	SYMBOL_CODE c;

	TERM_COLOR ta = 0;
	SYMBOL_CODE tc = '\0';

	for (x = p_ptr->x - Term->wid / 2 + 1; x <= p_ptr->x + Term->wid / 2; x++)
	{
		for (y = p_ptr->y - Term->hgt / 2 + 1; y <= p_ptr->y + Term->hgt / 2; y++)
		{
			if (in_bounds2(p_ptr->current_floor_ptr, y, x))
			{
				map_info(y, x, &a, &c, &ta, &tc);

				/* Hack -- fake monochrome */
				if (!use_graphics)
				{
					if (current_world_ptr->timewalk_m_idx) a = TERM_DARK;
					else if (IS_INVULN() || p_ptr->timewalk) a = TERM_WHITE;
					else if (p_ptr->wraith_form) a = TERM_L_DARK;
				}

				/* Hack -- Queue it */
				Term_queue_char(x - p_ptr->x + Term->wid / 2 - 1, y - p_ptr->y + Term->hgt / 2 - 1, a, c, ta, tc);
			}
			else
			{
				/* Clear out-of-bound tiles */

				/* Access darkness */
				feature_type *f_ptr = &f_info[feat_none];

				/* Normal attr */
				a = f_ptr->x_attr[F_LIT_STANDARD];

				/* Normal char */
				c = f_ptr->x_char[F_LIT_STANDARD];

				/* Hack -- Queue it */
				Term_queue_char(x - p_ptr->x + Term->wid / 2 - 1, y - p_ptr->y + Term->hgt / 2 - 1, a, c, ta, tc);
			}
		}
	}
}

/*!
 * @brief ダンジョンの地形をサブウィンドウに表示する / 
 * Hack -- display dungeon view in sub-windows
 * @return なし
 */
static void fix_dungeon(void)
{
	int j;

	/* Scan windows */
	for (j = 0; j < 8; j++)
	{
		term *old = Term;

		/* No window */
		if (!angband_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] & (PW_DUNGEON))) continue;

		/* Activate */
		Term_activate(angband_term[j]);

		/* Redraw dungeon view */
		display_dungeon();
		Term_fresh();
		Term_activate(old);
	}
}


/*!
 * @brief モンスターの思い出をサブウィンドウに表示する / 
 * Hack -- display dungeon view in sub-windows
 * @return なし
 */
static void fix_monster(void)
{
	int j;

	/* Scan windows */
	for (j = 0; j < 8; j++)
	{
		term *old = Term;

		/* No window */
		if (!angband_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] & (PW_MONSTER))) continue;

		/* Activate */
		Term_activate(angband_term[j]);

		/* Display monster race info */
		if (p_ptr->monster_race_idx) display_roff(p_ptr->monster_race_idx);
		Term_fresh();
		Term_activate(old);
	}
}


/*!
 * @brief ベースアイテム情報をサブウィンドウに表示する / 
 * Hack -- display object recall in sub-windows
 * @return なし
 */
static void fix_object(void)
{
	int j;

	/* Scan windows */
	for (j = 0; j < 8; j++)
	{
		term *old = Term;

		/* No window */
		if (!angband_term[j]) continue;

		/* No relevant flags */
		if (!(window_flag[j] & (PW_OBJECT))) continue;

		/* Activate */
		Term_activate(angband_term[j]);

		/* Display monster race info */
		if (p_ptr->object_kind_idx) display_koff(p_ptr->object_kind_idx);
		Term_fresh();
		Term_activate(old);
	}
}



/*!
 * @brief 射撃武器がプレイヤーにとって重すぎるかどうかの判定 /
 * @param o_ptr 判定する射撃武器のアイテム情報参照ポインタ
 * @return 重すぎるならばTRUE
 */
bool is_heavy_shoot(player_type *creature_ptr, object_type *o_ptr)
{
	int hold = adj_str_hold[creature_ptr->stat_ind[A_STR]];
	/* It is hard to carholdry a heavy bow */
	return (hold < o_ptr->weight / 10);
}


/*! 
 * @brief p_ptr->redraw のフラグに応じた更新をまとめて行う / Handle "p_ptr->redraw"
 * @return なし
 * @details 更新処理の対象はゲーム中の全描画処理
 */
void redraw_stuff(void)
{
	if (!p_ptr->redraw) return;

	/* Character is not ready yet, no screen updates */
	if (!current_world_ptr->character_generated) return;

	/* Character is in "icky" mode, no screen updates */
	if (current_world_ptr->character_icky) return;

	/* Hack -- clear the screen */
	if (p_ptr->redraw & (PR_WIPE))
	{
		p_ptr->redraw &= ~(PR_WIPE);
		msg_print(NULL);
		Term_clear();
	}

	if (p_ptr->redraw & (PR_MAP))
	{
		p_ptr->redraw &= ~(PR_MAP);
		prt_map();
	}

	if (p_ptr->redraw & (PR_BASIC))
	{
		p_ptr->redraw &= ~(PR_BASIC);
		p_ptr->redraw &= ~(PR_MISC | PR_TITLE | PR_STATS);
		p_ptr->redraw &= ~(PR_LEV | PR_EXP | PR_GOLD);
		p_ptr->redraw &= ~(PR_ARMOR | PR_HP | PR_MANA);
		p_ptr->redraw &= ~(PR_DEPTH | PR_HEALTH | PR_UHEALTH);
		prt_frame_basic();
		prt_time();
		prt_dungeon();
	}

	if (p_ptr->redraw & (PR_EQUIPPY))
	{
		p_ptr->redraw &= ~(PR_EQUIPPY);
		print_equippy(); /* To draw / delete equippy chars */
	}

	if (p_ptr->redraw & (PR_MISC))
	{
		p_ptr->redraw &= ~(PR_MISC);
		prt_field(rp_ptr->title, ROW_RACE, COL_RACE);
/*		prt_field(cp_ptr->title, ROW_CLASS, COL_CLASS); */
	}

	if (p_ptr->redraw & (PR_TITLE))
	{
		p_ptr->redraw &= ~(PR_TITLE);
		prt_title();
	}

	if (p_ptr->redraw & (PR_LEV))
	{
		p_ptr->redraw &= ~(PR_LEV);
		prt_level();
	}

	if (p_ptr->redraw & (PR_EXP))
	{
		p_ptr->redraw &= ~(PR_EXP);
		prt_exp();
	}

	if (p_ptr->redraw & (PR_STATS))
	{
		p_ptr->redraw &= ~(PR_STATS);
		prt_stat(A_STR);
		prt_stat(A_INT);
		prt_stat(A_WIS);
		prt_stat(A_DEX);
		prt_stat(A_CON);
		prt_stat(A_CHR);
	}

	if (p_ptr->redraw & (PR_STATUS))
	{
		p_ptr->redraw &= ~(PR_STATUS);
		prt_status();
	}

	if (p_ptr->redraw & (PR_ARMOR))
	{
		p_ptr->redraw &= ~(PR_ARMOR);
		prt_ac();
	}

	if (p_ptr->redraw & (PR_HP))
	{
		p_ptr->redraw &= ~(PR_HP);
		prt_hp();
	}

	if (p_ptr->redraw & (PR_MANA))
	{
		p_ptr->redraw &= ~(PR_MANA);
		prt_sp();
	}

	if (p_ptr->redraw & (PR_GOLD))
	{
		p_ptr->redraw &= ~(PR_GOLD);
		prt_gold();
	}

	if (p_ptr->redraw & (PR_DEPTH))
	{
		p_ptr->redraw &= ~(PR_DEPTH);
		prt_depth();
	}

	if (p_ptr->redraw & (PR_HEALTH))
	{
		p_ptr->redraw &= ~(PR_HEALTH);
		health_redraw(FALSE);
	}

	if (p_ptr->redraw & (PR_UHEALTH))
	{
		p_ptr->redraw &= ~(PR_UHEALTH);
		health_redraw(TRUE);
	}

	if (p_ptr->redraw & (PR_EXTRA))
	{
		p_ptr->redraw &= ~(PR_EXTRA);
		p_ptr->redraw &= ~(PR_CUT | PR_STUN);
		p_ptr->redraw &= ~(PR_HUNGER);
		p_ptr->redraw &= ~(PR_STATE | PR_SPEED | PR_STUDY | PR_IMITATION | PR_STATUS);
		prt_frame_extra();
	}

	if (p_ptr->redraw & (PR_CUT))
	{
		p_ptr->redraw &= ~(PR_CUT);
		prt_cut();
	}

	if (p_ptr->redraw & (PR_STUN))
	{
		p_ptr->redraw &= ~(PR_STUN);
		prt_stun();
	}

	if (p_ptr->redraw & (PR_HUNGER))
	{
		p_ptr->redraw &= ~(PR_HUNGER);
		prt_hunger();
	}

	if (p_ptr->redraw & (PR_STATE))
	{
		p_ptr->redraw &= ~(PR_STATE);
		prt_state();
	}

	if (p_ptr->redraw & (PR_SPEED))
	{
		p_ptr->redraw &= ~(PR_SPEED);
		prt_speed();
	}

	if (p_ptr->pclass == CLASS_IMITATOR)
	{
		if (p_ptr->redraw & (PR_IMITATION))
		{
			p_ptr->redraw &= ~(PR_IMITATION);
			prt_imitation();
		}
	}
	else if (p_ptr->redraw & (PR_STUDY))
	{
		p_ptr->redraw &= ~(PR_STUDY);
		prt_study();
	}
}

/*! 
 * @brief p_ptr->window のフラグに応じた更新をまとめて行う / Handle "p_ptr->window"
 * @return なし
 * @details 更新処理の対象はサブウィンドウ全般
 */
void window_stuff(void)
{
	int j;
	BIT_FLAGS mask = 0L;

	/* Nothing to do */
	if (!p_ptr->window) return;

	/* Scan windows */
	for (j = 0; j < 8; j++)
	{
		/* Save usable flags */
		if (angband_term[j]) mask |= window_flag[j];
	}

	/* Apply usable flags */
	p_ptr->window &= mask;

	/* Nothing to do */
	if (!p_ptr->window) return;

	/* Display p_ptr->inventory_list */
	if (p_ptr->window & (PW_INVEN))
	{
		p_ptr->window &= ~(PW_INVEN);
		fix_inven();
	}

	/* Display equipment */
	if (p_ptr->window & (PW_EQUIP))
	{
		p_ptr->window &= ~(PW_EQUIP);
		fix_equip();
	}

	/* Display spell list */
	if (p_ptr->window & (PW_SPELL))
	{
		p_ptr->window &= ~(PW_SPELL);
		fix_spell();
	}

	/* Display player */
	if (p_ptr->window & (PW_PLAYER))
	{
		p_ptr->window &= ~(PW_PLAYER);
		fix_player();
	}
	
	/* Display monster list */
	if (p_ptr->window & (PW_MONSTER_LIST))
	{
		p_ptr->window &= ~(PW_MONSTER_LIST);
		fix_monster_list();
	}
	
	/* Display overhead view */
	if (p_ptr->window & (PW_MESSAGE))
	{
		p_ptr->window &= ~(PW_MESSAGE);
		fix_message();
	}

	/* Display overhead view */
	if (p_ptr->window & (PW_OVERHEAD))
	{
		p_ptr->window &= ~(PW_OVERHEAD);
		fix_overhead();
	}

	/* Display overhead view */
	if (p_ptr->window & (PW_DUNGEON))
	{
		p_ptr->window &= ~(PW_DUNGEON);
		fix_dungeon();
	}

	/* Display monster recall */
	if (p_ptr->window & (PW_MONSTER))
	{
		p_ptr->window &= ~(PW_MONSTER);
		fix_monster();
	}

	/* Display object recall */
	if (p_ptr->window & (PW_OBJECT))
	{
		p_ptr->window &= ~(PW_OBJECT);
		fix_object();
	}
}

/*!
 * @brief コンソールのリサイズに合わせてマップを再描画する /
 * Map resizing whenever the main term changes size
 * @return なし
 */
void resize_map(void)
{
	/* Only if the dungeon exists */
	if (!current_world_ptr->character_dungeon) return;

	/* Mega-Hack -- no panel yet */
	panel_row_max = 0;
	panel_col_max = 0;

	/* Reset the panels */
	panel_row_min = p_ptr->current_floor_ptr->height;
	panel_col_min = p_ptr->current_floor_ptr->width;

	verify_panel();

	p_ptr->update |= (PU_TORCH | PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
	p_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_MON_LITE);
	p_ptr->update |= (PU_MONSTERS);
	p_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY);

	handle_stuff();
	Term_redraw();

	/*
	 * Waiting command;
	 * Place the cursor on the player
	 */
	if (can_save) move_cursor_relative(p_ptr->y, p_ptr->x);

	Term_fresh();
}

/*!
 * @brief コンソールを再描画する /
 * Redraw a term when it is resized
 * @return なし
 */
void redraw_window(void)
{
	/* Only if the dungeon exists */
	if (!current_world_ptr->character_dungeon) return;

	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER);
	p_ptr->window |= (PW_MESSAGE | PW_OVERHEAD | PW_DUNGEON | PW_MONSTER | PW_OBJECT);

	handle_stuff();
	Term_redraw();
}


/*!
 * @brief フォーカスを当てるべきマップ描画の基準座標を指定する（サブルーチン）
 * @param dy 変更先のフロアY座標
 * @param dx 変更先のフロアX座標
 * Handle a request to change the current panel
 * Return TRUE if the panel was changed.
 * Also used in do_cmd_locate
 * @return 実際に再描画が必要だった場合TRUEを返す
 */
bool change_panel(POSITION dy, POSITION dx)
{
	POSITION y, x;
	TERM_LEN wid, hgt;

	get_screen_size(&wid, &hgt);

	/* Apply the motion */
	y = panel_row_min + dy * hgt / 2;
	x = panel_col_min + dx * wid / 2;

	/* Verify the row */
	if (y > p_ptr->current_floor_ptr->height - hgt) y = p_ptr->current_floor_ptr->height - hgt;
	if (y < 0) y = 0;

	/* Verify the col */
	if (x > p_ptr->current_floor_ptr->width - wid) x = p_ptr->current_floor_ptr->width - wid;
	if (x < 0) x = 0;

	/* Handle "changes" */
	if ((y != panel_row_min) || (x != panel_col_min))
	{
		/* Save the new panel info */
		panel_row_min = y;
		panel_col_min = x;

		panel_bounds_center();

		p_ptr->update |= (PU_MONSTERS);
		p_ptr->redraw |= (PR_MAP);
		handle_stuff();

		/* Success */
		return (TRUE);
	}

	/* No change */
	return (FALSE);
}

/*!
 * @brief プレイヤーの装備一覧シンボルを固定位置に表示する
 * @return なし
 */
void print_equippy(void)
{
	display_player_equippy(ROW_EQUIPPY, COL_EQUIPPY, 0);
}

/*!
 * @brief 現在のコンソール表示の縦横を返す。 /
 * Get term size and calculate screen size
 * @param wid_p コンソールの表示幅文字数を返す
 * @param hgt_p コンソールの表示行数を返す
 * @return なし
 */
void get_screen_size(TERM_LEN *wid_p, TERM_LEN *hgt_p)
{
	Term_get_size(wid_p, hgt_p);
	*hgt_p -= ROW_MAP + 2;
	*wid_p -= COL_MAP + 2;
	if (use_bigtile) *wid_p /= 2;
}

/*
 * Calculate panel colum of a location in the map
 */
int panel_col_of(int col)
{
	col -= panel_col_min;
	if (use_bigtile) col *= 2;
	return col + 13;
}

/*
 * Prints the map of the dungeon
 *
 * Note that, for efficiency, we contain an "optimized" version
 * of both "lite_spot()" and "print_rel()", and that we use the
 * "lite_spot()" function to display the player grid, if needed.
 */
void prt_map(void)
{
	POSITION x, y;
	int v;

	/* map bounds */
	POSITION xmin, xmax, ymin, ymax;

	TERM_LEN wid, hgt;

	Term_get_size(&wid, &hgt);

	/* Remove map offset */
	wid -= COL_MAP + 2;
	hgt -= ROW_MAP + 2;

	/* Access the cursor state */
	(void)Term_get_cursor(&v);

	/* Hide the cursor */
	(void)Term_set_cursor(0);

	/* Get bounds */
	xmin = (0 < panel_col_min) ? panel_col_min : 0;
	xmax = (p_ptr->current_floor_ptr->width - 1 > panel_col_max) ? panel_col_max : p_ptr->current_floor_ptr->width - 1;
	ymin = (0 < panel_row_min) ? panel_row_min : 0;
	ymax = (p_ptr->current_floor_ptr->height - 1 > panel_row_max) ? panel_row_max : p_ptr->current_floor_ptr->height - 1;

	/* Bottom section of screen */
	for (y = 1; y <= ymin - panel_row_prt; y++)
	{
		/* Erase the section */
		Term_erase(COL_MAP, y, wid);
	}

	/* Top section of screen */
	for (y = ymax - panel_row_prt; y <= hgt; y++)
	{
		/* Erase the section */
		Term_erase(COL_MAP, y, wid);
	}

	/* Dump the map */
	for (y = ymin; y <= ymax; y++)
	{
		/* Scan the columns of row "y" */
		for (x = xmin; x <= xmax; x++)
		{
			TERM_COLOR a;
			SYMBOL_CODE c;

			TERM_COLOR ta;
			SYMBOL_CODE tc;

			/* Determine what is there */
			map_info(y, x, &a, &c, &ta, &tc);

			/* Hack -- fake monochrome */
			if (!use_graphics)
			{
				if (current_world_ptr->timewalk_m_idx) a = TERM_DARK;
				else if (IS_INVULN() || p_ptr->timewalk) a = TERM_WHITE;
				else if (p_ptr->wraith_form) a = TERM_L_DARK;
			}

			/* Efficiency -- Redraw that grid of the map */
			Term_queue_bigchar(panel_col_of(x), y - panel_row_prt, a, c, ta, tc);
		}
	}

	/* Display player */
	lite_spot(p_ptr->y, p_ptr->x);

	/* Restore the cursor */
	(void)Term_set_cursor(v);
}



/*!
 * 一般的にモンスターシンボルとして扱われる記号を定義する(幻覚処理向け) / Hack -- Legal monster codes
 */
static char image_monster_hack[] = \
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

/*!
 * 一般的にオブジェクトシンボルとして扱われる記号を定義する(幻覚処理向け) /  Hack -- Legal object codes
 */
static char image_object_hack[] = "?/|\\\"!$()_-=[]{},~";

/*!
 * @brief モンスターの表示を幻覚状態に差し替える / Mega-Hack -- Hallucinatory monster
 * @param ap 本来の色
 * @param cp 本来のシンボル
 * @return なし
 */
static void image_monster(TERM_COLOR *ap, SYMBOL_CODE *cp)
{
	/* Random symbol from set above */
	if (use_graphics)
	{
		monster_race *r_ptr = &r_info[randint1(max_r_idx - 1)];

		*cp = r_ptr->x_char;
		*ap = r_ptr->x_attr;
	}
	else
		/* Text mode */
	{
		*cp = (one_in_(25) ?
			image_object_hack[randint0(sizeof(image_object_hack) - 1)] :
			image_monster_hack[randint0(sizeof(image_monster_hack) - 1)]);

		/* Random color */
		*ap = randint1(15);
	}
}

/*!
 * @brief オブジェクトの表示を幻覚状態に差し替える / Hallucinatory object
 * @param ap 本来の色
 * @param cp 本来のシンボル
 * @return なし
 */
static void image_object(TERM_COLOR *ap, SYMBOL_CODE *cp)
{
	if (use_graphics)
	{
		object_kind *k_ptr = &k_info[randint1(max_k_idx - 1)];

		*cp = k_ptr->x_char;
		*ap = k_ptr->x_attr;
	}
	else
	{
		int n = sizeof(image_object_hack) - 1;

		*cp = image_object_hack[randint0(n)];

		/* Random color */
		*ap = randint1(15);
	}
}


/*!
 * @brief オブジェクト＆モンスターの表示を幻覚状態に差し替える / Hack -- Random hallucination
 * @param ap 本来の色
 * @param cp 本来のシンボル
 * @return なし
 */
static void image_random(TERM_COLOR *ap, SYMBOL_CODE *cp)
{
	/* Normally, assume monsters */
	if (randint0(100) < 75)
	{
		image_monster(ap, cp);
	}

	/* Otherwise, assume objects */
	else
	{
		image_object(ap, cp);
	}
}

/*!
 * 照明の表現を行うための色合いの関係を{暗闇時, 照明時} で定義する /
 * This array lists the effects of "brightness" on various "base" colours.\n
 *\n
 * This is used to do dynamic lighting effects in ascii :-)\n
 * At the moment, only the various "floor" tiles are affected.\n
 *\n
 * The layout of the array is [x][0] = light and [x][1] = dark.\n
 */
static TERM_COLOR lighting_colours[16][2] =
{
	/* TERM_DARK */
	{TERM_L_DARK, TERM_DARK},

	/* TERM_WHITE */
	{TERM_YELLOW, TERM_SLATE},

	/* TERM_SLATE */
	{TERM_WHITE, TERM_L_DARK},

	/* TERM_ORANGE */
	{TERM_L_UMBER, TERM_UMBER},

	/* TERM_RED */
	{TERM_RED, TERM_RED},

	/* TERM_GREEN */
	{TERM_L_GREEN, TERM_GREEN},

	/* TERM_BLUE */
	{TERM_BLUE, TERM_BLUE},

	/* TERM_UMBER */
	{TERM_L_UMBER, TERM_RED},

	/* TERM_L_DARK */
	{TERM_SLATE, TERM_L_DARK},

	/* TERM_L_WHITE */
	{TERM_WHITE, TERM_SLATE},

	/* TERM_VIOLET */
	{TERM_L_RED, TERM_BLUE},

	/* TERM_YELLOW */
	{TERM_YELLOW, TERM_ORANGE},

	/* TERM_L_RED */
	{TERM_L_RED, TERM_L_RED},

	/* TERM_L_GREEN */
	{TERM_L_GREEN, TERM_GREEN},

	/* TERM_L_BLUE */
	{TERM_L_BLUE, TERM_L_BLUE},

	/* TERM_L_UMBER */
	{TERM_L_UMBER, TERM_UMBER}
};


/*!
 * @brief 調査中
 * @todo コメントを付加すること
 */
void apply_default_feat_lighting(TERM_COLOR f_attr[F_LIT_MAX], SYMBOL_CODE f_char[F_LIT_MAX])
{
	TERM_COLOR s_attr = f_attr[F_LIT_STANDARD];
	SYMBOL_CODE s_char = f_char[F_LIT_STANDARD];
	int i;

	if (IS_ASCII_GRAPHICS(s_attr)) /* For ASCII */
	{
		f_attr[F_LIT_LITE] = lighting_colours[s_attr & 0x0f][0];
		f_attr[F_LIT_DARK] = lighting_colours[s_attr & 0x0f][1];
		for (i = F_LIT_NS_BEGIN; i < F_LIT_MAX; i++) f_char[i] = s_char;
	}
	else /* For tile graphics */
	{
		for (i = F_LIT_NS_BEGIN; i < F_LIT_MAX; i++) f_attr[i] = s_attr;
		f_char[F_LIT_LITE] = s_char + 2;
		f_char[F_LIT_DARK] = s_char + 1;
	}
}


/*!
 * @brief Mコマンドによる縮小マップの表示を行う / Extract the attr/char to display at the given (legal) map location
 * @details
 * Basically, we "paint" the chosen attr/char in several passes, starting\n
 * with any known "terrain features" (defaulting to darkness), then adding\n
 * any known "objects", and finally, adding any known "monsters".  This\n
 * is not the fastest method but since most of the calls to this function\n
 * are made for grids with no monsters or objects, it is fast enough.\n
 *\n
 * Note that this function, if used on the grid containing the "player",\n
 * will return the attr/char of the grid underneath the player, and not\n
 * the actual player attr/char itself, allowing a lot of optimization\n
 * in various "display" functions.\n
 *\n
 * Note that the "zero" entry in the feature/object/monster arrays are\n
 * used to provide "special" attr/char codes, with "monster zero" being\n
 * used for the player attr/char, "object zero" being used for the "stack"\n
 * attr/char, and "feature zero" being used for the "nothing" attr/char,\n
 * though this function makes use of only "feature zero".\n
 *\n
 * Note that monsters can have some "special" flags, including "ATTR_MULTI",\n
 * which means their color changes, and "ATTR_CLEAR", which means they take\n
 * the color of whatever is under them, and "CHAR_CLEAR", which means that\n
 * they take the symbol of whatever is under them.  Technically, the flag\n
 * "CHAR_MULTI" is supposed to indicate that a monster looks strange when\n
 * examined, but this flag is currently ignored.\n
 *\n
 * Currently, we do nothing with multi-hued objects, because there are\n
 * not any.  If there were, they would have to set "shimmer_objects"\n
 * when they were created, and then new "shimmer" code in "dungeon.c"\n
 * would have to be created handle the "shimmer" effect, and the code\n
 * in "p_ptr->current_floor_ptr->grid_array.c" would have to be updated to create the shimmer effect.\n
 *\n
 * Note the effects of hallucination.  Objects always appear as random\n
 * "objects", monsters as random "monsters", and normal grids occasionally\n
 * appear as random "monsters" or "objects", but note that these random\n
 * "monsters" and "objects" are really just "colored ascii symbols".\n
 *\n
 * Note that "floors" and "invisible traps" (and "zero" features) are\n
 * drawn as "floors" using a special check for optimization purposes,\n
 * and these are the only features which get drawn using the special\n
 * lighting effects activated by "view_special_lite".\n
 *\n
 * Note the use of the "mimic" field in the "terrain feature" processing,\n
 * which allows any feature to "pretend" to be another feature.  This is\n
 * used to "hide" secret doors, and to make all "doors" appear the same,\n
 * and all "walls" appear the same, and "hidden" treasure stay hidden.\n
 * It is possible to use this field to make a feature "look" like a floor,\n
 * but the "special lighting effects" for floors will not be used.\n
 *\n
 * Note the use of the new "terrain feature" information.  Note that the\n
 * assumption that all interesting "objects" and "terrain features" are\n
 * memorized allows extremely optimized processing below.  Note the use\n
 * of separate flags on objects to mark them as memorized allows a grid\n
 * to have memorized "terrain" without granting knowledge of any object\n
 * which may appear in that grid.\n
 *\n
 * Note the efficient code used to determine if a "floor" grid is\n
 * "memorized" or "viewable" by the player, where the test for the\n
 * grid being "viewable" is based on the facts that (1) the grid\n
 * must be "lit" (torch-lit or perma-lit), (2) the grid must be in\n
 * line of sight, and (3) the player must not be blind, and uses the\n
 * assumption that all torch-lit grids are in line of sight.\n
 *\n
 * Note that floors (and invisible traps) are the only grids which are\n
 * not memorized when seen, so only these grids need to check to see if\n
 * the grid is "viewable" to the player (if it is not memorized).  Since\n
 * most non-memorized grids are in fact walls, this induces *massive*\n
 * efficiency, at the cost of *forcing* the memorization of non-floor\n
 * grids when they are first seen.  Note that "invisible traps" are\n
 * always treated exactly like "floors", which prevents "cheating".\n
 *\n
 * Note the "special lighting effects" which can be activated for floor\n
 * grids using the "view_special_lite" option (for "white" floor grids),\n
 * causing certain grids to be displayed using special colors.  If the\n
 * player is "blind", we will use "dark gray", else if the grid is lit\n
 * by the torch, and the "view_yellow_lite" option is set, we will use\n
 * "yellow", else if the grid is "dark", we will use "dark gray", else\n
 * if the grid is not "viewable", and the "view_bright_lite" option is\n
 * set, and the we will use "slate" (gray).  We will use "white" for all\n
 * other cases, in particular, for illuminated viewable floor grids.\n
 *\n
 * Note the "special lighting effects" which can be activated for wall\n
 * grids using the "view_granite_lite" option (for "white" wall grids),\n
 * causing certain grids to be displayed using special colors.  If the\n
 * player is "blind", we will use "dark gray", else if the grid is lit\n
 * by the torch, and the "view_yellow_lite" option is set, we will use\n
 * "yellow", else if the "view_bright_lite" option is set, and the grid\n
 * is not "viewable", or is "dark", or is glowing, but not when viewed\n
 * from the player's current location, we will use "slate" (gray).  We\n
 * will use "white" for all other cases, in particular, for correctly\n
 * illuminated viewable wall grids.\n
 *\n
 * Note that, when "view_granite_lite" is set, we use an inline version\n
 * of the "player_can_see_bold()" function to check the "viewability" of\n
 * grids when the "view_bright_lite" option is set, and we do NOT use\n
 * any special colors for "dark" wall grids, since this would allow the\n
 * player to notice the walls of illuminated rooms from a hallway that\n
 * happened to run beside the room.  The alternative, by the way, would\n
 * be to prevent the generation of hallways next to rooms, but this\n
 * would still allow problems when digging towards a room.\n
 *\n
 * Note that bizarre things must be done when the "attr" and/or "char"\n
 * codes have the "high-bit" set, since these values are used to encode\n
 * various "special" pictures in some versions, and certain situations,\n
 * such as "multi-hued" or "clear" monsters, cause the attr/char codes\n
 * to be "scrambled" in various ways.\n
 *\n
 * Note that eventually we may use the "&" symbol for embedded treasure,\n
 * and use the "*" symbol to indicate multiple objects, though this will\n
 * have to wait for Angband 2.8.0 or later.  Note that currently, this\n
 * is not important, since only one object or terrain feature is allowed\n
 * in each grid.  If needed, "k_info[0]" will hold the "stack" attr/char.\n
 *\n
 * Note the assumption that doing "x_ptr = &x_info[x]" plus a few of\n
 * "x_ptr->xxx", is quicker than "x_info[x].xxx", if this is incorrect\n
 * then a whole lot of code should be changed...  XXX XXX\n
 */
void map_info(POSITION y, POSITION x, TERM_COLOR *ap, SYMBOL_CODE *cp, TERM_COLOR *tap, SYMBOL_CODE *tcp)
{
	/* Get the p_ptr->current_floor_ptr->grid_array */
	grid_type *g_ptr = &p_ptr->current_floor_ptr->grid_array[y][x];

	OBJECT_IDX this_o_idx, next_o_idx = 0;

	/* Feature code (applying "mimic" field) */
	FEAT_IDX feat = get_feat_mimic(g_ptr);

	/* Access floor */
	feature_type *f_ptr = &f_info[feat];

	TERM_COLOR a;
	SYMBOL_CODE c;

	/* Boring grids (floors, etc) */
	if (!have_flag(f_ptr->flags, FF_REMEMBER))
	{
		/*
		 * Handle Memorized or visible floor
		 *
		 * No visual when blinded.
		 *   (to prevent strange effects on darkness breath)
		 * otherwise,
		 * - Can see grids with CAVE_MARK.
		 * - Can see grids with CAVE_LITE or CAVE_MNLT.
		 *   (Such grids also have CAVE_VIEW)
		 * - Can see grids with CAVE_VIEW unless darkened by monsters.
		 */
		if (!p_ptr->blind &&
			((g_ptr->info & (CAVE_MARK | CAVE_LITE | CAVE_MNLT)) ||
			((g_ptr->info & CAVE_VIEW) && (((g_ptr->info & (CAVE_GLOW | CAVE_MNDK)) == CAVE_GLOW) || p_ptr->see_nocto))))
		{
			/* Normal attr/char */
			a = f_ptr->x_attr[F_LIT_STANDARD];
			c = f_ptr->x_char[F_LIT_STANDARD];

			if (p_ptr->wild_mode)
			{
				/* Special lighting effects */
				/* Handle "night" */
				if (view_special_lite && !is_daytime())
				{
					/* Use a darkened colour/tile */
					a = f_ptr->x_attr[F_LIT_DARK];
					c = f_ptr->x_char[F_LIT_DARK];
				}
			}

			/* Mega-Hack -- Handle "in-sight" and "darkened" grids */
			else if (darkened_grid(g_ptr))
			{
				/* Unsafe grid -- idea borrowed from Unangband */
				feat = (view_unsafe_grids && (g_ptr->info & CAVE_UNSAFE)) ? feat_undetected : feat_none;

				/* Access darkness */
				f_ptr = &f_info[feat];

				/* Char and attr of darkness */
				a = f_ptr->x_attr[F_LIT_STANDARD];
				c = f_ptr->x_char[F_LIT_STANDARD];
			}

			/* Special lighting effects */
			else if (view_special_lite)
			{
				/* Handle "torch-lit" grids */
				if (g_ptr->info & (CAVE_LITE | CAVE_MNLT))
				{
					/* Torch lite */
					if (view_yellow_lite)
					{
						/* Use a brightly lit colour/tile */
						a = f_ptr->x_attr[F_LIT_LITE];
						c = f_ptr->x_char[F_LIT_LITE];
					}
				}

				/* Handle "dark" grids */
				else if ((g_ptr->info & (CAVE_GLOW | CAVE_MNDK)) != CAVE_GLOW)
				{
					/* Use a darkened colour/tile */
					a = f_ptr->x_attr[F_LIT_DARK];
					c = f_ptr->x_char[F_LIT_DARK];
				}

				/* Handle "out-of-sight" grids */
				else if (!(g_ptr->info & CAVE_VIEW))
				{
					/* Special flag */
					if (view_bright_lite)
					{
						/* Use a darkened colour/tile */
						a = f_ptr->x_attr[F_LIT_DARK];
						c = f_ptr->x_char[F_LIT_DARK];
					}
				}
			}
		}

		/* Unknown */
		else
		{
			/* Unsafe grid -- idea borrowed from Unangband */
			feat = (view_unsafe_grids && (g_ptr->info & CAVE_UNSAFE)) ? feat_undetected : feat_none;

			/* Access darkness */
			f_ptr = &f_info[feat];

			/* Normal attr/char */
			a = f_ptr->x_attr[F_LIT_STANDARD];
			c = f_ptr->x_char[F_LIT_STANDARD];
		}
	}

	/* Interesting grids (non-floors) */
	else
	{
		/* Memorized grids */
		if (g_ptr->info & CAVE_MARK)
		{
			/* Normal attr/char */
			a = f_ptr->x_attr[F_LIT_STANDARD];
			c = f_ptr->x_char[F_LIT_STANDARD];

			if (p_ptr->wild_mode)
			{
				/* Special lighting effects */
				/* Handle "blind" or "night" */
				if (view_granite_lite && (p_ptr->blind || !is_daytime()))
				{
					/* Use a darkened colour/tile */
					a = f_ptr->x_attr[F_LIT_DARK];
					c = f_ptr->x_char[F_LIT_DARK];
				}
			}

			/* Mega-Hack -- Handle "in-sight" and "darkened" grids */
			else if (darkened_grid(g_ptr) && !p_ptr->blind)
			{
				if (have_flag(f_ptr->flags, FF_LOS) && have_flag(f_ptr->flags, FF_PROJECT))
				{
					/* Unsafe grid -- idea borrowed from Unangband */
					feat = (view_unsafe_grids && (g_ptr->info & CAVE_UNSAFE)) ? feat_undetected : feat_none;

					/* Access darkness */
					f_ptr = &f_info[feat];

					/* Char and attr of darkness */
					a = f_ptr->x_attr[F_LIT_STANDARD];
					c = f_ptr->x_char[F_LIT_STANDARD];
				}
				else if (view_granite_lite && view_bright_lite)
				{
					/* Use a darkened colour/tile */
					a = f_ptr->x_attr[F_LIT_DARK];
					c = f_ptr->x_char[F_LIT_DARK];
				}
			}

			/* Special lighting effects */
			else if (view_granite_lite)
			{
				/* Handle "blind" */
				if (p_ptr->blind)
				{
					/* Use a darkened colour/tile */
					a = f_ptr->x_attr[F_LIT_DARK];
					c = f_ptr->x_char[F_LIT_DARK];
				}

				/* Handle "torch-lit" grids */
				else if (g_ptr->info & (CAVE_LITE | CAVE_MNLT))
				{
					/* Torch lite */
					if (view_yellow_lite)
					{
						/* Use a brightly lit colour/tile */
						a = f_ptr->x_attr[F_LIT_LITE];
						c = f_ptr->x_char[F_LIT_LITE];
					}
				}

				/* Handle "view_bright_lite" */
				else if (view_bright_lite)
				{
					/* Not viewable */
					if (!(g_ptr->info & CAVE_VIEW))
					{
						/* Use a darkened colour/tile */
						a = f_ptr->x_attr[F_LIT_DARK];
						c = f_ptr->x_char[F_LIT_DARK];
					}

					/* Not glowing */
					else if ((g_ptr->info & (CAVE_GLOW | CAVE_MNDK)) != CAVE_GLOW)
					{
						/* Use a darkened colour/tile */
						a = f_ptr->x_attr[F_LIT_DARK];
						c = f_ptr->x_char[F_LIT_DARK];
					}

					/* Not glowing correctly */
					else if (!have_flag(f_ptr->flags, FF_LOS) && !check_local_illumination(y, x))
					{
						/* Use a darkened colour/tile */
						a = f_ptr->x_attr[F_LIT_DARK];
						c = f_ptr->x_char[F_LIT_DARK];
					}
				}
			}
		}

		/* Unknown */
		else
		{
			/* Unsafe grid -- idea borrowed from Unangband */
			feat = (view_unsafe_grids && (g_ptr->info & CAVE_UNSAFE)) ? feat_undetected : feat_none;

			/* Access feature */
			f_ptr = &f_info[feat];

			/* Normal attr/char */
			a = f_ptr->x_attr[F_LIT_STANDARD];
			c = f_ptr->x_char[F_LIT_STANDARD];
		}
	}

	if (feat_priority == -1) feat_priority = f_ptr->priority;

	/* Save the terrain info for the transparency effects */
	(*tap) = a;
	(*tcp) = c;

	/* Save the info */
	(*ap) = a;
	(*cp) = c;

	/* Hack -- rare random hallucination, except on outer dungeon walls */
	if (p_ptr->image)
	{
		if (one_in_(256))
		{
			image_random(ap, cp);
		}
	}

	/* Objects */
	for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;
		o_ptr = &p_ptr->current_floor_ptr->o_list[this_o_idx];
		next_o_idx = o_ptr->next_o_idx;

		/* Memorized objects */
		if (o_ptr->marked & OM_FOUND)
		{
			if (display_autopick)
			{
				byte act;

				match_autopick = is_autopick(o_ptr);
				if (match_autopick == -1)
					continue;

				act = autopick_list[match_autopick].action;

				if ((act & DO_DISPLAY) && (act & display_autopick))
				{
					autopick_obj = o_ptr;
				}
				else
				{
					match_autopick = -1;
					continue;
				}
			}
			/* Normal char */
			(*cp) = object_char(o_ptr);

			/* Normal attr */
			(*ap) = object_attr(o_ptr);

			feat_priority = 20;

			/* Hack -- hallucination */
			if (p_ptr->image) image_object(ap, cp);

			break;
		}
	}


	/* Handle monsters */
	if (g_ptr->m_idx && display_autopick == 0)
	{
		monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[g_ptr->m_idx];

		/* Visible monster */
		if (m_ptr->ml)
		{
			monster_race *r_ptr = &r_info[m_ptr->ap_r_idx];

			feat_priority = 30;

			/* Hallucination */
			if (p_ptr->image)
			{
				/*
				 * Monsters with both CHAR_CLEAR and ATTR_CLEAR
				 * flags are always unseen.
				 */
				if ((r_ptr->flags1 & (RF1_CHAR_CLEAR | RF1_ATTR_CLEAR)) == (RF1_CHAR_CLEAR | RF1_ATTR_CLEAR))
				{
					/* Do nothing */
				}
				else
				{
					image_monster(ap, cp);
				}
			}
			else
			{
				/* Monster attr/char */
				a = r_ptr->x_attr;
				c = r_ptr->x_char;

				if (!(r_ptr->flags1 & (RF1_CHAR_CLEAR | RF1_SHAPECHANGER | RF1_ATTR_CLEAR
					| RF1_ATTR_MULTI | RF1_ATTR_SEMIRAND)))
				{
					/* Desired monster attr/char */
					*ap = a;
					*cp = c;
				}

				/*
				 * Monsters with both CHAR_CLEAR and ATTR_CLEAR
				 * flags are always unseen.
				 */
				else if ((r_ptr->flags1 & (RF1_CHAR_CLEAR | RF1_ATTR_CLEAR)) == (RF1_CHAR_CLEAR | RF1_ATTR_CLEAR))
				{
					/* Do nothing */
				}

				else
				{
					/***  Monster's attr  ***/
					if ((r_ptr->flags1 & RF1_ATTR_CLEAR) && (*ap != TERM_DARK) && !use_graphics)
					{
						/* Clear-attr */
						/* Do nothing */
					}
					else if ((r_ptr->flags1 & RF1_ATTR_MULTI) && !use_graphics)
					{
						/* Multi-hued attr */
						if (r_ptr->flags2 & RF2_ATTR_ANY) *ap = randint1(15);
						else switch (randint1(7))
						{
						case 1: *ap = TERM_RED;     break;
						case 2: *ap = TERM_L_RED;   break;
						case 3: *ap = TERM_WHITE;   break;
						case 4: *ap = TERM_L_GREEN; break;
						case 5: *ap = TERM_BLUE;    break;
						case 6: *ap = TERM_L_DARK;  break;
						case 7: *ap = TERM_GREEN;   break;
						}
					}
					else if ((r_ptr->flags1 & RF1_ATTR_SEMIRAND) && !use_graphics)
					{
						/* Use semi-random attr (usually mimics' colors vary) */
						*ap = g_ptr->m_idx % 15 + 1;
					}
					else
					{
						/* Normal case */
						*ap = a;
					}

					/***  Monster's char  ***/
					if ((r_ptr->flags1 & RF1_CHAR_CLEAR) && (*cp != ' ') && !use_graphics)
					{
						/* Clear-char */
						/* Do nothing */
					}
					else if (r_ptr->flags1 & RF1_SHAPECHANGER)
					{
						if (use_graphics)
						{
							monster_race *tmp_r_ptr = &r_info[randint1(max_r_idx - 1)];
							*cp = tmp_r_ptr->x_char;
							*ap = tmp_r_ptr->x_attr;
						}
						else
						{
							*cp = (one_in_(25) ?
								image_object_hack[randint0(sizeof(image_object_hack) - 1)] :
								image_monster_hack[randint0(sizeof(image_monster_hack) - 1)]);
						}
					}
					else
					{
						/* Normal case */
						*cp = c;
					}
				}
			}
		}
	}

	/* Handle "player" */
	if (player_bold(p_ptr, y, x))
	{
		monster_race *r_ptr = &r_info[0];
		*ap = r_ptr->x_attr;
		*cp = r_ptr->x_char;
		feat_priority = 31;
	}
}


static concptr simplify_list[][2] =
{
#ifdef JP
	{"の魔法書", ""},
	{NULL, NULL}
#else
	{"^Ring of ",   "="},
	{"^Amulet of ", "\""},
	{"^Scroll of ", "?"},
	{"^Scroll titled ", "?"},
	{"^Wand of "  , "-"},
	{"^Rod of "   , "-"},
	{"^Staff of " , "_"},
	{"^Potion of ", "!"},
	{" Spellbook ",""},
	{"^Book of ",   ""},
	{" Magic [",   "["},
	{" Book [",    "["},
	{" Arts [",    "["},
	{"^Set of ",    ""},
	{"^Pair of ",   ""},
	{NULL, NULL}
#endif
};


static void display_shortened_item_name(object_type *o_ptr, int y)
{
	char buf[MAX_NLEN];
	char *c = buf;
	int len = 0;
	TERM_COLOR attr;

	object_desc(buf, o_ptr, (OD_NO_FLAVOR | OD_OMIT_PREFIX | OD_NAME_ONLY));
	attr = tval_to_attr[o_ptr->tval % 128];

	if (p_ptr->image)
	{
		attr = TERM_WHITE;
		strcpy(buf, _("何か奇妙な物", "something strange"));
	}

	for (c = buf; *c; c++)
	{
		int i;
		for (i = 0; simplify_list[i][1]; i++)
		{
			concptr org_w = simplify_list[i][0];

			if (*org_w == '^')
			{
				if (c == buf)
					org_w++;
				else
					continue;
			}

			if (!strncmp(c, org_w, strlen(org_w)))
			{
				char *s = c;
				concptr tmp = simplify_list[i][1];
				while (*tmp)
					*s++ = *tmp++;
				tmp = c + strlen(org_w);
				while (*tmp)
					*s++ = *tmp++;
				*s = '\0';
			}
		}
	}

	c = buf;
	len = 0;
	/* 半角 12 文字分で切る */
	while (*c)
	{
#ifdef JP
		if (iskanji(*c))
		{
			if (len + 2 > 12) break;
			c += 2;
			len += 2;
		}
		else
#endif
		{
			if (len + 1 > 12) break;
			c++;
			len++;
		}
	}
	*c = '\0';
	Term_putstr(0, y, 12, attr, buf);
}

/*
 * Display a "small-scale" map of the dungeon in the active Term
 */
void display_map(floor_type *floor_ptr, int *cy, int *cx)
{
	int i, j, x, y;

	TERM_COLOR ta;
	SYMBOL_CODE tc;

	byte tp;

	TERM_COLOR **bigma;
	SYMBOL_CODE **bigmc;
	byte **bigmp;

	TERM_COLOR **ma;
	SYMBOL_CODE **mc;
	byte **mp;

	/* Save lighting effects */
	bool old_view_special_lite = view_special_lite;
	bool old_view_granite_lite = view_granite_lite;

	TERM_LEN hgt, wid, yrat, xrat;

	int **match_autopick_yx;
	object_type ***object_autopick_yx;

	Term_get_size(&wid, &hgt);
	hgt -= 2;
	wid -= 14;
	if (use_bigtile) wid /= 2;

	yrat = (floor_ptr->height + hgt - 1) / hgt;
	xrat = (floor_ptr->width + wid - 1) / wid;

	/* Disable lighting effects */
	view_special_lite = FALSE;
	view_granite_lite = FALSE;

	/* Allocate the maps */
	C_MAKE(ma, (hgt + 2), TERM_COLOR *);
	C_MAKE(mc, (hgt + 2), char_ptr);
	C_MAKE(mp, (hgt + 2), byte_ptr);
	C_MAKE(match_autopick_yx, (hgt + 2), sint_ptr);
	C_MAKE(object_autopick_yx, (hgt + 2), object_type **);

	/* Allocate and wipe each line map */
	for (y = 0; y < (hgt + 2); y++)
	{
		/* Allocate one row each array */
		C_MAKE(ma[y], (wid + 2), TERM_COLOR);
		C_MAKE(mc[y], (wid + 2), char);
		C_MAKE(mp[y], (wid + 2), byte);
		C_MAKE(match_autopick_yx[y], (wid + 2), int);
		C_MAKE(object_autopick_yx[y], (wid + 2), object_type *);

		for (x = 0; x < wid + 2; ++x)
		{
			match_autopick_yx[y][x] = -1;
			object_autopick_yx[y][x] = NULL;

			/* Nothing here */
			ma[y][x] = TERM_WHITE;
			mc[y][x] = ' ';

			/* No priority */
			mp[y][x] = 0;
		}
	}

	/* Allocate the maps */
	C_MAKE(bigma, (floor_ptr->height + 2), TERM_COLOR *);
	C_MAKE(bigmc, (floor_ptr->height + 2), char_ptr);
	C_MAKE(bigmp, (floor_ptr->height + 2), byte_ptr);

	/* Allocate and wipe each line map */
	for (y = 0; y < (floor_ptr->height + 2); y++)
	{
		/* Allocate one row each array */
		C_MAKE(bigma[y], (floor_ptr->width + 2), TERM_COLOR);
		C_MAKE(bigmc[y], (floor_ptr->width + 2), char);
		C_MAKE(bigmp[y], (floor_ptr->width + 2), byte);

		for (x = 0; x < floor_ptr->width + 2; ++x)
		{
			/* Nothing here */
			bigma[y][x] = TERM_WHITE;
			bigmc[y][x] = ' ';

			/* No priority */
			bigmp[y][x] = 0;
		}
	}

	/* Fill in the map */
	for (i = 0; i < floor_ptr->width; ++i)
	{
		for (j = 0; j < floor_ptr->height; ++j)
		{
			x = i / xrat + 1;
			y = j / yrat + 1;

			match_autopick = -1;
			autopick_obj = NULL;
			feat_priority = -1;

			/* Extract the current attr/char at that map location */
			map_info(j, i, &ta, &tc, &ta, &tc);

			/* Extract the priority */
			tp = (byte_hack)feat_priority;

			if (match_autopick != -1
				&& (match_autopick_yx[y][x] == -1
					|| match_autopick_yx[y][x] > match_autopick))
			{
				match_autopick_yx[y][x] = match_autopick;
				object_autopick_yx[y][x] = autopick_obj;
				tp = 0x7f;
			}

			/* Save the char, attr and priority */
			bigmc[j + 1][i + 1] = tc;
			bigma[j + 1][i + 1] = ta;
			bigmp[j + 1][i + 1] = tp;
		}
	}

	for (j = 0; j < floor_ptr->height; ++j)
	{
		for (i = 0; i < floor_ptr->width; ++i)
		{
			x = i / xrat + 1;
			y = j / yrat + 1;

			tc = bigmc[j + 1][i + 1];
			ta = bigma[j + 1][i + 1];
			tp = bigmp[j + 1][i + 1];

			/* rare feature has more priority */
			if (mp[y][x] == tp)
			{
				int t;
				int cnt = 0;

				for (t = 0; t < 8; t++)
				{
					if (tc == bigmc[j + 1 + ddy_cdd[t]][i + 1 + ddx_cdd[t]] &&
						ta == bigma[j + 1 + ddy_cdd[t]][i + 1 + ddx_cdd[t]])
						cnt++;
				}
				if (cnt <= 4)
					tp++;
			}

			/* Save "best" */
			if (mp[y][x] < tp)
			{
				/* Save the char, attr and priority */
				mc[y][x] = tc;
				ma[y][x] = ta;
				mp[y][x] = tp;
			}
		}
	}


	/* Corners */
	x = wid + 1;
	y = hgt + 1;

	/* Draw the corners */
	mc[0][0] = mc[0][x] = mc[y][0] = mc[y][x] = '+';

	/* Draw the horizontal edges */
	for (x = 1; x <= wid; x++) mc[0][x] = mc[y][x] = '-';

	/* Draw the vertical edges */
	for (y = 1; y <= hgt; y++) mc[y][0] = mc[y][x] = '|';


	/* Display each map line in order */
	for (y = 0; y < hgt + 2; ++y)
	{
		/* Start a new line */
		Term_gotoxy(COL_MAP, y);

		/* Display the line */
		for (x = 0; x < wid + 2; ++x)
		{
			ta = ma[y][x];
			tc = mc[y][x];

			/* Hack -- fake monochrome */
			if (!use_graphics)
			{
				if (current_world_ptr->timewalk_m_idx) ta = TERM_DARK;
				else if (IS_INVULN() || p_ptr->timewalk) ta = TERM_WHITE;
				else if (p_ptr->wraith_form) ta = TERM_L_DARK;
			}

			/* Add the character */
			Term_add_bigch(ta, tc);
		}
	}


	for (y = 1; y < hgt + 1; ++y)
	{
		match_autopick = -1;
		for (x = 1; x <= wid; x++) {
			if (match_autopick_yx[y][x] != -1 &&
				(match_autopick > match_autopick_yx[y][x] ||
					match_autopick == -1)) {
				match_autopick = match_autopick_yx[y][x];
				autopick_obj = object_autopick_yx[y][x];
			}
		}

		/* Clear old display */
		Term_putstr(0, y, 12, 0, "            ");

		if (match_autopick != -1)
#if 1
			display_shortened_item_name(autopick_obj, y);
#else
		{
			char buf[13] = "\0";
			strncpy(buf, autopick_list[match_autopick].name, 12);
			buf[12] = '\0';
			put_str(buf, y, 0);
		}
#endif

	}

	/* Player location */
	(*cy) = p_ptr->y / yrat + 1 + ROW_MAP;
	if (!use_bigtile)
		(*cx) = p_ptr->x / xrat + 1 + COL_MAP;
	else
		(*cx) = (p_ptr->x / xrat + 1) * 2 + COL_MAP;

	/* Restore lighting effects */
	view_special_lite = old_view_special_lite;
	view_granite_lite = old_view_granite_lite;

	/* Free each line map */
	for (y = 0; y < (hgt + 2); y++)
	{
		/* Free one row each array */
		C_KILL(ma[y], (wid + 2), TERM_COLOR);
		C_KILL(mc[y], (wid + 2), SYMBOL_CODE);
		C_KILL(mp[y], (wid + 2), byte);
		C_KILL(match_autopick_yx[y], (wid + 2), int);
		C_KILL(object_autopick_yx[y], (wid + 2), object_type *);
	}

	/* Free each line map */
	C_KILL(ma, (hgt + 2), TERM_COLOR *);
	C_KILL(mc, (hgt + 2), char_ptr);
	C_KILL(mp, (hgt + 2), byte_ptr);
	C_KILL(match_autopick_yx, (hgt + 2), sint_ptr);
	C_KILL(object_autopick_yx, (hgt + 2), object_type **);

	/* Free each line map */
	for (y = 0; y < (floor_ptr->height + 2); y++)
	{
		/* Free one row each array */
		C_KILL(bigma[y], (floor_ptr->width + 2), TERM_COLOR);
		C_KILL(bigmc[y], (floor_ptr->width + 2), SYMBOL_CODE);
		C_KILL(bigmp[y], (floor_ptr->width + 2), byte);
	}

	/* Free each line map */
	C_KILL(bigma, (floor_ptr->height + 2), TERM_COLOR *);
	C_KILL(bigmc, (floor_ptr->height + 2), char_ptr);
	C_KILL(bigmp, (floor_ptr->height + 2), byte_ptr);
}


/*
 * Display a "small-scale" map of the dungeon for the player
 *
 * Currently, the "player" is displayed on the map.
 */
void do_cmd_view_map(void)
{
	int cy, cx;

	screen_save();

	prt(_("お待ち下さい...", "Please wait..."), 0, 0);

	Term_fresh();
	Term_clear();

	display_autopick = 0;

	/* Display the map */
	display_map(p_ptr->current_floor_ptr, &cy, &cx);

	/* Wait for it */
	if (max_autopick && !p_ptr->wild_mode)
	{
		display_autopick = ITEM_DISPLAY;

		while (1)
		{
			int i;
			byte flag;

			int wid, hgt, row_message;

			Term_get_size(&wid, &hgt);
			row_message = hgt - 1;

			put_str(_("何かキーを押してください('M':拾う 'N':放置 'D':M+N 'K':壊すアイテムを表示)",
				" Hit M, N(for ~), K(for !), or D(same as M+N) to display auto-picker items."), row_message, 1);

			/* Hilite the player */
			move_cursor(cy, cx);

			i = inkey();

			if ('M' == i)
				flag = (DO_AUTOPICK | DO_QUERY_AUTOPICK);
			else if ('N' == i)
				flag = DONT_AUTOPICK;
			else if ('K' == i)
				flag = DO_AUTODESTROY;
			else if ('D' == i)
				flag = (DO_AUTOPICK | DO_QUERY_AUTOPICK | DONT_AUTOPICK);
			else
				break;

			Term_fresh();

			if (~display_autopick & flag)
				display_autopick |= flag;
			else
				display_autopick &= ~flag;
			/* Display the map */
			display_map(p_ptr->current_floor_ptr, &cy, &cx);
		}

		display_autopick = 0;

	}
	else
	{
		put_str(_("何かキーを押すとゲームに戻ります", "Hit any key to continue"), 23, 30);
		/* Hilite the player */
		move_cursor(cy, cx);
		/* Get any key */
		inkey();
	}
	screen_load();
}

/*
 * Track a new monster
 */
void health_track(MONSTER_IDX m_idx)
{
	/* Mount monster is already tracked */
	if (m_idx && m_idx == p_ptr->riding) return;

	/* Track a new guy */
	p_ptr->health_who = m_idx;

	/* Redraw (later) */
	p_ptr->redraw |= (PR_HEALTH);
}


/*
 * Moves the cursor to a given MAP (y,x) location
 */
void move_cursor_relative(int row, int col)
{
	/* Real co-ords convert to screen positions */
	row -= panel_row_prt;

	/* Go there */
	Term_gotoxy(panel_col_of(col), row);
}


/*
 * print project path
 */
void prt_path(POSITION y, POSITION x)
{
	int i;
	int path_n;
	u16b path_g[512];
	byte_hack default_color = TERM_SLATE;

	if (!display_path) return;
	if (-1 == project_length)
		return;

	/* Get projection path */
	path_n = project_path(path_g, (project_length ? project_length : MAX_RANGE), p_ptr->y, p_ptr->x, y, x, PROJECT_PATH | PROJECT_THRU);

	p_ptr->redraw |= (PR_MAP);
	handle_stuff();

	/* Draw path */
	for (i = 0; i < path_n; i++)
	{
		POSITION ny = GRID_Y(path_g[i]);
		POSITION nx = GRID_X(path_g[i]);
		grid_type *g_ptr = &p_ptr->current_floor_ptr->grid_array[ny][nx];

		if (panel_contains(ny, nx))
		{
			TERM_COLOR a = default_color;
			char c;

			TERM_COLOR ta = default_color;
			char tc = '*';

			if (g_ptr->m_idx && p_ptr->current_floor_ptr->m_list[g_ptr->m_idx].ml)
			{
				/* Determine what is there */
				map_info(ny, nx, &a, &c, &ta, &tc);

				if (!IS_ASCII_GRAPHICS(a))
					a = default_color;
				else if (c == '.' && (a == TERM_WHITE || a == TERM_L_WHITE))
					a = default_color;
				else if (a == default_color)
					a = TERM_WHITE;
			}

			if (!use_graphics)
			{
				if (current_world_ptr->timewalk_m_idx) a = TERM_DARK;
				else if (IS_INVULN() || p_ptr->timewalk) a = TERM_WHITE;
				else if (p_ptr->wraith_form) a = TERM_L_DARK;
			}

			c = '*';

			/* Hack -- Queue it */
			Term_queue_bigchar(panel_col_of(nx), ny - panel_row_prt, a, c, ta, tc);
		}

		/* Known Wall */
		if ((g_ptr->info & CAVE_MARK) && !cave_have_flag_grid(g_ptr, FF_PROJECT)) break;

		/* Change color */
		if (nx == x && ny == y) default_color = TERM_L_DARK;
	}
}


/*
 * Hack -- track the given monster race
 */
void monster_race_track(MONRACE_IDX r_idx)
{
	/* Save this monster ID */
	p_ptr->monster_race_idx = r_idx;

	p_ptr->window |= (PW_MONSTER);
}

/*
 * Hack -- track the given object kind
 */
void object_kind_track(KIND_OBJECT_IDX k_idx)
{
	/* Save this monster ID */
	p_ptr->object_kind_idx = k_idx;

	p_ptr->window |= (PW_OBJECT);
}


/*!
 * @brief 実ゲームプレイ時間を更新する
 */
void update_playtime(void)
{
	/* Check if the game has started */
	if (current_world_ptr->start_time != 0)
	{
		u32b tmp = (u32b)time(NULL);
		current_world_ptr->play_time += (tmp - current_world_ptr->start_time);
		current_world_ptr->start_time = tmp;
	}
}

