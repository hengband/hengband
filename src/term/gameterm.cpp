#include "term/gameterm.h"
#include "system/system-variables.h"
#include "term/term-color-types.h"
#include "util/quarks.h"
#include "util/string-processor.h"

 /*
 * Convert an "attr"/"char" pair into a "pict" (P)
 */
#define PICT(A, C) ((((uint16_t)(A)) << 8) | ((byte)(C)))

/*
 * Standard window names
 */
const char angband_term_name[8][16] =
{
	"Hengband",
	"Term-1",
	"Term-2",
	"Term-3",
	"Term-4",
	"Term-5",
	"Term-6",
	"Term-7"
};

/*
 * Global table of color definitions
 */
byte angband_color_table[256][4] =
{
	{0x00, 0x00, 0x00, 0x00},	/* TERM_DARK */
	{0x00, 0xFF, 0xFF, 0xFF},	/* TERM_WHITE */
	{0x00, 0x80, 0x80, 0x80},	/* TERM_SLATE */
	{0x00, 0xFF, 0x80, 0x00},	/* TERM_ORANGE */
	{0x00, 0xC0, 0x00, 0x00},	/* TERM_RED */
	{0x00, 0x00, 0x80, 0x40},	/* TERM_GREEN */
	{0x00, 0x00, 0x00, 0xFF},	/* TERM_BLUE */
	{0x00, 0x80, 0x40, 0x00},	/* TERM_UMBER */
	{0x00, 0x40, 0x40, 0x40},	/* TERM_L_DARK */
	{0x00, 0xC0, 0xC0, 0xC0},	/* TERM_L_WHITE */
	{0x00, 0xFF, 0x00, 0xFF},	/* TERM_VIOLET */
	{0x00, 0xFF, 0xFF, 0x00},	/* TERM_YELLOW */
	{0x00, 0xFF, 0x00, 0x00},	/* TERM_L_RED */
	{0x00, 0x00, 0xFF, 0x00},	/* TERM_L_GREEN */
	{0x00, 0x00, 0xFF, 0xFF},	/* TERM_L_BLUE */
	{0x00, 0xC0, 0x80, 0x40}	/* TERM_L_UMBER */
};

/*!
 * @brief 色名称テーブル / Hack -- the "basic" color names (see "TERM_xxx")
 */
const concptr color_names[16] =
{
#ifdef JP
	"黒",
	"白",
	"青灰色",
	"オレンジ",
	"赤",
	"緑",
	"青",
	"琥珀色",
	"灰色",
	"明青灰色",
	"紫",
	"黄",
	"明るい赤",
	"明るい緑",
	"明るい青",
	"明琥珀色",
#else
	"Dark",
	"White",
	"Slate",
	"Orange",
	"Red",
	"Green",
	"Blue",
	"Umber",
	"Light Dark",
	"Light Slate",
	"Violet",
	"Yellow",
	"Light Red",
	"Light Green",
	"Light Blue",
	"Light Umber",
#endif

};


/*!
 * @brief サブウィンドウ名称テーブル
 * @details
 * <pre>
 * Certain "screens" always use the main screen, including News, Birth,
 * Dungeon, Tomb-stone, High-scores, Macros, Colors, Visuals, Options.
 *
 * Later, special flags may allow sub-windows to "steal" stuff from the
 * main window, including File dump (help), File dump (artifacts, uniques),
 * Character screen, Small scale map, Previous Messages, Store screen, etc.
 *
 * The "ctrl-i" (tab) command flips the "Display inven/equip" and "Display
 * equip/inven" flags for all windows.
 *
 * The "ctrl-g" command (or pseudo-command) should perhaps grab a snapshot
 * of the main screen into any interested windows.
 * </pre>
 */
const concptr window_flag_desc[32] =
{
	_("持ち物/装備一覧", "Display inven/equip"),
	_("装備/持ち物一覧", "Display equip/inven"),
	_("呪文一覧", "Display spell list"),
	_("キャラクタ情報", "Display character"),
	_("視界内のモンスター表示", "Display monsters in sight"),
	NULL,
	_("メッセージ", "Display messages"),
	_("ダンジョン全体図", "Display overhead view"),
	_("モンスターの思い出", "Display monster recall"),
	_("アイテムの詳細", "Display object recall"),
	_("自分の周囲を表示", "Display dungeon view"),
	_("記念撮影", "Display snap-shot"),
	_("足元/床上のアイテム一覧", "Display items on floor"),
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};


/*!
 * @brief シンボル解説テーブル /
 * The table of "symbol info" -- each entry is a string of the form "X:desc" where "X" is the trigger, and "desc" is the "info".
 */
const concptr ident_info[] =
{
#ifdef JP
	" :暗闇",
	"!:薬, オイル",
	"\":アミュレット, 頸飾り",
	"#:壁(隠しドア)/植物/気体",
	"$:財宝(金か宝石)",
	"%:鉱脈(溶岩か石英)",
	"&:箱",
	"':開いたドア",
	"(:軟らかい防具",
	"):盾",
	"*:財宝を含んだ鉱脈または球形の怪物",
	"+:閉じたドア",
	",:食べ物, おばけキノコ",
	"-:魔法棒, ロッド",
	".:床",
	"/:竿状武器(アックス/パイク/等)",
	"0:博物館の入口",
	"1:雑貨屋の入口",
	"2:防具屋の入口",
	"3:武器専門店の入口",
	"4:寺院の入口",
	"5:錬金術の店の入口",
	"6:魔法の店の入口",
	"7:ブラックマーケットの入口",
	"8:我が家の入口",
	"9:書店の入口",
	"::岩石",
	";:守りのルーン/爆発のルーン",
	"<:上り階段",
	"=:指輪",
	">:下り階段",
	"?:巻物",
	"@:プレイヤー",
	"A:天使",
	"B:鳥",
	"C:犬",
	"D:古代ドラゴン/ワイアーム",
	"E:エレメンタル",
	"F:トンボ",
	"G:ゴースト",
	"H:雑種",
	"I:昆虫",
	"J:ヘビ",
	"K:キラー・ビートル",
	"L:リッチ",
	"M:多首の爬虫類",
	"N:謎の生物",
	"O:オーガ",
	"P:巨大人間型生物",
	"Q:クイルスルグ(脈打つ肉塊)",
	"R:爬虫類/両生類",
	"S:蜘蛛/サソリ/ダニ",
	"T:トロル",
	"U:上級デーモン",
	"V:バンパイア",
	"W:ワイト/レイス/等",
	"X:ゾーン/ザレン/等",
	"Y:イエティ",
	"Z:ハウンド",
	"[:堅いアーマー",
	"\\:鈍器(メイス/ムチ/等)",
	"]:種々の防具",
	"^:トラップ",
	"_:杖",
	"`:人形，彫像",
	"a:アリ",
	"b:コウモリ",
	"c:ムカデ",
	"d:ドラゴン",
	"e:目玉",
	"f:ネコ",
	"g:ゴーレム",
	"h:ホビット/エルフ/ドワーフ",
	"i:ベトベト",
	"j:ゼリー",
	"k:コボルド",
	"l:水棲生物",
	"m:モルド",
	"n:ナーガ",
	"o:オーク",
	"p:人間",
	"q:四足獣",
	"r:ネズミ",
	"s:スケルトン",
	"t:町の人",
	"u:下級デーモン",
	"v:ボルテックス",
	"w:イモムシ/大群",
	/* "x:unused", */
	"y:イーク",
	"z:ゾンビ/ミイラ",
	"{:飛び道具の弾(矢/弾)",
	"|:刀剣類(ソード/ダガー/等)",
	"}:飛び道具(弓/クロスボウ/スリング)",
	"~:水/溶岩流(種々のアイテム)",
#else
	" :A dark grid",
	"!:A potion (or oil)",
	"\":An amulet (or necklace)",
	"#:A wall (or secret door) / a plant / a gas",
	"$:Treasure (gold or gems)",
	"%:A vein (magma or quartz)",
	"&:A chest",
	"':An open door",
	"(:Soft armor",
	"):A shield",
	"*:A vein with treasure or a ball monster",
	"+:A closed door",
	",:Food (or mushroom patch)",
	"-:A wand (or rod)",
	".:Floor",
	"/:A polearm (Axe/Pike/etc)",
	"0:Entrance to the Museum",
	"1:Entrance to the General Store",
	"2:Entrance to the Armoury",
	"3:Entrance to the Weaponsmith's Shop",
	"4:Entrance to the Temple",
	"5:Entrance to the Alchemy Shop",
	"6:Entrance to the Magic Shop",
	"7:Entrance to the Black Market",
	"8:Entrance to your home",
	"9:Entrance to the Bookstore",
	"::Rubble",
	";:A rune of protection / an explosive rune",
	"<:An up staircase",
	"=:A ring",
	">:A down staircase",
	"?:A scroll",
	"@:You",
	"A:Angel",
	"B:Bird",
	"C:Canine",
	"D:Ancient Dragon/Wyrm",
	"E:Elemental",
	"F:Dragon Fly",
	"G:Ghost",
	"H:Hybrid",
	"I:Insect",
	"J:Snake",
	"K:Killer Beetle",
	"L:Lich",
	"M:Multi-Headed Reptile",
	"N:Mystery Living",
	"O:Ogre",
	"P:Giant Humanoid",
	"Q:Quylthulg (Pulsing Flesh Mound)",
	"R:Reptile/Amphibian",
	"S:Spider/Scorpion/Tick",
	"T:Troll",
	"U:Major Demon",
	"V:Vampire",
	"W:Wight/Wraith/etc",
	"X:Xorn/Xaren/etc",
	"Y:Yeti",
	"Z:Zephyr Hound",
	"[:Hard armor",
	"\\:A hafted weapon (mace/whip/etc)",
	"]:Misc. armor",
	"^:A trap",
	"_:A staff",
	"`:A figurine or statue",
	"a:Ant",
	"b:Bat",
	"c:Centipede",
	"d:Dragon",
	"e:Floating Eye",
	"f:Feline",
	"g:Golem",
	"h:Hobbit/Elf/Dwarf",
	"i:Icky Thing",
	"j:Jelly",
	"k:Kobold",
	"l:Aquatic monster",
	"m:Mold",
	"n:Naga",
	"o:Orc",
	"p:Person/Human",
	"q:Quadruped",
	"r:Rodent",
	"s:Skeleton",
	"t:Townsperson",
	"u:Minor Demon",
	"v:Vortex",
	"w:Worm/Worm-Mass",
	/* "x:unused", */
	"y:Yeek",
	"z:Zombie/Mummy",
	"{:A missile (arrow/bolt/shot)",
	"|:An edged weapon (sword/dagger/etc)",
	"}:A launcher (bow/crossbow/sling)",
	"~:Fluid terrain (or miscellaneous item)",
#endif

	NULL
};

/*
 * The array of window pointers
 */
term_type *angband_term[8];

/*!
 * スクリーン表示色キャラクタ /
 * Encode the screen colors
 */
static const concptr color_char = "dwsorgbuDWvyRGBU";

/*
 * Specify attr/char pairs for visual special effects
 * Be sure to use "index & 0x7F" to avoid illegal access
 */
TERM_COLOR misc_to_attr[256];
SYMBOL_CODE misc_to_char[256];

/*
 * Specify attr/char pairs for inventory items (by tval)
 * Be sure to use "index & 0x7F" to avoid illegal access
 */
TERM_COLOR tval_to_attr[128];

/*
 * Default spell color table (quark index)
 */
TERM_COLOR gf_color[MAX_GF];

/*!
 * @brief 万色表現用にランダムな色を選択する関数 /
 * Get a legal "multi-hued" color for drawing "spells"
 * @param max 色IDの最大値
 * @return 選択した色ID
 */
static TERM_COLOR mh_attr(int max)
{
	switch (randint1(max))
	{
	case  1: return (TERM_RED);
	case  2: return (TERM_GREEN);
	case  3: return (TERM_BLUE);
	case  4: return (TERM_YELLOW);
	case  5: return (TERM_ORANGE);
	case  6: return (TERM_VIOLET);
	case  7: return (TERM_L_RED);
	case  8: return (TERM_L_GREEN);
	case  9: return (TERM_L_BLUE);
	case 10: return (TERM_UMBER);
	case 11: return (TERM_L_UMBER);
	case 12: return (TERM_SLATE);
	case 13: return (TERM_WHITE);
	case 14: return (TERM_L_WHITE);
	case 15: return (TERM_L_DARK);
	}

	return (TERM_WHITE);
}


/*!
 * @brief 魔法属性に応じたエフェクトの色を返す /
 * Return a color to use for the bolt/ball spells
 * @param type 魔法属性
 * @return 対応する色ID
 */
static TERM_COLOR spell_color(EFFECT_ID type)
{
	/* Check if A.B.'s new graphics should be used (rr9) */
	if (streq(ANGBAND_GRAF, "new") || streq(ANGBAND_GRAF, "ne2"))
	{
		/* Analyze */
		switch (type)
		{
		case GF_PSY_SPEAR:		return (0x06);
		case GF_MISSILE:		return (0x0F);
		case GF_ACID:			return (0x04);
		case GF_ELEC:			return (0x02);
		case GF_FIRE:			return (0x00);
		case GF_COLD:			return (0x01);
		case GF_POIS:			return (0x03);
		case GF_HOLY_FIRE:		return (0x00);
		case GF_HELL_FIRE:		return (0x00);
		case GF_MANA:			return (0x0E);
			/* by henkma */
		case GF_SEEKER:			return (0x0E);
		case GF_SUPER_RAY:		return (0x0E);

		case GF_ARROW:			return (0x0F);
		case GF_WATER:			return (0x04);
		case GF_NETHER:			return (0x07);
		case GF_CHAOS:			return (mh_attr(15));
		case GF_DISENCHANT:		return (0x05);
		case GF_NEXUS:			return (0x0C);
		case GF_CONFUSION:		return (mh_attr(4));
		case GF_SOUND:			return (0x09);
		case GF_SHARDS:			return (0x08);
		case GF_FORCE:			return (0x09);
		case GF_INERTIAL:		return (0x09);
		case GF_GRAVITY:		return (0x09);
		case GF_TIME:			return (0x09);
		case GF_LITE_WEAK:		return (0x06);
		case GF_LITE:			return (0x06);
		case GF_DARK_WEAK:		return (0x07);
		case GF_DARK:			return (0x07);
		case GF_PLASMA:			return (0x0B);
		case GF_METEOR:			return (0x00);
		case GF_ICE:			return (0x01);
		case GF_ROCKET:			return (0x0F);
		case GF_DEATH_RAY:		return (0x07);
		case GF_NUKE:			return (mh_attr(2));
		case GF_DISINTEGRATE:	return (0x05);
		case GF_PSI:
		case GF_PSI_DRAIN:
		case GF_TELEKINESIS:
		case GF_DOMINATION:
		case GF_DRAIN_MANA:
		case GF_MIND_BLAST:
		case GF_BRAIN_SMASH:
			return (0x09);
		case GF_CAUSE_1:
		case GF_CAUSE_2:
		case GF_CAUSE_3:
		case GF_CAUSE_4:		return (0x0E);
		case GF_HAND_DOOM:		return (0x07);
		case GF_CAPTURE:		return (0x0E);
		case GF_IDENTIFY:		return (0x01);
		case GF_ATTACK:			return (0x0F);
		case GF_PHOTO:		return (0x06);
		}
	}
	/* Normal tiles or ASCII */
	else
	{
		TERM_COLOR a;
		SYMBOL_CODE c;

		/* Lookup the default colors for this type */
		concptr s = quark_str(gf_color[type]);

		if (!s) return (TERM_WHITE);

		/* Pick a random color */
		c = s[randint0(strlen(s))];

		/* Lookup this color */
		a = angband_strchr(color_char, c) - color_char;

		/* Invalid color (note check for < 0 removed, gave a silly
		 * warning because bytes are always >= 0 -- RG) */
		if (a > 15) return (TERM_WHITE);

		/* Use this color */
		return (a);
	}

	/* Standard "color" */
	return (TERM_WHITE);
}


/*!
 * @brief 始点から終点にかけた方向毎にボルトのキャラクタを返す /
 * Find the attr/char pair to use for a spell effect
 * @param y 始点Y座標
 * @param x 始点X座標
 * @param ny 終点Y座標
 * @param nx 終点X座標
 * @param typ 魔法の効果属性
 * @return 方向キャラID
 * @details
 * <pre>
 * It is moving (or has moved) from (x,y) to (nx,ny).
 * If the distance is not "one", we (may) return "*".
 * </pre>
 */
uint16_t bolt_pict(POSITION y, POSITION x, POSITION ny, POSITION nx, EFFECT_ID typ)
{
	int base;

	byte k;

	TERM_COLOR a;
	SYMBOL_CODE c;

	/* No motion (*) */
	if ((ny == y) && (nx == x)) base = 0x30;

	/* Vertical (|) */
	else if (nx == x) base = 0x40;

	/* Horizontal (-) */
	else if (ny == y) base = 0x50;

	/* Diagonal (/) */
	else if ((ny - y) == (x - nx)) base = 0x60;

	/* Diagonal (\) */
	else if ((ny - y) == (nx - x)) base = 0x70;

	/* Weird (*) */
	else base = 0x30;

	/* Basic spell color */
	k = spell_color(typ);

	/* Obtain attr/char */
	a = misc_to_attr[base + k];
	c = misc_to_char[base + k];

	/* Create pict */
	return (PICT(a, c));
}




/*!
 * @brief シンボル1文字をカラーIDに変更する /
 * Convert a "color letter" into an "actual" color
 * The colors are: dwsorgbuDWvyRGBU, as shown below
 * @param c シンボル文字
 * @return カラーID
 */
TERM_COLOR color_char_to_attr(SYMBOL_CODE c)
{
	switch (c)
	{
	case 'd': return (TERM_DARK);
	case 'w': return (TERM_WHITE);
	case 's': return (TERM_SLATE);
	case 'o': return (TERM_ORANGE);
	case 'r': return (TERM_RED);
	case 'g': return (TERM_GREEN);
	case 'b': return (TERM_BLUE);
	case 'u': return (TERM_UMBER);

	case 'D': return (TERM_L_DARK);
	case 'W': return (TERM_L_WHITE);
	case 'v': return (TERM_VIOLET);
	case 'y': return (TERM_YELLOW);
	case 'R': return (TERM_L_RED);
	case 'G': return (TERM_L_GREEN);
	case 'B': return (TERM_L_BLUE);
	case 'U': return (TERM_L_UMBER);
	}

	return (255);
}


