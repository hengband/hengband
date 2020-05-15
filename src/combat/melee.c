/*!
 * @file melee1.c
 * @brief 打撃処理 / Melee process.
 * @date 2014/01/17
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen.\n
 * @details
 */

#include "system/angband.h"
#include "core/stuff-handler.h"
#include "util.h"
#include "main/sound-definitions-table.h"

#include "artifact.h"
#include "cmd-pet.h"
#include "monster/monsterrace-hook.h"
#include "melee.h"
#include "monster/monster.h"
#include "monster/monster-status.h"
#include "spell/monster-spell.h"
#include "avatar.h"
#include "realm/realm-hex.h"
#include "realm/realm-song.h"
#include "object/object-flavor.h"
#include "object/object-hook.h"
#include "grid.h"
#include "dungeon/dungeon.h"
#include "effect/effect-characteristics.h"
#include "dungeon/dungeon.h"
#include "spell/process-effect.h"
#include "spell/spells-type.h"
#include "files.h"
#include "player/player-move.h"
#include "player/player-effects.h"
#include "player/player-skill.h"
#include "player/player-damage.h"
#include "player/player-status.h"
#include "player/mimic-info-table.h"
#include "player/player-class.h"
#include "player/player-personality.h"
#include "view/display-main-window.h"
#include "world/world.h"
#include "spell/spells-floor.h"
#include "spell/spells2.h"
#include "spell/spells3.h"

#define BLOW_EFFECT_TYPE_NONE  0
#define BLOW_EFFECT_TYPE_FEAR  1
#define BLOW_EFFECT_TYPE_SLEEP 2
#define BLOW_EFFECT_TYPE_HEAL  3

 /*!
  * @brief モンスターの打撃効力テーブル /
  * The table of monsters' blow effects
  */
const mbe_info_type mbe_info[] =
{
	{  0, 0,             }, /* None      */
	{ 60, GF_MISSILE,    }, /* HURT      */
	{  5, GF_POIS,       }, /* POISON    */
	{ 20, GF_DISENCHANT, }, /* UN_BONUS  */
	{ 15, GF_MISSILE,    }, /* UN_POWER  */ /* ToDo: Apply the correct effects */
	{  5, GF_MISSILE,    }, /* EAT_GOLD  */
	{  5, GF_MISSILE,    }, /* EAT_ITEM  */
	{  5, GF_MISSILE,    }, /* EAT_FOOD  */
	{  5, GF_MISSILE,    }, /* EAT_LITE  */
	{  0, GF_ACID,       }, /* ACID      */
	{ 10, GF_ELEC,       }, /* ELEC      */
	{ 10, GF_FIRE,       }, /* FIRE      */
	{ 10, GF_COLD,       }, /* COLD      */
	{  2, GF_MISSILE,    }, /* BLIND     */
	{ 10, GF_CONFUSION,  }, /* CONFUSE   */
	{ 10, GF_MISSILE,    }, /* TERRIFY   */
	{  2, GF_MISSILE,    }, /* PARALYZE  */
	{  0, GF_MISSILE,    }, /* LOSE_STR  */
	{  0, GF_MISSILE,    }, /* LOSE_INT  */
	{  0, GF_MISSILE,    }, /* LOSE_WIS  */
	{  0, GF_MISSILE,    }, /* LOSE_DEX  */
	{  0, GF_MISSILE,    }, /* LOSE_CON  */
	{  0, GF_MISSILE,    }, /* LOSE_CHR  */
	{  2, GF_MISSILE,    }, /* LOSE_ALL  */
	{ 60, GF_ROCKET,     }, /* SHATTER   */
	{  5, GF_MISSILE,    }, /* EXP_10    */
	{  5, GF_MISSILE,    }, /* EXP_20    */
	{  5, GF_MISSILE,    }, /* EXP_40    */
	{  5, GF_MISSILE,    }, /* EXP_80    */
	{  5, GF_POIS,       }, /* DISEASE   */
	{  5, GF_TIME,       }, /* TIME      */
	{  5, GF_MISSILE,    }, /* EXP_VAMP  */
	{  5, GF_MANA,       }, /* DR_MANA   */
	{ 60, GF_MISSILE,    }, /* SUPERHURT */
};

 /*!
  * @brief 幻覚時の打撃記述テーブル / Weird melee attack types when hallucinating
  */
#ifdef JP
const concptr silly_attacks[MAX_SILLY_ATTACK] =
{
	"に小便をかけられた。",
	"があなたの回りを3回回ってワンと言った。",
	"にしばきまわされた。",
	"に靴をなめさせられた。",
	"にハナクソを飛ばされた。",
	"にジャン拳で攻撃された。",
	"があなたの頬を札束でしばいた。",
	"があなたの前でポージングをした。",
	"にアカンベーされた。",
	"に「神の国」発言の撤回を求められた。",
	"にメッ○ールを飲まされた。",
	"につっこみを入れられた。",
	"はあなたと踊った。",
	"に顔にらく書きをされた。",
	"に借金の返済をせまられた。",
	"にスカートをめくられた。",
	"はあなたの手相を占った。",
	"から役満を上がられた。",
	"から愛の告白をされた。",
	"はあなたを時給500円で雇った。",
	"はあなたの100の秘密について熱く語った。",
	"がニャーと鳴いた。",
	"はあなたに気をつけた。",
	"はあなたをポリゴン化させた。",
	"に少しかじられた。",
	"はアルテマの呪文を唱えた！",
	"はあなたのスパイクをブロックした。",
	"はスライド移動した。",
	"は昇龍拳コマンドの入力に失敗した。",
	"は拡散波動砲を発射した。",
	"はデスラー戦法をしかけた。",
	"にライダーキックで攻撃された。",
	"に二週間以内でビデオを人に見せないと死ぬ呪いをかけられた。",
	"はパルプンテを唱えた。",
	"はスーパーウルトラギャラクティカマグナムを放った。",
	"にしゃがみ小キックでハメられた。",
	"にジェットストリームアタックをかけられた。",
	"はあなたに卍固めをかけて「1、2、3、ダーッ！」と叫んだ。",
	"は「いくじなし！ばかばかばか！」といって駆け出した。",
	"が「ごらん、ルーベンスの絵だよ」と言って静かに目を閉じた。",
	"は言った。「変愚蛮怒、絶賛公開中！」",
};

/*!
 * @brief 幻覚時の打撃記述テーブル(フォーマットつき) / Weird melee attack types when hallucinating (%s for strfmt())
 */
const concptr silly_attacks2[MAX_SILLY_ATTACK] =
{
	"%sに小便をかけた。",
	"%sの回りを3回回ってワンと言った。",
	"%sをしばきまわした。",
	"%sに靴をなめさせた。",
	"%sにハナクソを飛ばした。",
	"%sをジャン拳で攻撃した。",
	"%sの頬を札束でしばいた。",
	"%sの前でポージングをした。",
	"%sにアカンベーした。",
	"%sに「神の国」発言の撤回を求めた。",
	"%sにメッ○ールを飲ませた。",
	"%sにつっこみを入れた。",
	"%sと踊った。",
	"%sの顔にらく書きをした。",
	"%sに借金の返済をせまった。",
	"%sのスカートをめくった。",
	"%sの手相を占った。",
	"%sから役満を上がった。",
	"%sに愛の告白をした。",
	"%sを時給500円で雇った。",
	"%sの100の秘密について熱く語った。",
	"ニャーと鳴いた。",
	"%sに気をつけた。",
	"%sをポリゴン化させた。",
	"%sを少しかじった。",
	"アルテマの呪文を唱えた！",
	"%sのスパイクをブロックした。",
	"スライド移動した。",
	"昇龍拳コマンドの入力に失敗した。",
	"%sに拡散波動砲を発射した。",
	"%sにデスラー戦法をしかけた。",
	"%sをライダーキックで攻撃した。",
	"%sに二週間以内でビデオを人に見せないと死ぬ呪いをかけた。",
	"パルプンテを唱えた。",
	"%sにスーパーウルトラギャラクティカマグナムを放った。",
	"%sをしゃがみ小キックでハメた。",
	"%sにジェットストリームアタックをかけた。",
	"%sに卍固めをかけて「1、2、3、ダーッ！」と叫んだ。",
	"「いくじなし！ばかばかばか！」といって駆け出した。",
	"「ごらん、ルーベンスの絵だよ」と言って静かに目を閉じた。",
	"言った。「変愚蛮怒、絶賛公開中！」",
};
#else
const concptr silly_attacks[MAX_SILLY_ATTACK] =
{
	"smothers",
	"hugs",
	"humiliates",
	"whips",
	"kisses",

	"disgusts",
	"pees all over",
	"passes the gas on",
	"makes obscene gestures at",
	"licks",

	"stomps on",
	"swallows",
	"drools on",
	"misses",
	"shrinks",

	"emasculates",
	"evaporates",
	"solidifies",
	"digitizes",
	"insta-kills",

	"massacres",
	"slaughters",
	"drugs",
	"psychoanalyzes",
	"deconstructs",

	"falsifies",
	"disbelieves",
	"molests",
	"pusupusu",
};
#endif


/*!
 * @brief マーシャルアーツ打撃テーブル
 */
const martial_arts ma_blows[MAX_MA] =
{
#ifdef JP
	{ "%sを殴った。",                          1, 0, 1, 5, 0 },
	{ "%sを蹴った。",                           2, 0, 1, 7, 0 },
	{ "%sに正拳突きをくらわした。",                         3, 0, 1, 9, 0 },
	{ "%sに膝蹴りをくらわした。",             5, 5, 2, 4, MA_KNEE },
	{ "%sに肘打ちをくらわした。",            7, 5, 1, 12, 0 },
	{ "%sに体当りした。",                           9, 10, 2, 6, 0 },
	{ "%sを蹴った。",                           11, 10, 3, 6, MA_SLOW },
	{ "%sにアッパーをくらわした。",                       13, 12, 5, 5, 6 },
	{ "%sに二段蹴りをくらわした。",                    16, 15, 5, 6, 8 },
	{ "%sに猫爪撃をくらわした。",          20, 20, 5, 8, 0 },
	{ "%sに跳空脚をくらわした。",           24, 25, 6, 8, 10 },
	{ "%sに鷲爪襲をくらわした。",       28, 25, 7, 9, 0 },
	{ "%sに回し蹴りをくらわした。",         32, 30, 8, 10, 10 },
	{ "%sに鉄拳撃をくらわした。",          35, 35, 8, 11, 10 },
	{ "%sに飛空脚をくらわした。",         39, 35, 8, 12, 12 },
	{ "%sに昇龍拳をくらわした。",         43, 35, 9, 12, 16 },
	{ "%sに石破天驚拳をくらわした。",       48, 40, 10, 13, 18 },
#else
	{ "You punch %s.",                          1, 0, 1, 4, 0 },
	{ "You kick %s.",                           2, 0, 1, 6, 0 },
	{ "You strike %s.",                         3, 0, 1, 7, 0 },
	{ "You hit %s with your knee.",             5, 5, 2, 3, MA_KNEE },
	{ "You hit %s with your elbow.",            7, 5, 1, 8, 0 },
	{ "You butt %s.",                           9, 10, 2, 5, 0 },
	{ "You kick %s.",                           11, 10, 3, 4, MA_SLOW },
	{ "You uppercut %s.",                       13, 12, 4, 4, 6 },
	{ "You double-kick %s.",                    16, 15, 5, 4, 8 },
	{ "You hit %s with a Cat's Claw.",          20, 20, 5, 5, 0 },
	{ "You hit %s with a jump kick.",           25, 25, 5, 6, 10 },
	{ "You hit %s with an Eagle's Claw.",       29, 25, 6, 6, 0 },
	{ "You hit %s with a circle kick.",         33, 30, 6, 8, 10 },
	{ "You hit %s with an Iron Fist.",          37, 35, 8, 8, 10 },
	{ "You hit %s with a flying kick.",         41, 35, 8, 10, 12 },
	{ "You hit %s with a Dragon Fist.",         45, 35, 10, 10, 16 },
	{ "You hit %s with a Crushing Blow.",       48, 35, 10, 12, 18 },
#endif

};

/*!
 * @brief 修行僧のターンダメージ算出テーブル
 */
const int monk_ave_damage[PY_MAX_LEVEL + 1][3] =
{
  {0, 0, 0},
  {249, 249, 249},
  {324, 324, 324},
  {382, 438, 382},
  {382, 439, 382},
  {390, 446, 390},
  {394, 473, 394},
  {425, 528, 425},
  {430, 535, 430},
  {505, 560, 435},
  {517, 575, 444},
  {566, 655, 474},
  {585, 713, 486},
  {653, 843, 527},
  {678, 890, 544},
  {703, 973, 558},
  {765, 1096, 596},
  {914, 1146, 614},
  {943, 1240, 629},
  {971, 1276, 643},
  {1018, 1350, 667},
  {1063, 1464, 688},
  {1099, 1515, 705},
  {1128, 1559, 721},
  {1153, 1640, 735},
  {1336, 1720, 757},
  {1387, 1789, 778},
  {1430, 1893, 794},
  {1610, 2199, 863},
  {1666, 2280, 885},
  {1713, 2401, 908},
  {1755, 2465, 925},
  {1909, 2730, 984},
  {2156, 2891, 1009},
  {2218, 2970, 1031},
  {2319, 3107, 1063},
  {2404, 3290, 1098},
  {2477, 3389, 1125},
  {2544, 3483, 1150},
  {2771, 3899, 1228},
  {2844, 3982, 1259},
  {3129, 4064, 1287},
  {3200, 4190, 1313},
  {3554, 4674, 1432},
  {3614, 4738, 1463},
  {3679, 4853, 1485},
  {3741, 4905, 1512},
  {3785, 4943, 1538},
  {4141, 5532, 1652},
  {4442, 5581, 1679},
  {4486, 5636, 1702},
};

/*!
 * 腕力による攻撃回数算定値テーブル
 * Stat Table (STR) -- help index into the "blow" table
 */
const byte adj_str_blow[] =
{
	3       /* 3 */,
	4       /* 4 */,
	5       /* 5 */,
	6       /* 6 */,
	7       /* 7 */,
	8       /* 8 */,
	9       /* 9 */,
	10      /* 10 */,
	11      /* 11 */,
	12      /* 12 */,
	13      /* 13 */,
	14      /* 14 */,
	15      /* 15 */,
	16      /* 16 */,
	17      /* 17 */,
	20 /* 18/00-18/09 */,
	30 /* 18/10-18/19 */,
	40 /* 18/20-18/29 */,
	50 /* 18/30-18/39 */,
	60 /* 18/40-18/49 */,
	70 /* 18/50-18/59 */,
	80 /* 18/60-18/69 */,
	90 /* 18/70-18/79 */,
	100 /* 18/80-18/89 */,
	110 /* 18/90-18/99 */,
	120 /* 18/100-18/109 */,
	130 /* 18/110-18/119 */,
	140 /* 18/120-18/129 */,
	150 /* 18/130-18/139 */,
	160 /* 18/140-18/149 */,
	170 /* 18/150-18/159 */,
	180 /* 18/160-18/169 */,
	190 /* 18/170-18/179 */,
	200 /* 18/180-18/189 */,
	210 /* 18/190-18/199 */,
	220 /* 18/200-18/209 */,
	230 /* 18/210-18/219 */,
	240 /* 18/220+ */
};


/*!
 * 器用さによる攻撃回数インデックステーブル
 * Stat Table (DEX) -- index into the "blow" table
 */
const byte adj_dex_blow[] =
{
	0       /* 3 */,
	0       /* 4 */,
	0       /* 5 */,
	0       /* 6 */,
	0       /* 7 */,
	0       /* 8 */,
	0       /* 9 */,
	1       /* 10 */,
	1       /* 11 */,
	1       /* 12 */,
	1       /* 13 */,
	1       /* 14 */,
	2       /* 15 */,
	2       /* 16 */,
	2       /* 17 */,
	2       /* 18/00-18/09 */,
	3       /* 18/10-18/19 */,
	3       /* 18/20-18/29 */,
	3       /* 18/30-18/39 */,
	4       /* 18/40-18/49 */,
	4       /* 18/50-18/59 */,
	5       /* 18/60-18/69 */,
	5       /* 18/70-18/79 */,
	6       /* 18/80-18/89 */,
	6       /* 18/90-18/99 */,
	7       /* 18/100-18/109 */,
	7       /* 18/110-18/119 */,
	8       /* 18/120-18/129 */,
	8       /* 18/130-18/139 */,
	9      /* 18/140-18/149 */,
	9      /* 18/150-18/159 */,
	10      /* 18/160-18/169 */,
	10      /* 18/170-18/179 */,
	11      /* 18/180-18/189 */,
	11      /* 18/190-18/199 */,
	12      /* 18/200-18/209 */,
	12      /* 18/210-18/219 */,
	13      /* 18/220+ */
};


/*!
 * @brief
 * 腕力、器用さに応じた攻撃回数テーブル /
 * This table is used to help calculate the number of blows the player can
 * make in a single round of attacks (one player turn) with a normal weapon.
 * @details
 * <pre>
 * This number ranges from a single blow/round for weak players to up to six
 * blows/round for powerful warriors.
 *
 * Note that certain artifacts and ego-items give "bonus" blows/round.
 *
 * First, from the player class, we extract some values:
 *
 * Warrior       num = 6; mul = 5; div = MAX(70, weapon_weight);
 * Berserker     num = 6; mul = 7; div = MAX(70, weapon_weight);
 * Mage          num = 3; mul = 2; div = MAX(100, weapon_weight);
 * Priest        num = 5; mul = 3; div = MAX(100, weapon_weight);
 * Mindcrafter   num = 5; mul = 3; div = MAX(100, weapon_weight);
 * Rogue         num = 5; mul = 3; div = MAX(40, weapon_weight);
 * Ranger        num = 5; mul = 4; div = MAX(70, weapon_weight);
 * Paladin       num = 5; mul = 4; div = MAX(70, weapon_weight);
 * Weaponsmith   num = 5; mul = 5; div = MAX(150, weapon_weight);
 * Warrior-Mage  num = 5; mul = 3; div = MAX(70, weapon_weight);
 * Chaos Warrior num = 5; mul = 4; div = MAX(70, weapon_weight);
 * Monk          num = 5; mul = 3; div = MAX(60, weapon_weight);
 * Tourist       num = 4; mul = 3; div = MAX(100, weapon_weight);
 * Imitator      num = 5; mul = 4; div = MAX(70, weapon_weight);
 * Beastmaster   num = 5; mul = 3; div = MAX(70, weapon_weight);
 * Cavalry(Ride) num = 5; mul = 4; div = MAX(70, weapon_weight);
 * Cavalry(Walk) num = 5; mul = 3; div = MAX(100, weapon_weight);
 * Sorcerer      num = 1; mul = 1; div = MAX(1, weapon_weight);
 * Archer        num = 4; mul = 2; div = MAX(70, weapon_weight);
 * Magic eater   num = 4; mul = 2; div = MAX(70, weapon_weight);
 * ForceTrainer  num = 4; mul = 2; div = MAX(60, weapon_weight);
 * Mirror Master num = 3; mul = 3; div = MAX(100, weapon_weight);
 * Ninja         num = 4; mul = 1; div = MAX(20, weapon_weight);
 *
 * To get "P", we look up the relevant "adj_str_blow[]" (see above),
 * multiply it by "mul", and then divide it by "div".
 * Increase P by 1 if you wield a weapon two-handed.
 * Decrease P by 1 if you are a Ninja.
 *
 * To get "D", we look up the relevant "adj_dex_blow[]" (see above),
 *
 * The player gets "blows_table[P][D]" blows/round, as shown below,
 * up to a maximum of "num" blows/round, plus any "bonus" blows/round.
 * </pre>
 */
const byte blows_table[12][12] =
{
	/* P/D */
	/*      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11+ */
	/*      3   10   15  /10  /40  /60  /80 /100 /120 /140 /160 /180  */
	/* 0 */{	1,   1,   1,   1,   1,   2,   2,   2,   2,   3,   3,   4 },
	/* 1 */{	1,   1,   1,   2,   2,   2,   3,   3,   3,   4,   4,   4 },
	/* 2 */{	1,   1,   2,   2,   3,   3,   4,   4,   4,   5,   5,   5 },
	/* 3 */{	1,   1,   2,   3,   3,   4,   4,   4,   5,   5,   5,   5 },
	/* 4 */{	1,   1,   2,   3,   3,   4,   4,   5,   5,   5,   5,   5 },
	/* 5 */{	1,   1,   2,   3,   4,   4,   4,   5,   5,   5,   5,   6 },
	/* 6 */{	1,   1,   2,   3,   4,   4,   4,   5,   5,   5,   5,   6 },
	/* 7 */{	1,   2,   2,   3,   4,   4,   4,   5,   5,   5,   5,   6 },
	/* 8 */{	1,   2,   3,   3,   4,   4,   4,   5,   5,   5,   6,   6 },
	/* 9 */{	1,   2,   3,   4,   4,   4,   5,   5,   5,   5,   6,   6 },
	/* 10*/{	2,   2,   3,   4,   4,   4,   5,   5,   5,   6,   6,   6 },
	/*11+*/{	2,   2,   3,   4,   4,   4,   5,   5,   6,   6,   6,   6 },
};

 /*!
 * @brief プレイヤーからモンスターへの打撃命中判定 /
 * Determine if the player "hits" a monster (normal combat).
 * @param chance 基本命中値
 * @param ac モンスターのAC
 * @param visible 目標を視界に捕らえているならばTRUEを指定
 * @return 命中と判定された場合TRUEを返す
 * @note Always miss 5%, always hit 5%, otherwise random.
 */
bool test_hit_norm(player_type *attacker_ptr, HIT_RELIABILITY chance, ARMOUR_CLASS ac, bool visible)
{
	if (!visible) chance = (chance + 1) / 2;
	return hit_chance(attacker_ptr, chance, ac) >= randint1(100);
}

/*!
 * @brief モンスターへの命中率の計算
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param to_h 命中値
 * @param ac 敵AC
 * @return 命中確率
 */
PERCENTAGE hit_chance(player_type *attacker_ptr, HIT_RELIABILITY reli, ARMOUR_CLASS ac)
{
	PERCENTAGE chance = 5, chance_left = 90;
	if(reli <= 0) return 5;
	if(attacker_ptr->pseikaku == SEIKAKU_NAMAKE) chance_left = (chance_left * 19 + 9) / 20;
	chance += (100 - ((ac * 75) / reli)) * chance_left / 100;
	if (chance < 5) chance = 5;
	return chance;
}


/*!
 * @brief プレイヤー攻撃の種族スレイング倍率計算
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param mult 算出前の基本倍率(/10倍)
 * @param flgs スレイフラグ配列
 * @param m_ptr 目標モンスターの構造体参照ポインタ
 * @return スレイング加味後の倍率(/10倍)
 */
static MULTIPLY mult_slaying(player_type *player_ptr, MULTIPLY mult, const BIT_FLAGS* flgs, monster_type* m_ptr)
{
	static const struct slay_table_t {
		int slay_flag;
		BIT_FLAGS affect_race_flag;
		MULTIPLY slay_mult;
		size_t flag_offset;
		size_t r_flag_offset;
	} slay_table[] = {
#define OFFSET(X) offsetof(monster_race, X)
		{TR_SLAY_ANIMAL, RF3_ANIMAL, 25, OFFSET(flags3), OFFSET(r_flags3)},
		{TR_KILL_ANIMAL, RF3_ANIMAL, 40, OFFSET(flags3), OFFSET(r_flags3)},
		{TR_SLAY_EVIL,   RF3_EVIL,   20, OFFSET(flags3), OFFSET(r_flags3)},
		{TR_KILL_EVIL,   RF3_EVIL,   35, OFFSET(flags3), OFFSET(r_flags3)},
		{TR_SLAY_GOOD,   RF3_GOOD,   20, OFFSET(flags3), OFFSET(r_flags3)},
		{TR_KILL_GOOD,   RF3_GOOD,   35, OFFSET(flags3), OFFSET(r_flags3)},
		{TR_SLAY_HUMAN,  RF2_HUMAN,  25, OFFSET(flags2), OFFSET(r_flags2)},
		{TR_KILL_HUMAN,  RF2_HUMAN,  40, OFFSET(flags2), OFFSET(r_flags2)},
		{TR_SLAY_UNDEAD, RF3_UNDEAD, 30, OFFSET(flags3), OFFSET(r_flags3)},
		{TR_KILL_UNDEAD, RF3_UNDEAD, 50, OFFSET(flags3), OFFSET(r_flags3)},
		{TR_SLAY_DEMON,  RF3_DEMON,  30, OFFSET(flags3), OFFSET(r_flags3)},
		{TR_KILL_DEMON,  RF3_DEMON,  50, OFFSET(flags3), OFFSET(r_flags3)},
		{TR_SLAY_ORC,    RF3_ORC,    30, OFFSET(flags3), OFFSET(r_flags3)},
		{TR_KILL_ORC,    RF3_ORC,    50, OFFSET(flags3), OFFSET(r_flags3)},
		{TR_SLAY_TROLL,  RF3_TROLL,  30, OFFSET(flags3), OFFSET(r_flags3)},
		{TR_KILL_TROLL,  RF3_TROLL,  50, OFFSET(flags3), OFFSET(r_flags3)},
		{TR_SLAY_GIANT,  RF3_GIANT,  30, OFFSET(flags3), OFFSET(r_flags3)},
		{TR_KILL_GIANT,  RF3_GIANT,  50, OFFSET(flags3), OFFSET(r_flags3)},
		{TR_SLAY_DRAGON, RF3_DRAGON, 30, OFFSET(flags3), OFFSET(r_flags3)},
		{TR_KILL_DRAGON, RF3_DRAGON, 50, OFFSET(flags3), OFFSET(r_flags3)},
#undef OFFSET
	};

	monster_race* r_ptr = &r_info[m_ptr->r_idx];
	for (size_t i = 0; i < sizeof(slay_table) / sizeof(slay_table[0]); ++i)
	{
		const struct slay_table_t* p = &slay_table[i];

		if (!have_flag(flgs, p->slay_flag) ||
			!(atoffset(BIT_FLAGS, r_ptr, p->flag_offset) & p->affect_race_flag))
			continue;

		if (is_original_ap_and_seen(player_ptr, m_ptr))
		{
			atoffset(BIT_FLAGS, r_ptr, p->r_flag_offset) |= p->affect_race_flag;
		}

		mult = MAX(mult, p->slay_mult);
	}

	return mult;
}


/*!
 * @brief プレイヤー攻撃の属性スレイング倍率計算
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param mult 算出前の基本倍率(/10倍)
 * @param flgs スレイフラグ配列
 * @param m_ptr 目標モンスターの構造体参照ポインタ
 * @return スレイング加味後の倍率(/10倍)
 */
static MULTIPLY mult_brand(player_type *player_ptr, MULTIPLY mult, const BIT_FLAGS* flgs, monster_type* m_ptr)
{
	static const struct brand_table_t {
		int brand_flag;
		BIT_FLAGS resist_mask;
		BIT_FLAGS hurt_flag;
	} brand_table[] = {
		{TR_BRAND_ACID, RFR_EFF_IM_ACID_MASK, 0U           },
		{TR_BRAND_ELEC, RFR_EFF_IM_ELEC_MASK, 0U           },
		{TR_BRAND_FIRE, RFR_EFF_IM_FIRE_MASK, RF3_HURT_FIRE},
		{TR_BRAND_COLD, RFR_EFF_IM_COLD_MASK, RF3_HURT_COLD},
		{TR_BRAND_POIS, RFR_EFF_IM_POIS_MASK, 0U           },
	};

	monster_race* r_ptr = &r_info[m_ptr->r_idx];
	for (size_t i = 0; i < sizeof(brand_table) / sizeof(brand_table[0]); ++i)
	{
		const struct brand_table_t* p = &brand_table[i];

		if (!have_flag(flgs, p->brand_flag)) continue;

		/* Notice immunity */
		if (r_ptr->flagsr & p->resist_mask)
		{
			if (is_original_ap_and_seen(player_ptr, m_ptr))
			{
				r_ptr->r_flagsr |= (r_ptr->flagsr & p->resist_mask);
			}

			continue;
		}

		/* Otherwise, take the damage */
		if (r_ptr->flags3 & p->hurt_flag)
		{
			if (is_original_ap_and_seen(player_ptr, m_ptr))
			{
				r_ptr->r_flags3 |= p->hurt_flag;
			}

			mult = MAX(mult, 50);
			continue;
		}

		mult = MAX(mult, 25);
	}

	return mult;
}


/*!
 * @brief 剣術のスレイ倍率計算を行う /
 * Calcurate magnification of hissatsu technics
 * @param mult 剣術のスレイ効果以前に算出している多要素の倍率(/10倍)
 * @param flgs 剣術に使用する武器のスレイフラグ配列
 * @param m_ptr 目標となるモンスターの構造体参照ポインタ
 * @param mode 剣術のスレイ型ID
 * @return スレイの倍率(/10倍)
 */
static MULTIPLY mult_hissatsu(player_type *attacker_ptr, MULTIPLY mult, BIT_FLAGS *flgs, monster_type *m_ptr, BIT_FLAGS mode)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Burning Strike (Fire) */
	if (mode == HISSATSU_FIRE)
	{
		/* Notice immunity */
		if (r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK)
		{
			if (is_original_ap_and_seen(attacker_ptr, m_ptr))
			{
				r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK);
			}
		}

		/* Otherwise, take the damage */
		else if (have_flag(flgs, TR_BRAND_FIRE))
		{
			if (r_ptr->flags3 & RF3_HURT_FIRE)
			{
				if (mult < 70) mult = 70;
				if (is_original_ap_and_seen(attacker_ptr, m_ptr))
				{
					r_ptr->r_flags3 |= RF3_HURT_FIRE;
				}
			}
			else if (mult < 35) mult = 35;
		}
		else
		{
			if (r_ptr->flags3 & RF3_HURT_FIRE)
			{
				if (mult < 50) mult = 50;
				if (is_original_ap_and_seen(attacker_ptr, m_ptr))
				{
					r_ptr->r_flags3 |= RF3_HURT_FIRE;
				}
			}
			else if (mult < 25) mult = 25;
		}
	}

	/* Serpent's Tongue (Poison) */
	if (mode == HISSATSU_POISON)
	{
		/* Notice immunity */
		if (r_ptr->flagsr & RFR_EFF_IM_POIS_MASK)
		{
			if (is_original_ap_and_seen(attacker_ptr, m_ptr))
			{
				r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_POIS_MASK);
			}
		}

		/* Otherwise, take the damage */
		else if (have_flag(flgs, TR_BRAND_POIS))
		{
			if (mult < 35) mult = 35;
		}
		else
		{
			if (mult < 25) mult = 25;
		}
	}

	/* Zammaken (Nonliving Evil) */
	if (mode == HISSATSU_ZANMA)
	{
		if (!monster_living(m_ptr->r_idx) && (r_ptr->flags3 & RF3_EVIL))
		{
			if (mult < 15) mult = 25;
			else if (mult < 50) mult = MIN(50, mult + 20);
		}
	}

	/* Rock Smash (Hurt Rock) */
	if (mode == HISSATSU_HAGAN)
	{
		if (r_ptr->flags3 & RF3_HURT_ROCK)
		{
			if (is_original_ap_and_seen(attacker_ptr, m_ptr))
			{
				r_ptr->r_flags3 |= RF3_HURT_ROCK;
			}
			if (mult == 10) mult = 40;
			else if (mult < 60) mult = 60;
		}
	}

	/* Midare-Setsugekka (Cold) */
	if (mode == HISSATSU_COLD)
	{
		/* Notice immunity */
		if (r_ptr->flagsr & RFR_EFF_IM_COLD_MASK)
		{
			if (is_original_ap_and_seen(attacker_ptr, m_ptr))
			{
				r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_COLD_MASK);
			}
		}
		/* Otherwise, take the damage */
		else if (have_flag(flgs, TR_BRAND_COLD))
		{
			if (r_ptr->flags3 & RF3_HURT_COLD)
			{
				if (mult < 70) mult = 70;
				if (is_original_ap_and_seen(attacker_ptr, m_ptr))
				{
					r_ptr->r_flags3 |= RF3_HURT_COLD;
				}
			}
			else if (mult < 35) mult = 35;
		}
		else
		{
			if (r_ptr->flags3 & RF3_HURT_COLD)
			{
				if (mult < 50) mult = 50;
				if (is_original_ap_and_seen(attacker_ptr, m_ptr))
				{
					r_ptr->r_flags3 |= RF3_HURT_COLD;
				}
			}
			else if (mult < 25) mult = 25;
		}
	}

	/* Lightning Eagle (Elec) */
	if (mode == HISSATSU_ELEC)
	{
		/* Notice immunity */
		if (r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK)
		{
			if (is_original_ap_and_seen(attacker_ptr, m_ptr))
			{
				r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK);
			}
		}

		/* Otherwise, take the damage */
		else if (have_flag(flgs, TR_BRAND_ELEC))
		{
			if (mult < 70) mult = 70;
		}
		else
		{
			if (mult < 50) mult = 50;
		}
	}

	/* Bloody Maelstrom */
	if ((mode == HISSATSU_SEKIRYUKA) && attacker_ptr->cut && monster_living(m_ptr->r_idx))
	{
		MULTIPLY tmp = MIN(100, MAX(10, attacker_ptr->cut / 10));
		if (mult < tmp) mult = tmp;
	}

	/* Keiun-Kininken */
	if (mode == HISSATSU_UNDEAD)
	{
		if (r_ptr->flags3 & RF3_UNDEAD)
		{
			if (is_original_ap_and_seen(attacker_ptr, m_ptr))
			{
				r_ptr->r_flags3 |= RF3_UNDEAD;
			}
			if (mult == 10) mult = 70;
			else if (mult < 140) mult = MIN(140, mult + 60);
		}
		if (mult == 10) mult = 40;
		else if (mult < 60) mult = MIN(60, mult + 30);
	}

	if (mult > 150) mult = 150;
	return mult;
}


/*!
 * @brief ダメージにスレイ要素を加える総合処理ルーチン /
 * Extract the "total damage" from a given object hitting a given monster.
 * @param o_ptr 使用武器オブジェクトの構造体参照ポインタ
 * @param tdam 現在算出途中のダメージ値
 * @param m_ptr 目標モンスターの構造体参照ポインタ
 * @param mode 剣術のID
 * @param thrown 投擲処理ならばTRUEを指定する
 * @return 総合的なスレイを加味したダメージ値
 * @note
 * Note that "flasks of oil" do NOT do fire damage, although they\n
 * certainly could be made to do so.  XXX XXX\n
 *\n
 * Note that most brands and slays are x3, except Slay Animal (x2),\n
 * Slay Evil (x2), and Kill dragon (x5).\n
 */
HIT_POINT tot_dam_aux(player_type *attacker_ptr, object_type *o_ptr, HIT_POINT tdam, monster_type *m_ptr, BIT_FLAGS mode, bool thrown)
{
	BIT_FLAGS flgs[TR_FLAG_SIZE];
	object_flags(o_ptr, flgs);
	torch_flags(o_ptr, flgs); /* torches has secret flags */

	if (!thrown)
	{
		if (attacker_ptr->special_attack & (ATTACK_ACID)) add_flag(flgs, TR_BRAND_ACID);
		if (attacker_ptr->special_attack & (ATTACK_COLD)) add_flag(flgs, TR_BRAND_COLD);
		if (attacker_ptr->special_attack & (ATTACK_ELEC)) add_flag(flgs, TR_BRAND_ELEC);
		if (attacker_ptr->special_attack & (ATTACK_FIRE)) add_flag(flgs, TR_BRAND_FIRE);
		if (attacker_ptr->special_attack & (ATTACK_POIS)) add_flag(flgs, TR_BRAND_POIS);
	}

	if (hex_spelling(attacker_ptr, HEX_RUNESWORD)) add_flag(flgs, TR_SLAY_GOOD);

	MULTIPLY mult = 10;
	switch (o_ptr->tval)
	{
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
	case TV_HAFTED:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_DIGGING:
	case TV_LITE:
	{
		mult = mult_slaying(attacker_ptr, mult, flgs, m_ptr);

		mult = mult_brand(attacker_ptr, mult, flgs, m_ptr);

		if (attacker_ptr->pclass == CLASS_SAMURAI)
		{
			mult = mult_hissatsu(attacker_ptr, mult, flgs, m_ptr, mode);
		}

		if ((attacker_ptr->pclass != CLASS_SAMURAI) && (have_flag(flgs, TR_FORCE_WEAPON)) && (attacker_ptr->csp > (o_ptr->dd * o_ptr->ds / 5)))
		{
			attacker_ptr->csp -= (1 + (o_ptr->dd * o_ptr->ds / 5));
			attacker_ptr->redraw |= (PR_MANA);
			mult = mult * 3 / 2 + 20;
		}

		if ((o_ptr->name1 == ART_NOTHUNG) && (m_ptr->r_idx == MON_FAFNER))
			mult = 150;
		break;
	}
	}

	if (mult > 150) mult = 150;
	return (tdam * mult / 10);
}

/*!
* @brief プレイヤーからモンスターへの打撃クリティカル判定 /
* Critical hits (by player) Factor in weapon weight, total plusses, player melee bonus
* @param weight 矢弾の重量
* @param plus 武器の命中修正
* @param dam 現在算出中のダメージ値
* @param meichuu 打撃の基本命中力
* @param mode オプションフラグ
* @return クリティカル修正が入ったダメージ値
*/
HIT_POINT critical_norm(player_type *attacker_ptr, WEIGHT weight, int plus, HIT_POINT dam, s16b meichuu, BIT_FLAGS mode)
{
	/* Extract "blow" power */
	int i = (weight + (meichuu * 3 + plus * 5) + attacker_ptr->skill_thn);

	/* Chance */
	bool is_special_option = randint1((attacker_ptr->pclass == CLASS_NINJA) ? 4444 : 5000) <= i;
	is_special_option |= mode == HISSATSU_MAJIN;
	is_special_option |= mode == HISSATSU_3DAN;
	if (!is_special_option) return dam;
	
	int k = weight + randint1(650);
	if ((mode == HISSATSU_MAJIN) || (mode == HISSATSU_3DAN)) k += randint1(650);

	if (k < 400)
	{
		msg_print(_("手ごたえがあった！", "It was a good hit!"));

		dam = 2 * dam + 5;
		return dam;
	}

	if (k < 700)
	{
		msg_print(_("かなりの手ごたえがあった！", "It was a great hit!"));
		dam = 2 * dam + 10;
		return dam;
	}
	
	if (k < 900)
	{
		msg_print(_("会心の一撃だ！", "It was a superb hit!"));
		dam = 3 * dam + 15;
		return dam;
	}
	
	if (k < 1300)
	{
		msg_print(_("最高の会心の一撃だ！", "It was a *GREAT* hit!"));
		dam = 3 * dam + 20;
		return dam;
	}

	msg_print(_("比類なき最高の会心の一撃だ！", "It was a *SUPERB* hit!"));
	dam = ((7 * dam) / 2) + 25;
	return dam;
}

/*!
 * @brief モンスター打撃のクリティカルランクを返す /
 * Critical blow. All hits that do 95% of total possible damage,
 * @param dice モンスター打撃のダイス数
 * @param sides モンスター打撃の最大ダイス目
 * @param dam プレイヤーに与えたダメージ
 * @details
 * and which also do at least 20 damage, or, sometimes, N damage.
 * This is used only to determine "cuts" and "stuns".
 */
static int monster_critical(DICE_NUMBER dice, DICE_SID sides, HIT_POINT dam)
{
	/* Must do at least 95% of perfect */
	int total = dice * sides;
	if (dam < total * 19 / 20) return 0;

	/* Weak blows rarely work */
	if ((dam < 20) && (randint0(100) >= dam)) return 0;

	/* Perfect damage */
	int max = 0;
	if ((dam >= total) && (dam >= 40)) max++;

	/* Super-charge */
	if (dam >= 20)
	{
		while (randint0(100) < 2) max++;
	}

	/* Critical damage */
	if (dam > 45) return (6 + max);
	if (dam > 33) return (5 + max);
	if (dam > 25) return (4 + max);
	if (dam > 18) return (3 + max);
	if (dam > 11) return (2 + max);
	return (1 + max);
}


/*!
 * @brief モンスター打撃の命中を判定する /
 * Determine if a monster attack against the player succeeds.
 * @param power 打撃属性毎の基本命中値
 * @param level モンスターのレベル
 * @param stun モンスターの朦朧値
 * @return TRUEならば命中判定
 * @details
 * Always miss 5% of the time, Always hit 5% of the time.
 * Otherwise, match monster power against player armor.
 */
static int check_hit(player_type *target_ptr, int power, DEPTH level, int stun)
{
	int k = randint0(100);
	if (stun && one_in_(2)) return FALSE;
	if (k < 10) return (k < 5);
	int i = (power + (level * 3));

	int ac = target_ptr->ac + target_ptr->to_a;
	if (target_ptr->special_attack & ATTACK_SUIKEN) ac += (target_ptr->lev * 2);

	if ((i > 0) && (randint1(i) > ((ac * 3) / 4))) return TRUE;
	return FALSE;
}

/*!
 * @brief モンスターから敵モンスターへの命中判定
 * @param power 打撃属性による基本命中値
 * @param level 攻撃側モンスターのレベル
 * @param ac 目標モンスターのAC
 * @param stun 攻撃側モンスターが朦朧状態ならTRUEを返す
 * @return 命中ならばTRUEを返す
 */
static int check_hit2(int power, DEPTH level, ARMOUR_CLASS ac, int stun)
{
	int k = randint0(100);
	if (stun && one_in_(2)) return FALSE;
	if (k < 10) return (k < 5);
	int i = (power + (level * 3));

	if ((i > 0) && (randint1(i) > ((ac * 3) / 4))) return TRUE;
	return FALSE;
}

/*! モンスターの侮辱行為メッセージテーブル / Hack -- possible "insult" messages */
static concptr desc_insult[] =
{
#ifdef JP
	"があなたを侮辱した！",
	"があなたの母を侮辱した！",
	"があなたを軽蔑した！",
	"があなたを辱めた！",
	"があなたを汚した！",
	"があなたの回りで踊った！",
	"が猥褻な身ぶりをした！",
	"があなたをぼんやりと見た！！！",
	"があなたをパラサイト呼ばわりした！",
	"があなたをサイボーグ扱いした！"
#else
	"insults you!",
	"insults your mother!",
	"gives you the finger!",
	"humiliates you!",
	"defiles you!",
	"dances around you!",
	"makes obscene gestures!",
	"moons you!!!"
	"calls you a parasite!",
	"calls you a cyborg!"
#endif

};

/*! マゴットのぼやきメッセージテーブル / Hack -- possible "insult" messages */
static concptr desc_moan[] =
{
#ifdef JP
	"は何かを悲しんでいるようだ。",
	"が彼の飼い犬を見なかったかと尋ねている。",
	"が縄張りから出て行けと言っている。",
	"はキノコがどうとか呟いている。"
#else
	"seems sad about something.",
	"asks if you have seen his dogs.",
	"tells you to get off his land.",
	"mumbles something about mushrooms."
#endif

};


/*!
* @brief 敵オーラによるプレイヤーのダメージ処理（補助）
* @param m_ptr オーラを持つモンスターの構造体参照ポインタ
* @param immune ダメージを回避できる免疫フラグ
* @param flags_offset オーラフラグ配列の参照オフセット
* @param r_flags_offset モンスターの耐性配列の参照オフセット
* @param aura_flag オーラフラグ配列
* @param dam_func ダメージ処理を行う関数の参照ポインタ
* @param message オーラダメージを受けた際のメッセージ
* @return なし
*/
static void touch_zap_player_aux(monster_type *m_ptr, player_type *touched_ptr, bool immune, int flags_offset, int r_flags_offset, u32b aura_flag,
	HIT_POINT(*dam_func)(player_type *creature_type, HIT_POINT dam, concptr kb_str, int monspell, bool aura), concptr message)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	if (!(atoffset(BIT_FLAGS, r_ptr, flags_offset) & aura_flag) || immune) return;
	
	GAME_TEXT mon_name[MAX_NLEN];
	int aura_damage = damroll(1 + (r_ptr->level / 26), 1 + (r_ptr->level / 17));

	monster_desc(touched_ptr, mon_name, m_ptr, MD_WRONGDOER_NAME);
	msg_print(message);
	dam_func(touched_ptr, aura_damage, mon_name, -1, TRUE);

	if (is_original_ap_and_seen(touched_ptr, m_ptr))
	{
		atoffset(BIT_FLAGS, r_ptr, r_flags_offset) |= aura_flag;
	}

	handle_stuff(touched_ptr);
}


/*!
* @brief 敵オーラによるプレイヤーのダメージ処理（メイン）
* @param m_ptr オーラを持つモンスターの構造体参照ポインタ
* @param touched_ptr オーラを持つ相手に振れたクリーチャーの参照ポインタ
* @return なし
*/
static void touch_zap_player(monster_type *m_ptr, player_type *touched_ptr)
{
	touch_zap_player_aux(m_ptr, touched_ptr, touched_ptr->immune_fire, offsetof(monster_race, flags2), offsetof(monster_race, r_flags2), RF2_AURA_FIRE,
		fire_dam, _("突然とても熱くなった！", "You are suddenly very hot!"));
	touch_zap_player_aux(m_ptr, touched_ptr, touched_ptr->immune_cold, offsetof(monster_race, flags3), offsetof(monster_race, r_flags3), RF3_AURA_COLD,
		cold_dam, _("突然とても寒くなった！", "You are suddenly very cold!"));
	touch_zap_player_aux(m_ptr, touched_ptr, touched_ptr->immune_elec, offsetof(monster_race, flags2), offsetof(monster_race, r_flags2), RF2_AURA_ELEC,
		elec_dam, _("電撃をくらった！", "You get zapped!"));
}


/*!
* @brief プレイヤーの変異要素による打撃処理
* @param attacker_ptr プレーヤーへの参照ポインタ
* @param m_idx 攻撃目標となったモンスターの参照ID
* @param attack 変異要素による攻撃要素の種類
* @param fear 攻撃を受けたモンスターが恐慌状態に陥ったかを返す参照ポインタ
* @param mdeath 攻撃を受けたモンスターが死亡したかを返す参照ポインタ
* @return なし
*/
static void natural_attack(player_type *attacker_ptr, MONSTER_IDX m_idx, int attack, bool *fear, bool *mdeath)
{
	WEIGHT n_weight = 0;
	monster_type *m_ptr = &attacker_ptr->current_floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	int dice_num, dice_side;
	concptr atk_desc;
	switch (attack)
	{
	case MUT2_SCOR_TAIL:
		dice_num = 3;
		dice_side = 7;
		n_weight = 5;
		atk_desc = _("尻尾", "tail");

		break;
	case MUT2_HORNS:
		dice_num = 2;
		dice_side = 6;
		n_weight = 15;
		atk_desc = _("角", "horns");

		break;
	case MUT2_BEAK:
		dice_num = 2;
		dice_side = 4;
		n_weight = 5;
		atk_desc = _("クチバシ", "beak");

		break;
	case MUT2_TRUNK:
		dice_num = 1;
		dice_side = 4;
		n_weight = 35;
		atk_desc = _("象の鼻", "trunk");

		break;
	case MUT2_TENTACLES:
		dice_num = 2;
		dice_side = 5;
		n_weight = 5;
		atk_desc = _("触手", "tentacles");

		break;
	default:
		dice_num = dice_side = n_weight = 1;
		atk_desc = _("未定義の部位", "undefined body part");

	}

	GAME_TEXT m_name[MAX_NLEN];
	monster_desc(attacker_ptr, m_name, m_ptr, 0);

	/* Calculate the "attack quality" */
	int bonus = attacker_ptr->to_h_m + (attacker_ptr->lev * 6 / 5);
	int chance = (attacker_ptr->skill_thn + (bonus * BTH_PLUS_ADJ));

	bool is_hit = ((r_ptr->flags2 & RF2_QUANTUM) == 0) || !randint0(2);
	is_hit &= test_hit_norm(attacker_ptr, chance, r_ptr->ac, m_ptr->ml);
	if (!is_hit)
	{
		sound(SOUND_MISS);
		msg_format(_("ミス！ %sにかわされた。", "You miss %s."), m_name);
		return;
	}

	sound(SOUND_HIT);
	msg_format(_("%sを%sで攻撃した。", "You hit %s with your %s."), m_name, atk_desc);

	HIT_POINT k = damroll(dice_num, dice_side);
	k = critical_norm(attacker_ptr, n_weight, bonus, k, (s16b)bonus, 0);

	/* Apply the player damage bonuses */
	k += attacker_ptr->to_d_m;

	/* No negative damage */
	if (k < 0) k = 0;

	/* Modify the damage */
	k = mon_damage_mod(attacker_ptr, m_ptr, k, FALSE);

	/* Complex message */
	msg_format_wizard(CHEAT_MONSTER,
		_("%dのダメージを与えた。(残りHP %d/%d(%d))", "You do %d damage. (left HP %d/%d(%d))"),
		k, m_ptr->hp - k, m_ptr->maxhp, m_ptr->max_maxhp);

	/* Anger the monster */
	if (k > 0) anger_monster(attacker_ptr, m_ptr);

	/* Damage, check for fear and mdeath */
	switch (attack)
	{
	case MUT2_SCOR_TAIL:
		project(attacker_ptr, 0, 0, m_ptr->fy, m_ptr->fx, k, GF_POIS, PROJECT_KILL, -1);
		*mdeath = (m_ptr->r_idx == 0);
		break;
	case MUT2_HORNS:
		*mdeath = mon_take_hit(attacker_ptr, m_idx, k, fear, NULL);
		break;
	case MUT2_BEAK:
		*mdeath = mon_take_hit(attacker_ptr, m_idx, k, fear, NULL);
		break;
	case MUT2_TRUNK:
		*mdeath = mon_take_hit(attacker_ptr, m_idx, k, fear, NULL);
		break;
	case MUT2_TENTACLES:
		*mdeath = mon_take_hit(attacker_ptr, m_idx, k, fear, NULL);
		break;
	default:
		*mdeath = mon_take_hit(attacker_ptr, m_idx, k, fear, NULL);
	}

	touch_zap_player(m_ptr, attacker_ptr);
}

/*!
* @brief プレイヤーの打撃処理サブルーチン /
* Player attacks a (poor, defenseless) creature        -RAK-
* @param y 攻撃目標のY座標
* @param x 攻撃目標のX座標
* @param fear 攻撃を受けたモンスターが恐慌状態に陥ったかを返す参照ポインタ
* @param mdeath 攻撃を受けたモンスターが死亡したかを返す参照ポインタ
* @param hand 攻撃を行うための武器を持つ手
* @param mode 発動中の剣術ID
* @return なし
* @details
* If no "weapon" is available, then "punch" the monster one time.
*/
static void py_attack_aux(player_type *attacker_ptr, POSITION y, POSITION x, bool *fear, bool *mdeath, s16b hand, COMBAT_OPTION_IDX mode)
{
	int num = 0, bonus, chance, vir;
	HIT_POINT k;

	floor_type *floor_ptr = attacker_ptr->current_floor_ptr;
	grid_type       *g_ptr = &floor_ptr->grid_array[y][x];

	monster_type    *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
	monster_race    *r_ptr = &r_info[m_ptr->r_idx];

	/* Access the weapon */
	object_type     *o_ptr = &attacker_ptr->inventory_list[INVEN_RARM + hand];

	GAME_TEXT m_name[MAX_NLEN];

	bool            success_hit = FALSE;
	bool            backstab = FALSE;
	bool            vorpal_cut = FALSE;
	int             chaos_effect = 0;
	bool            stab_fleeing = FALSE;
	bool            fuiuchi = FALSE;
	bool            monk_attack = FALSE;
	bool            do_quake = FALSE;
	bool            weak = FALSE;
	bool            drain_msg = TRUE;
	int             drain_result = 0, drain_heal = 0;
	bool            can_drain = FALSE;
	int             num_blow;
	int             drain_left = MAX_VAMPIRIC_DRAIN;
	BIT_FLAGS flgs[TR_FLAG_SIZE]; /* A massive hack -- life-draining weapons */
	bool            is_human = (r_ptr->d_char == 'p');
	bool            is_lowlevel = (r_ptr->level < (attacker_ptr->lev - 15));
	bool            zantetsu_mukou, e_j_mukou;

	switch (attacker_ptr->pclass)
	{
	case CLASS_ROGUE:
	case CLASS_NINJA:
		if (has_melee_weapon(attacker_ptr, INVEN_RARM + hand) && !attacker_ptr->icky_wield[hand])
		{
			int tmp = attacker_ptr->lev * 6 + (attacker_ptr->skill_stl + 10) * 4;
			if (attacker_ptr->monlite && (mode != HISSATSU_NYUSIN)) tmp /= 3;
			if (attacker_ptr->cursed & TRC_AGGRAVATE) tmp /= 2;
			if (r_ptr->level > (attacker_ptr->lev * attacker_ptr->lev / 20 + 10)) tmp /= 3;
			if (MON_CSLEEP(m_ptr) && m_ptr->ml)
			{
				/* Can't backstab creatures that we can't see, right? */
				backstab = TRUE;
			}
			else if ((attacker_ptr->special_defense & NINJA_S_STEALTH) && (randint0(tmp) > (r_ptr->level + 20)) && m_ptr->ml && !(r_ptr->flagsr & RFR_RES_ALL))
			{
				fuiuchi = TRUE;
			}
			else if (MON_MONFEAR(m_ptr) && m_ptr->ml)
			{
				stab_fleeing = TRUE;
			}
		}

		break;

	case CLASS_MONK:
	case CLASS_FORCETRAINER:
	case CLASS_BERSERKER:
		if ((empty_hands(attacker_ptr, TRUE) & EMPTY_HAND_RARM) && !attacker_ptr->riding) monk_attack = TRUE;
		break;
	}

	if (!o_ptr->k_idx) /* Empty hand */
	{
		if ((r_ptr->level + 10) > attacker_ptr->lev)
		{
			if (attacker_ptr->skill_exp[GINOU_SUDE] < s_info[attacker_ptr->pclass].s_max[GINOU_SUDE])
			{
				if (attacker_ptr->skill_exp[GINOU_SUDE] < WEAPON_EXP_BEGINNER)
					attacker_ptr->skill_exp[GINOU_SUDE] += 40;
				else if ((attacker_ptr->skill_exp[GINOU_SUDE] < WEAPON_EXP_SKILLED))
					attacker_ptr->skill_exp[GINOU_SUDE] += 5;
				else if ((attacker_ptr->skill_exp[GINOU_SUDE] < WEAPON_EXP_EXPERT) && (attacker_ptr->lev > 19))
					attacker_ptr->skill_exp[GINOU_SUDE] += 1;
				else if ((attacker_ptr->lev > 34))
					if (one_in_(3)) attacker_ptr->skill_exp[GINOU_SUDE] += 1;
				attacker_ptr->update |= (PU_BONUS);
			}
		}
	}
	else if (object_is_melee_weapon(o_ptr))
	{
		if ((r_ptr->level + 10) > attacker_ptr->lev)
		{
			OBJECT_TYPE_VALUE tval = attacker_ptr->inventory_list[INVEN_RARM + hand].tval - TV_WEAPON_BEGIN;
			OBJECT_SUBTYPE_VALUE sval = attacker_ptr->inventory_list[INVEN_RARM + hand].sval;
			int now_exp = attacker_ptr->weapon_exp[tval][sval];
			if (now_exp < s_info[attacker_ptr->pclass].w_max[tval][sval])
			{
				SUB_EXP amount = 0;
				if (now_exp < WEAPON_EXP_BEGINNER) amount = 80;
				else if (now_exp < WEAPON_EXP_SKILLED) amount = 10;
				else if ((now_exp < WEAPON_EXP_EXPERT) && (attacker_ptr->lev > 19)) amount = 1;
				else if ((attacker_ptr->lev > 34) && one_in_(2)) amount = 1;
				attacker_ptr->weapon_exp[tval][sval] += amount;
				attacker_ptr->update |= (PU_BONUS);
			}
		}
	}

	/* Disturb the monster */
	(void)set_monster_csleep(attacker_ptr, g_ptr->m_idx, 0);

	monster_desc(attacker_ptr, m_name, m_ptr, 0);

	/* Calculate the "attack quality" */
	bonus = attacker_ptr->to_h[hand] + o_ptr->to_h;
	chance = (attacker_ptr->skill_thn + (bonus * BTH_PLUS_ADJ));
	if (mode == HISSATSU_IAI) chance += 60;
	if (attacker_ptr->special_defense & KATA_KOUKIJIN) chance += 150;
	if (attacker_ptr->sutemi) chance = MAX(chance * 3 / 2, chance + 60);

	vir = virtue_number(attacker_ptr, V_VALOUR);
	if (vir)
	{
		chance += (attacker_ptr->virtues[vir - 1] / 10);
	}

	zantetsu_mukou = ((o_ptr->name1 == ART_ZANTETSU) && (r_ptr->d_char == 'j'));
	e_j_mukou = ((o_ptr->name1 == ART_EXCALIBUR_J) && (r_ptr->d_char == 'S'));

	if ((mode == HISSATSU_KYUSHO) || (mode == HISSATSU_MINEUCHI) || (mode == HISSATSU_3DAN) || (mode == HISSATSU_IAI)) num_blow = 1;
	else if (mode == HISSATSU_COLD) num_blow = attacker_ptr->num_blow[hand] + 2;
	else num_blow = attacker_ptr->num_blow[hand];

	/* Hack -- DOKUBARI always hit once */
	if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_POISON_NEEDLE)) num_blow = 1;

	/* Attack once for each legal blow */
	while ((num++ < num_blow) && !attacker_ptr->is_dead)
	{
		if (((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_POISON_NEEDLE)) || (mode == HISSATSU_KYUSHO))
		{
			int n = 1;

			if (attacker_ptr->migite && attacker_ptr->hidarite)
			{
				n *= 2;
			}
			if (mode == HISSATSU_3DAN)
			{
				n *= 2;
			}

			success_hit = one_in_(n);
		}
		else if ((attacker_ptr->pclass == CLASS_NINJA) && ((backstab || fuiuchi) && !(r_ptr->flagsr & RFR_RES_ALL))) success_hit = TRUE;
		else success_hit = test_hit_norm(attacker_ptr, chance, r_ptr->ac, m_ptr->ml);

		if (mode == HISSATSU_MAJIN)
		{
			if (one_in_(2))
				success_hit = FALSE;
		}

		if (!success_hit)
		{
			backstab = FALSE; /* Clumsy! */
			fuiuchi = FALSE; /* Clumsy! */

			if ((o_ptr->tval == TV_POLEARM) && (o_ptr->sval == SV_DEATH_SCYTHE) && one_in_(3))
			{
				BIT_FLAGS flgs_aux[TR_FLAG_SIZE];

				sound(SOUND_HIT);

				msg_format(_("ミス！ %sにかわされた。", "You miss %s."), m_name);
				msg_print(_("振り回した大鎌が自分自身に返ってきた！", "Your scythe returns to you!"));
				object_flags(o_ptr, flgs_aux);

				k = damroll(o_ptr->dd + attacker_ptr->to_dd[hand], o_ptr->ds + attacker_ptr->to_ds[hand]);
				{
					int mult;
					switch (attacker_ptr->mimic_form)
					{
					case MIMIC_NONE:
						switch (attacker_ptr->prace)
						{
						case RACE_YEEK:
						case RACE_KLACKON:
						case RACE_HUMAN:
						case RACE_AMBERITE:
						case RACE_DUNADAN:
						case RACE_BARBARIAN:
						case RACE_BEASTMAN:
							mult = 25; break;
						case RACE_HALF_ORC:
						case RACE_HALF_TROLL:
						case RACE_HALF_OGRE:
						case RACE_HALF_GIANT:
						case RACE_HALF_TITAN:
						case RACE_CYCLOPS:
						case RACE_IMP:
						case RACE_SKELETON:
						case RACE_ZOMBIE:
						case RACE_VAMPIRE:
						case RACE_SPECTRE:
						case RACE_DEMON:
						case RACE_DRACONIAN:
							mult = 30; break;
						default:
							mult = 10; break;
						}
						break;
					case MIMIC_DEMON:
					case MIMIC_DEMON_LORD:
					case MIMIC_VAMPIRE:
						mult = 30; break;
					default:
						mult = 10; break;
					}

					if (attacker_ptr->align < 0 && mult < 20)
						mult = 20;
					if (!(attacker_ptr->resist_acid || is_oppose_acid(attacker_ptr) || attacker_ptr->immune_acid) && (mult < 25))
						mult = 25;
					if (!(attacker_ptr->resist_elec || is_oppose_elec(attacker_ptr) || attacker_ptr->immune_elec) && (mult < 25))
						mult = 25;
					if (!(attacker_ptr->resist_fire || is_oppose_fire(attacker_ptr) || attacker_ptr->immune_fire) && (mult < 25))
						mult = 25;
					if (!(attacker_ptr->resist_cold || is_oppose_cold(attacker_ptr) || attacker_ptr->immune_cold) && (mult < 25))
						mult = 25;
					if (!(attacker_ptr->resist_pois || is_oppose_pois(attacker_ptr)) && (mult < 25))
						mult = 25;

					if ((attacker_ptr->pclass != CLASS_SAMURAI) && (have_flag(flgs_aux, TR_FORCE_WEAPON)) && (attacker_ptr->csp > (attacker_ptr->msp / 30)))
					{
						attacker_ptr->csp -= (1 + (attacker_ptr->msp / 30));
						attacker_ptr->redraw |= (PR_MANA);
						mult = mult * 3 / 2 + 20;
					}
					k *= (HIT_POINT)mult;
					k /= 10;
				}

				k = critical_norm(attacker_ptr, o_ptr->weight, o_ptr->to_h, k, attacker_ptr->to_h[hand], mode);
				if (one_in_(6))
				{
					int mult = 2;
					msg_format(_("グッサリ切り裂かれた！", "Your weapon cuts deep into yourself!"));
					/* Try to increase the damage */
					while (one_in_(4))
					{
						mult++;
					}

					k *= (HIT_POINT)mult;
				}
				k += (attacker_ptr->to_d[hand] + o_ptr->to_d);
				if (k < 0) k = 0;

				take_hit(attacker_ptr, DAMAGE_FORCE, k, _("死の大鎌", "Death scythe"), -1);
				handle_stuff(attacker_ptr);
			}
			else
			{
				sound(SOUND_MISS);
				msg_format(_("ミス！ %sにかわされた。", "You miss %s."), m_name);
			}

			continue;
		}

		int vorpal_chance = ((o_ptr->name1 == ART_VORPAL_BLADE) || (o_ptr->name1 == ART_CHAINSWORD)) ? 2 : 4;

		sound(SOUND_HIT);

		if (backstab) msg_format(_("あなたは冷酷にも眠っている無力な%sを突き刺した！", "You cruelly stab the helpless, sleeping %s!"), m_name);
		else if (fuiuchi) msg_format(_("不意を突いて%sに強烈な一撃を喰らわせた！", "You make surprise attack, and hit %s with a powerful blow!"), m_name);
		else if (stab_fleeing) msg_format(_("逃げる%sを背中から突き刺した！", "You backstab the fleeing %s!"), m_name);
		else if (!monk_attack) msg_format(_("%sを攻撃した。", "You hit %s."), m_name);

		/* Hack -- bare hands do one damage */
		k = 1;

		object_flags(o_ptr, flgs);

		/* Select a chaotic effect (50% chance) */
		if ((have_flag(flgs, TR_CHAOTIC)) && one_in_(2))
		{
			if (one_in_(10))
				chg_virtue(attacker_ptr, V_CHANCE, 1);

			if (randint1(5) < 3)
			{
				/* Vampiric (20%) */
				chaos_effect = 1;
			}
			else if (one_in_(250))
			{
				/* Quake (0.12%) */
				chaos_effect = 2;
			}
			else if (!one_in_(10))
			{
				/* Confusion (26.892%) */
				chaos_effect = 3;
			}
			else if (one_in_(2))
			{
				/* Teleport away (1.494%) */
				chaos_effect = 4;
			}
			else
			{
				/* Polymorph (1.494%) */
				chaos_effect = 5;
			}
		}

		/* Vampiric drain */
		if ((have_flag(flgs, TR_VAMPIRIC)) || (chaos_effect == 1) || (mode == HISSATSU_DRAIN) || hex_spelling(attacker_ptr, HEX_VAMP_BLADE))
		{
			/* Only drain "living" monsters */
			if (monster_living(m_ptr->r_idx))
				can_drain = TRUE;
			else
				can_drain = FALSE;
		}

		if ((have_flag(flgs, TR_VORPAL) || hex_spelling(attacker_ptr, HEX_RUNESWORD)) && (randint1(vorpal_chance * 3 / 2) == 1) && !zantetsu_mukou)
			vorpal_cut = TRUE;
		else vorpal_cut = FALSE;

		if (monk_attack)
		{
			int special_effect = 0, stun_effect = 0, times = 0, max_times;
			int min_level = 1;
			const martial_arts *ma_ptr = &ma_blows[0], *old_ptr = &ma_blows[0];
			int resist_stun = 0;
			WEIGHT weight = 8;

			if (r_ptr->flags1 & RF1_UNIQUE) resist_stun += 88;
			if (r_ptr->flags3 & RF3_NO_STUN) resist_stun += 66;
			if (r_ptr->flags3 & RF3_NO_CONF) resist_stun += 33;
			if (r_ptr->flags3 & RF3_NO_SLEEP) resist_stun += 33;
			if ((r_ptr->flags3 & RF3_UNDEAD) || (r_ptr->flags3 & RF3_NONLIVING))
				resist_stun += 66;

			if (attacker_ptr->special_defense & KAMAE_BYAKKO)
				max_times = (attacker_ptr->lev < 3 ? 1 : attacker_ptr->lev / 3);
			else if (attacker_ptr->special_defense & KAMAE_SUZAKU)
				max_times = 1;
			else if (attacker_ptr->special_defense & KAMAE_GENBU)
				max_times = 1;
			else
				max_times = (attacker_ptr->lev < 7 ? 1 : attacker_ptr->lev / 7);
			/* Attempt 'times' */
			for (times = 0; times < max_times; times++)
			{
				do
				{
					ma_ptr = &ma_blows[randint0(MAX_MA)];
					if ((attacker_ptr->pclass == CLASS_FORCETRAINER) && (ma_ptr->min_level > 1)) min_level = ma_ptr->min_level + 3;
					else min_level = ma_ptr->min_level;
				} while ((min_level > attacker_ptr->lev) ||
					(randint1(attacker_ptr->lev) < ma_ptr->chance));

				/* keep the highest level attack available we found */
				if ((ma_ptr->min_level > old_ptr->min_level) &&
					!attacker_ptr->stun && !attacker_ptr->confused)
				{
					old_ptr = ma_ptr;

					if (current_world_ptr->wizard && cheat_xtra)
					{
						msg_print(_("攻撃を再選択しました。", "Attack re-selected."));
					}
				}
				else
				{
					ma_ptr = old_ptr;
				}
			}

			if (attacker_ptr->pclass == CLASS_FORCETRAINER) min_level = MAX(1, ma_ptr->min_level - 3);
			else min_level = ma_ptr->min_level;
			k = damroll(ma_ptr->dd + attacker_ptr->to_dd[hand], ma_ptr->ds + attacker_ptr->to_ds[hand]);
			if (attacker_ptr->special_attack & ATTACK_SUIKEN) k *= 2;

			if (ma_ptr->effect == MA_KNEE)
			{
				if (r_ptr->flags1 & RF1_MALE)
				{
					msg_format(_("%sに金的膝蹴りをくらわした！", "You hit %s in the groin with your knee!"), m_name);
					sound(SOUND_PAIN);
					special_effect = MA_KNEE;
				}
				else
					msg_format(ma_ptr->desc, m_name);
			}

			else if (ma_ptr->effect == MA_SLOW)
			{
				if (!((r_ptr->flags1 & RF1_NEVER_MOVE) ||
					my_strchr("~#{}.UjmeEv$,DdsbBFIJQSXclnw!=?", r_ptr->d_char)))
				{
					msg_format(_("%sの足首に関節蹴りをくらわした！", "You kick %s in the ankle."), m_name);
					special_effect = MA_SLOW;
				}
				else msg_format(ma_ptr->desc, m_name);
			}
			else
			{
				if (ma_ptr->effect)
				{
					stun_effect = (ma_ptr->effect / 2) + randint1(ma_ptr->effect / 2);
				}

				msg_format(ma_ptr->desc, m_name);
			}

			if (attacker_ptr->special_defense & KAMAE_SUZAKU) weight = 4;
			if ((attacker_ptr->pclass == CLASS_FORCETRAINER) && P_PTR_KI)
			{
				weight += (P_PTR_KI / 30);
				if (weight > 20) weight = 20;
			}

			k = critical_norm(attacker_ptr, attacker_ptr->lev * weight, min_level, k, attacker_ptr->to_h[0], 0);

			if ((special_effect == MA_KNEE) && ((k + attacker_ptr->to_d[hand]) < m_ptr->hp))
			{
				msg_format(_("%^sは苦痛にうめいている！", "%^s moans in agony!"), m_name);
				stun_effect = 7 + randint1(13);
				resist_stun /= 3;
			}

			else if ((special_effect == MA_SLOW) && ((k + attacker_ptr->to_d[hand]) < m_ptr->hp))
			{
				if (!(r_ptr->flags1 & RF1_UNIQUE) &&
					(randint1(attacker_ptr->lev) > r_ptr->level) &&
					m_ptr->mspeed > 60)
				{
					msg_format(_("%^sは足をひきずり始めた。", "%^s starts limping slower."), m_name);
					m_ptr->mspeed -= 10;
				}
			}

			if (stun_effect && ((k + attacker_ptr->to_d[hand]) < m_ptr->hp))
			{
				if (attacker_ptr->lev > randint1(r_ptr->level + resist_stun + 10))
				{
					if (set_monster_stunned(attacker_ptr, g_ptr->m_idx, stun_effect + MON_STUNNED(m_ptr)))
					{
						msg_format(_("%^sはフラフラになった。", "%^s is stunned."), m_name);
					}
					else
					{
						msg_format(_("%^sはさらにフラフラになった。", "%^s is more stunned."), m_name);
					}
				}
			}
		}

		/* Handle normal weapon */
		else if (o_ptr->k_idx)
		{
			k = damroll(o_ptr->dd + attacker_ptr->to_dd[hand], o_ptr->ds + attacker_ptr->to_ds[hand]);
			k = tot_dam_aux(attacker_ptr, o_ptr, k, m_ptr, mode, FALSE);

			if (backstab)
			{
				k *= (3 + (attacker_ptr->lev / 20));
			}
			else if (fuiuchi)
			{
				k = k * (5 + (attacker_ptr->lev * 2 / 25)) / 2;
			}
			else if (stab_fleeing)
			{
				k = (3 * k) / 2;
			}

			if ((attacker_ptr->impact[hand] && ((k > 50) || one_in_(7))) ||
				(chaos_effect == 2) || (mode == HISSATSU_QUAKE))
			{
				do_quake = TRUE;
			}

			if ((!(o_ptr->tval == TV_SWORD) || !(o_ptr->sval == SV_POISON_NEEDLE)) && !(mode == HISSATSU_KYUSHO))
				k = critical_norm(attacker_ptr, o_ptr->weight, o_ptr->to_h, k, attacker_ptr->to_h[hand], mode);

			drain_result = k;

			if (vorpal_cut)
			{
				int mult = 2;

				if ((o_ptr->name1 == ART_CHAINSWORD) && !one_in_(2))
				{
					char chainsword_noise[1024];
					if (!get_rnd_line(_("chainswd_j.txt", "chainswd.txt"), 0, chainsword_noise))
					{
						msg_print(chainsword_noise);
					}
				}

				if (o_ptr->name1 == ART_VORPAL_BLADE)
				{
					msg_print(_("目にも止まらぬヴォーパルブレード、手錬の早業！", "Your Vorpal Blade goes snicker-snack!"));
				}
				else
				{
					msg_format(_("%sをグッサリ切り裂いた！", "Your weapon cuts deep into %s!"), m_name);
				}

				/* Try to increase the damage */
				while (one_in_(vorpal_chance))
				{
					mult++;
				}

				k *= (HIT_POINT)mult;

				/* Ouch! */
				if (((r_ptr->flagsr & RFR_RES_ALL) ? k / 100 : k) > m_ptr->hp)
				{
					msg_format(_("%sを真っ二つにした！", "You cut %s in half!"), m_name);
				}
				else
				{
					switch (mult)
					{
					case 2: msg_format(_("%sを斬った！", "You gouge %s!"), m_name); break;
					case 3: msg_format(_("%sをぶった斬った！", "You maim %s!"), m_name); break;
					case 4: msg_format(_("%sをメッタ斬りにした！", "You carve %s!"), m_name); break;
					case 5: msg_format(_("%sをメッタメタに斬った！", "You cleave %s!"), m_name); break;
					case 6: msg_format(_("%sを刺身にした！", "You smite %s!"), m_name); break;
					case 7: msg_format(_("%sを斬って斬って斬りまくった！", "You eviscerate %s!"), m_name); break;
					default: msg_format(_("%sを細切れにした！", "You shred %s!"), m_name); break;
					}
				}
				drain_result = drain_result * 3 / 2;
			}

			k += o_ptr->to_d;
			drain_result += o_ptr->to_d;
		}

		/* Apply the player damage bonuses */
		k += attacker_ptr->to_d[hand];
		drain_result += attacker_ptr->to_d[hand];

		if ((mode == HISSATSU_SUTEMI) || (mode == HISSATSU_3DAN)) k *= 2;
		if ((mode == HISSATSU_SEKIRYUKA) && !monster_living(m_ptr->r_idx)) k = 0;
		if ((mode == HISSATSU_SEKIRYUKA) && !attacker_ptr->cut) k /= 2;

		/* No negative damage */
		if (k < 0) k = 0;

		if ((mode == HISSATSU_ZANMA) && !(!monster_living(m_ptr->r_idx) && (r_ptr->flags3 & RF3_EVIL)))
		{
			k = 0;
		}

		if (zantetsu_mukou)
		{
			msg_print(_("こんな軟らかいものは切れん！", "You cannot cut such a elastic thing!"));
			k = 0;
		}

		if (e_j_mukou)
		{
			msg_print(_("蜘蛛は苦手だ！", "Spiders are difficult for you to deal with!"));
			k /= 2;
		}

		if (mode == HISSATSU_MINEUCHI)
		{
			int tmp = (10 + randint1(15) + attacker_ptr->lev / 5);

			k = 0;
			anger_monster(attacker_ptr, m_ptr);

			if (!(r_ptr->flags3 & (RF3_NO_STUN)))
			{
				/* Get stunned */
				if (MON_STUNNED(m_ptr))
				{
					msg_format(_("%sはひどくもうろうとした。", "%s is more dazed."), m_name);
					tmp /= 2;
				}
				else
				{
					msg_format(_("%s はもうろうとした。", "%s is dazed."), m_name);
				}

				/* Apply stun */
				(void)set_monster_stunned(attacker_ptr, g_ptr->m_idx, MON_STUNNED(m_ptr) + tmp);
			}
			else
			{
				msg_format(_("%s には効果がなかった。", "%s is not effected."), m_name);
			}
		}

		/* Modify the damage */
		k = mon_damage_mod(attacker_ptr, m_ptr, k, (bool)(((o_ptr->tval == TV_POLEARM) && (o_ptr->sval == SV_DEATH_SCYTHE)) || ((attacker_ptr->pclass == CLASS_BERSERKER) && one_in_(2))));
		if (((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_POISON_NEEDLE)) || (mode == HISSATSU_KYUSHO))
		{
			if ((randint1(randint1(r_ptr->level / 7) + 5) == 1) && !(r_ptr->flags1 & RF1_UNIQUE) && !(r_ptr->flags7 & RF7_UNIQUE2))
			{
				k = m_ptr->hp + 1;
				msg_format(_("%sの急所を突き刺した！", "You hit %s on a fatal spot!"), m_name);
			}
			else k = 1;
		}
		else if ((attacker_ptr->pclass == CLASS_NINJA) && has_melee_weapon(attacker_ptr, INVEN_RARM + hand) && !attacker_ptr->icky_wield[hand] && ((attacker_ptr->cur_lite <= 0) || one_in_(7)))
		{
			int maxhp = maxroll(r_ptr->hdice, r_ptr->hside);
			if (one_in_(backstab ? 13 : (stab_fleeing || fuiuchi) ? 15 : 27))
			{
				k *= 5;
				drain_result *= 2;
				msg_format(_("刃が%sに深々と突き刺さった！", "You critically injured %s!"), m_name);
			}
			else if (((m_ptr->hp < maxhp / 2) && one_in_((attacker_ptr->num_blow[0] + attacker_ptr->num_blow[1] + 1) * 10)) || ((one_in_(666) || ((backstab || fuiuchi) && one_in_(11))) && !(r_ptr->flags1 & RF1_UNIQUE) && !(r_ptr->flags7 & RF7_UNIQUE2)))
			{
				if ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags7 & RF7_UNIQUE2) || (m_ptr->hp >= maxhp / 2))
				{
					k = MAX(k * 5, m_ptr->hp / 2);
					drain_result *= 2;
					msg_format(_("%sに致命傷を負わせた！", "You fatally injured %s!"), m_name);
				}
				else
				{
					k = m_ptr->hp + 1;
					msg_format(_("刃が%sの急所を貫いた！", "You hit %s on a fatal spot!"), m_name);
				}
			}
		}

		msg_format_wizard(CHEAT_MONSTER,
			_("%dのダメージを与えた。(残りHP %d/%d(%d))", "You do %d damage. (left HP %d/%d(%d))"), k,
			m_ptr->hp - k, m_ptr->maxhp, m_ptr->max_maxhp);

		if (k <= 0) can_drain = FALSE;

		if (drain_result > m_ptr->hp)
			drain_result = m_ptr->hp;

		/* Damage, check for fear and death */
		if (mon_take_hit(attacker_ptr, g_ptr->m_idx, k, fear, NULL))
		{
			*mdeath = TRUE;
			if ((attacker_ptr->pclass == CLASS_BERSERKER) && attacker_ptr->energy_use)
			{
				if (attacker_ptr->migite && attacker_ptr->hidarite)
				{
					if (hand) attacker_ptr->energy_use = attacker_ptr->energy_use * 3 / 5 + attacker_ptr->energy_use*num * 2 / (attacker_ptr->num_blow[hand] * 5);
					else attacker_ptr->energy_use = attacker_ptr->energy_use*num * 3 / (attacker_ptr->num_blow[hand] * 5);
				}
				else
				{
					attacker_ptr->energy_use = attacker_ptr->energy_use*num / attacker_ptr->num_blow[hand];
				}
			}
			if ((o_ptr->name1 == ART_ZANTETSU) && is_lowlevel)
				msg_print(_("またつまらぬものを斬ってしまった．．．", "Sigh... Another trifling thing I've cut...."));
			break;
		}

		/* Anger the monster */
		if (k > 0) anger_monster(attacker_ptr, m_ptr);

		touch_zap_player(m_ptr, attacker_ptr);

		/* Are we draining it?  A little note: If the monster is
		dead, the drain does not work... */

		if (can_drain && (drain_result > 0))
		{
			if (o_ptr->name1 == ART_MURAMASA)
			{
				if (is_human)
				{
					HIT_PROB to_h = o_ptr->to_h;
					HIT_POINT to_d = o_ptr->to_d;
					int i, flag;

					flag = 1;
					for (i = 0; i < to_h + 3; i++) if (one_in_(4)) flag = 0;
					if (flag) to_h++;

					flag = 1;
					for (i = 0; i < to_d + 3; i++) if (one_in_(4)) flag = 0;
					if (flag) to_d++;

					if (o_ptr->to_h != to_h || o_ptr->to_d != to_d)
					{
						msg_print(_("妖刀は血を吸って強くなった！", "Muramasa sucked blood, and became more powerful!"));
						o_ptr->to_h = to_h;
						o_ptr->to_d = to_d;
					}
				}
			}
			else
			{
				if (drain_result > 5) /* Did we really hurt it? */
				{
					drain_heal = damroll(2, drain_result / 6);

					if (hex_spelling(attacker_ptr, HEX_VAMP_BLADE)) drain_heal *= 2;

					if (cheat_xtra)
					{
						msg_format(_("Draining left: %d", "Draining left: %d"), drain_left);
					}

					if (drain_left)
					{
						if (drain_heal < drain_left)
						{
							drain_left -= drain_heal;
						}
						else
						{
							drain_heal = drain_left;
							drain_left = 0;
						}

						if (drain_msg)
						{
							msg_format(_("刃が%sから生命力を吸い取った！", "Your weapon drains life from %s!"), m_name);
							drain_msg = FALSE;
						}

						drain_heal = (drain_heal * attacker_ptr->mutant_regenerate_mod) / 100;

						hp_player(attacker_ptr, drain_heal);
						/* We get to keep some of it! */
					}
				}
			}

			m_ptr->maxhp -= (k + 7) / 8;
			if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;
			if (m_ptr->maxhp < 1) m_ptr->maxhp = 1;
			weak = TRUE;
		}

		can_drain = FALSE;
		drain_result = 0;

		/* Confusion attack */
		if ((attacker_ptr->special_attack & ATTACK_CONFUSE) || (chaos_effect == 3) || (mode == HISSATSU_CONF) || hex_spelling(attacker_ptr, HEX_CONFUSION))
		{
			/* Cancel glowing hands */
			if (attacker_ptr->special_attack & ATTACK_CONFUSE)
			{
				attacker_ptr->special_attack &= ~(ATTACK_CONFUSE);
				msg_print(_("手の輝きがなくなった。", "Your hands stop glowing."));
				attacker_ptr->redraw |= (PR_STATUS);

			}

			/* Confuse the monster */
			if (r_ptr->flags3 & RF3_NO_CONF)
			{
				if (is_original_ap_and_seen(attacker_ptr, m_ptr)) r_ptr->r_flags3 |= RF3_NO_CONF;
				msg_format(_("%^sには効果がなかった。", "%^s is unaffected."), m_name);

			}
			else if (randint0(100) < r_ptr->level)
			{
				msg_format(_("%^sには効果がなかった。", "%^s is unaffected."), m_name);
			}
			else
			{
				msg_format(_("%^sは混乱したようだ。", "%^s appears confused."), m_name);
				(void)set_monster_confused(attacker_ptr, g_ptr->m_idx, MON_CONFUSED(m_ptr) + 10 + randint0(attacker_ptr->lev) / 5);
			}
		}

		else if (chaos_effect == 4)
		{
			bool resists_tele = FALSE;

			if (r_ptr->flagsr & RFR_RES_TELE)
			{
				if (r_ptr->flags1 & RF1_UNIQUE)
				{
					if (is_original_ap_and_seen(attacker_ptr, m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
					msg_format(_("%^sには効果がなかった。", "%^s is unaffected!"), m_name);
					resists_tele = TRUE;
				}
				else if (r_ptr->level > randint1(100))
				{
					if (is_original_ap_and_seen(attacker_ptr, m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
					msg_format(_("%^sは抵抗力を持っている！", "%^s resists!"), m_name);
					resists_tele = TRUE;
				}
			}

			if (!resists_tele)
			{
				msg_format(_("%^sは消えた！", "%^s disappears!"), m_name);
				teleport_away(attacker_ptr, g_ptr->m_idx, 50, TELEPORT_PASSIVE);
				num = num_blow + 1; /* Can't hit it anymore! */
				*mdeath = TRUE;
			}
		}

		else if ((chaos_effect == 5) && (randint1(90) > r_ptr->level))
		{
			if (!(r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) &&
				!(r_ptr->flagsr & RFR_EFF_RES_CHAO_MASK))
			{
				if (polymorph_monster(attacker_ptr, y, x))
				{
					msg_format(_("%^sは変化した！", "%^s changes!"), m_name);
					*fear = FALSE;
					weak = FALSE;
				}
				else
				{
					msg_format(_("%^sには効果がなかった。", "%^s is unaffected."), m_name);
				}

				/* Hack -- Get new monster */
				m_ptr = &floor_ptr->m_list[g_ptr->m_idx];

				/* Oops, we need a different name... */
				monster_desc(attacker_ptr, m_name, m_ptr, 0);

				/* Hack -- Get new race */
				r_ptr = &r_info[m_ptr->r_idx];
			}
		}
		else if (o_ptr->name1 == ART_G_HAMMER)
		{
			monster_type *target_ptr = &floor_ptr->m_list[g_ptr->m_idx];

			if (target_ptr->hold_o_idx)
			{
				object_type *q_ptr = &floor_ptr->o_list[target_ptr->hold_o_idx];
				GAME_TEXT o_name[MAX_NLEN];

				object_desc(attacker_ptr, o_name, q_ptr, OD_NAME_ONLY);
				q_ptr->held_m_idx = 0;
				q_ptr->marked = OM_TOUCHED;
				target_ptr->hold_o_idx = q_ptr->next_o_idx;
				q_ptr->next_o_idx = 0;
				msg_format(_("%sを奪った。", "You snatched %s."), o_name);
				inven_carry(attacker_ptr, q_ptr);
			}
		}

		backstab = FALSE;
		fuiuchi = FALSE;
	}

	if (weak && !(*mdeath))
	{
		msg_format(_("%sは弱くなったようだ。", "%^s seems weakened."), m_name);
	}

	if ((drain_left != MAX_VAMPIRIC_DRAIN) && one_in_(4))
	{
		chg_virtue(attacker_ptr, V_UNLIFE, 1);
	}

	/* Mega-Hac
	k -- apply earthquake brand */
	if (do_quake)
	{
		earthquake(attacker_ptr, attacker_ptr->y, attacker_ptr->x, 10, 0);
		if (!floor_ptr->grid_array[y][x].m_idx) *mdeath = TRUE;
	}
}


/*!
* @brief プレイヤーの打撃処理メインルーチン
* @param y 攻撃目標のY座標
* @param x 攻撃目標のX座標
* @param mode 発動中の剣術ID
* @return 実際に攻撃処理が行われた場合TRUEを返す。
* @details
* If no "weapon" is available, then "punch" the monster one time.
*/
bool py_attack(player_type *attacker_ptr, POSITION y, POSITION x, COMBAT_OPTION_IDX mode)
{
	grid_type       *g_ptr = &attacker_ptr->current_floor_ptr->grid_array[y][x];
	monster_type    *m_ptr = &attacker_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
	monster_race    *r_ptr = &r_info[m_ptr->r_idx];
	GAME_TEXT m_name[MAX_NLEN];

	disturb(attacker_ptr, FALSE, TRUE);

	take_turn(attacker_ptr, 100);

	if (!attacker_ptr->migite && !attacker_ptr->hidarite &&
		!(attacker_ptr->muta2 & (MUT2_HORNS | MUT2_BEAK | MUT2_SCOR_TAIL | MUT2_TRUNK | MUT2_TENTACLES)))
	{
		msg_format(_("%s攻撃できない。", "You cannot do attacking."),
			(empty_hands(attacker_ptr, FALSE) == EMPTY_HAND_NONE) ? _("両手がふさがって", "") : "");
		return FALSE;
	}

	monster_desc(attacker_ptr, m_name, m_ptr, 0);

	if (m_ptr->ml)
	{
		/* Auto-Recall if possible and visible */
		if (!attacker_ptr->image) monster_race_track(attacker_ptr, m_ptr->ap_r_idx);

		health_track(attacker_ptr, g_ptr->m_idx);
	}

	if ((r_ptr->flags1 & RF1_FEMALE) &&
		!(attacker_ptr->stun || attacker_ptr->confused || attacker_ptr->image || !m_ptr->ml))
	{
		if ((attacker_ptr->inventory_list[INVEN_RARM].name1 == ART_ZANTETSU) || (attacker_ptr->inventory_list[INVEN_LARM].name1 == ART_ZANTETSU))
		{
			msg_print(_("拙者、おなごは斬れぬ！", "I can not attack women!"));
			return FALSE;
		}
	}

	if (d_info[attacker_ptr->dungeon_idx].flags1 & DF1_NO_MELEE)
	{
		msg_print(_("なぜか攻撃することができない。", "Something prevents you from attacking."));
		return FALSE;
	}

	/* Stop if friendly */
	bool stormbringer = FALSE;
	if (!is_hostile(m_ptr) &&
		!(attacker_ptr->stun || attacker_ptr->confused || attacker_ptr->image ||
			attacker_ptr->shero || !m_ptr->ml))
	{
		if (attacker_ptr->inventory_list[INVEN_RARM].name1 == ART_STORMBRINGER) stormbringer = TRUE;
		if (attacker_ptr->inventory_list[INVEN_LARM].name1 == ART_STORMBRINGER) stormbringer = TRUE;
		if (stormbringer)
		{
			msg_format(_("黒い刃は強欲に%sを攻撃した！", "Your black blade greedily attacks %s!"), m_name);
			chg_virtue(attacker_ptr, V_INDIVIDUALISM, 1);
			chg_virtue(attacker_ptr, V_HONOUR, -1);
			chg_virtue(attacker_ptr, V_JUSTICE, -1);
			chg_virtue(attacker_ptr, V_COMPASSION, -1);
		}
		else if (attacker_ptr->pclass != CLASS_BERSERKER)
		{
			if (get_check(_("本当に攻撃しますか？", "Really hit it? ")))
			{
				chg_virtue(attacker_ptr, V_INDIVIDUALISM, 1);
				chg_virtue(attacker_ptr, V_HONOUR, -1);
				chg_virtue(attacker_ptr, V_JUSTICE, -1);
				chg_virtue(attacker_ptr, V_COMPASSION, -1);
			}
			else
			{
				msg_format(_("%sを攻撃するのを止めた。", "You stop to avoid hitting %s."), m_name);
				return FALSE;
			}
		}
	}

	/* Handle player fear */
	if (attacker_ptr->afraid)
	{
		if (m_ptr->ml)
			msg_format(_("恐くて%sを攻撃できない！", "You are too afraid to attack %s!"), m_name);
		else
			msg_format(_("そっちには何か恐いものがいる！", "There is something scary in your way!"));

		/* Disturb the monster */
		(void)set_monster_csleep(attacker_ptr, g_ptr->m_idx, 0);

		return FALSE;
	}

	if (MON_CSLEEP(m_ptr)) /* It is not honorable etc to attack helpless victims */
	{
		if (!(r_ptr->flags3 & RF3_EVIL) || one_in_(5)) chg_virtue(attacker_ptr, V_COMPASSION, -1);
		if (!(r_ptr->flags3 & RF3_EVIL) || one_in_(5)) chg_virtue(attacker_ptr, V_HONOUR, -1);
	}

	if (attacker_ptr->migite && attacker_ptr->hidarite)
	{
		if ((attacker_ptr->skill_exp[GINOU_NITOURYU] < s_info[attacker_ptr->pclass].s_max[GINOU_NITOURYU]) && ((attacker_ptr->skill_exp[GINOU_NITOURYU] - 1000) / 200 < r_ptr->level))
		{
			if (attacker_ptr->skill_exp[GINOU_NITOURYU] < WEAPON_EXP_BEGINNER)
				attacker_ptr->skill_exp[GINOU_NITOURYU] += 80;
			else if (attacker_ptr->skill_exp[GINOU_NITOURYU] < WEAPON_EXP_SKILLED)
				attacker_ptr->skill_exp[GINOU_NITOURYU] += 4;
			else if (attacker_ptr->skill_exp[GINOU_NITOURYU] < WEAPON_EXP_EXPERT)
				attacker_ptr->skill_exp[GINOU_NITOURYU] += 1;
			else if (attacker_ptr->skill_exp[GINOU_NITOURYU] < WEAPON_EXP_MASTER)
				if (one_in_(3)) attacker_ptr->skill_exp[GINOU_NITOURYU] += 1;
			attacker_ptr->update |= (PU_BONUS);
		}
	}

	/* Gain riding experience */
	if (attacker_ptr->riding)
	{
		int cur = attacker_ptr->skill_exp[GINOU_RIDING];
		int max = s_info[attacker_ptr->pclass].s_max[GINOU_RIDING];

		if (cur < max)
		{
			DEPTH ridinglevel = r_info[attacker_ptr->current_floor_ptr->m_list[attacker_ptr->riding].r_idx].level;
			DEPTH targetlevel = r_ptr->level;
			int inc = 0;

			if ((cur / 200 - 5) < targetlevel)
				inc += 1;

			/* Extra experience */
			if ((cur / 100) < ridinglevel)
			{
				if ((cur / 100 + 15) < ridinglevel)
					inc += 1 + (ridinglevel - (cur / 100 + 15));
				else
					inc += 1;
			}

			attacker_ptr->skill_exp[GINOU_RIDING] = MIN(max, cur + inc);
			attacker_ptr->update |= (PU_BONUS);
		}
	}

	attacker_ptr->riding_t_m_idx = g_ptr->m_idx;
	bool fear = FALSE;
	bool mdeath = FALSE;
	if (attacker_ptr->migite) py_attack_aux(attacker_ptr, y, x, &fear, &mdeath, 0, mode);
	if (attacker_ptr->hidarite && !mdeath) py_attack_aux(attacker_ptr, y, x, &fear, &mdeath, 1, mode);

	/* Mutations which yield extra 'natural' attacks */
	if (!mdeath)
	{
		if ((attacker_ptr->muta2 & MUT2_HORNS) && !mdeath)
			natural_attack(attacker_ptr, g_ptr->m_idx, MUT2_HORNS, &fear, &mdeath);
		if ((attacker_ptr->muta2 & MUT2_BEAK) && !mdeath)
			natural_attack(attacker_ptr, g_ptr->m_idx, MUT2_BEAK, &fear, &mdeath);
		if ((attacker_ptr->muta2 & MUT2_SCOR_TAIL) && !mdeath)
			natural_attack(attacker_ptr, g_ptr->m_idx, MUT2_SCOR_TAIL, &fear, &mdeath);
		if ((attacker_ptr->muta2 & MUT2_TRUNK) && !mdeath)
			natural_attack(attacker_ptr, g_ptr->m_idx, MUT2_TRUNK, &fear, &mdeath);
		if ((attacker_ptr->muta2 & MUT2_TENTACLES) && !mdeath)
			natural_attack(attacker_ptr, g_ptr->m_idx, MUT2_TENTACLES, &fear, &mdeath);
	}

	/* Hack -- delay fear messages */
	if (fear && m_ptr->ml && !mdeath)
	{
		sound(SOUND_FLEE);

		msg_format(_("%^sは恐怖して逃げ出した！", "%^s flees in terror!"), m_name);
	}

	if ((attacker_ptr->special_defense & KATA_IAI) && ((mode != HISSATSU_IAI) || mdeath))
	{
		set_action(attacker_ptr, ACTION_NONE);
	}

	return mdeath;
}


/*!
 * @brief モンスターからプレイヤーへの打撃処理 / Attack the player via physical attacks.
 * @param m_idx 打撃を行うモンスターのID
 * @return 実際に攻撃処理を行った場合TRUEを返す
 */
bool make_attack_normal(player_type *target_ptr, MONSTER_IDX m_idx)
{
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *m_ptr = &floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	int k, tmp;
	ARMOUR_CLASS ac;
	DEPTH rlev;
	int do_cut, do_stun;

	PRICE gold;
	object_type *o_ptr;
	GAME_TEXT o_name[MAX_NLEN];
	GAME_TEXT m_name[MAX_NLEN];
	GAME_TEXT ddesc[80];

	bool blinked;
	bool touched = FALSE, fear = FALSE, alive = TRUE;
	bool explode = FALSE;
	bool do_silly_attack = (one_in_(2) && target_ptr->image);
	HIT_POINT get_damage = 0;
	int abbreviate = 0;	// ２回目以降の省略表現フラグ

	/* Not allowed to attack */
	if (r_ptr->flags1 & (RF1_NEVER_BLOW)) return FALSE;

	if (d_info[target_ptr->dungeon_idx].flags1 & DF1_NO_MELEE) return FALSE;

	/* ...nor if friendly */
	if (!is_hostile(m_ptr)) return FALSE;

	/* Extract the effective monster level */
	rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);

	/* Get the monster name (or "it") */
	monster_desc(target_ptr, m_name, m_ptr, 0);

	monster_desc(target_ptr, ddesc, m_ptr, MD_WRONGDOER_NAME);

	if (target_ptr->special_defense & KATA_IAI)
	{
		msg_format(_("相手が襲いかかる前に素早く武器を振るった。", "You took sen, drew and cut in one motion before %s moved."), m_name);
		if (py_attack(target_ptr, m_ptr->fy, m_ptr->fx, HISSATSU_IAI)) return TRUE;
	}

	if ((target_ptr->special_defense & NINJA_KAWARIMI) && (randint0(55) < (target_ptr->lev*3/5+20)))
	{
		if (kawarimi(target_ptr, TRUE)) return TRUE;
	}

	/* Assume no blink */
	blinked = FALSE;

	/* Scan through all four blows */
	for (int ap_cnt = 0; ap_cnt < 4; ap_cnt++)
	{
		bool obvious = FALSE;

		HIT_POINT power = 0;
		HIT_POINT damage = 0;

		concptr act = NULL;

		/* Extract the attack infomation */
		int effect = r_ptr->blow[ap_cnt].effect;
		int method = r_ptr->blow[ap_cnt].method;
		int d_dice = r_ptr->blow[ap_cnt].d_dice;
		int d_side = r_ptr->blow[ap_cnt].d_side;

		if (!monster_is_valid(m_ptr)) break;

		/* Hack -- no more attacks */
		if (!method) break;

		if (is_pet(m_ptr) && (r_ptr->flags1 & RF1_UNIQUE) && (method == RBM_EXPLODE))
		{
			method = RBM_HIT;
			d_dice /= 10;
		}

		/* Stop if player is dead or gone */
		if (!target_ptr->playing || target_ptr->is_dead) break;
		if (distance(target_ptr->y, target_ptr->x, m_ptr->fy, m_ptr->fx) > 1) break;

		/* Handle "leaving" */
		if (target_ptr->leaving) break;

		if (method == RBM_SHOOT) continue;

		/* Extract the attack "power" */
		power = mbe_info[effect].power;

		/* Total armor */
		ac = target_ptr->ac + target_ptr->to_a;

		/* Monster hits player */
		if (!effect || check_hit(target_ptr, power, rlev, MON_STUNNED(m_ptr)))
		{
			/* Always disturbing */
			disturb(target_ptr, TRUE, TRUE);


			/* Hack -- Apply "protection from evil" */
			if ((target_ptr->protevil > 0) &&
			    (r_ptr->flags3 & RF3_EVIL) &&
			    (target_ptr->lev >= rlev) &&
			    ((randint0(100) + target_ptr->lev) > 50))
			{
				/* Remember the Evil-ness */
				if (is_original_ap_and_seen(target_ptr, m_ptr)) r_ptr->r_flags3 |= RF3_EVIL;

#ifdef JP
				if (abbreviate) msg_format("撃退した。");
				else msg_format("%^sは撃退された。", m_name);
				abbreviate = 1; /*２回目以降は省略 */
#else
				msg_format("%^s is repelled.", m_name);
#endif

				continue;
			}

			/* Assume no cut or stun */
			do_cut = do_stun = 0;

			/* Describe the attack method */
			switch (method)
			{
				case RBM_HIT:
				{
					act = _("殴られた。", "hits you.");
					do_cut = do_stun = 1;
					touched = TRUE;
					sound(SOUND_HIT);
					break;
				}

				case RBM_TOUCH:
				{
					act = _("触られた。", "touches you.");
					touched = TRUE;
					sound(SOUND_TOUCH);
					break;
				}

				case RBM_PUNCH:
				{
					act = _("パンチされた。", "punches you.");
					touched = TRUE;
					do_stun = 1;
					sound(SOUND_HIT);
					break;
				}

				case RBM_KICK:
				{
					act = _("蹴られた。", "kicks you.");
					touched = TRUE;
					do_stun = 1;
					sound(SOUND_HIT);
					break;
				}

				case RBM_CLAW:
				{
					act = _("ひっかかれた。", "claws you.");
					touched = TRUE;
					do_cut = 1;
					sound(SOUND_CLAW);
					break;
				}

				case RBM_BITE:
				{
					act = _("噛まれた。", "bites you.");
					do_cut = 1;
					touched = TRUE;
					sound(SOUND_BITE);
					break;
				}

				case RBM_STING:
				{
					act = _("刺された。", "stings you.");
					touched = TRUE;
					sound(SOUND_STING);
					break;
				}

				case RBM_SLASH:
				{
					act = _("斬られた。", "slashes you.");
					touched = TRUE;
					do_cut = 1;
					sound(SOUND_CLAW);
					break;
				}

				case RBM_BUTT:
				{
					act = _("角で突かれた。", "butts you.");
					do_stun = 1;
					touched = TRUE;
					sound(SOUND_HIT);
					break;
				}

				case RBM_CRUSH:
				{
					act = _("体当たりされた。", "crushes you.");
					do_stun = 1;
					touched = TRUE;
					sound(SOUND_CRUSH);
					break;
				}

				case RBM_ENGULF:
				{
					act = _("飲み込まれた。", "engulfs you.");
					touched = TRUE;
					sound(SOUND_CRUSH);
					break;
				}

				case RBM_CHARGE:
				{
					abbreviate = -1;
					act = _("は請求書をよこした。", "charges you.");
					touched = TRUE;
					sound(SOUND_BUY); /* Note! This is "charges", not "charges at". */
					break;
				}

				case RBM_CRAWL:
				{
					abbreviate = -1;
					act = _("が体の上を這い回った。", "crawls on you.");
					touched = TRUE;
					sound(SOUND_SLIME);
					break;
				}

				case RBM_DROOL:
				{
					act = _("よだれをたらされた。", "drools on you.");
					sound(SOUND_SLIME);
					break;
				}

				case RBM_SPIT:
				{
					act = _("唾を吐かれた。", "spits on you.");
					sound(SOUND_SLIME);
					break;
				}

				case RBM_EXPLODE:
				{
					abbreviate = -1;
					act = _("は爆発した。", "explodes.");
					explode = TRUE;
					break;
				}

				case RBM_GAZE:
				{
					act = _("にらまれた。", "gazes at you.");
					break;
				}

				case RBM_WAIL:
				{
					act = _("泣き叫ばれた。", "wails at you.");
					sound(SOUND_WAIL);
					break;
				}

				case RBM_SPORE:
				{
					act = _("胞子を飛ばされた。", "releases spores at you.");
					sound(SOUND_SLIME);
					break;
				}

				case RBM_XXX4:
				{
					abbreviate = -1;
					act = _("が XXX4 を発射した。", "projects XXX4's at you.");
					break;
				}

				case RBM_BEG:
				{
					act = _("金をせがまれた。", "begs you for money.");
					sound(SOUND_MOAN);
					break;
				}

				case RBM_INSULT:
				{
#ifdef JP
					abbreviate = -1;
#endif
					act = desc_insult[randint0(m_ptr->r_idx == MON_DEBBY ? 10 : 8)];
					sound(SOUND_MOAN);
					break;
				}

				case RBM_MOAN:
				{
#ifdef JP
					abbreviate = -1;
#endif
					act = desc_moan[randint0(4)];
					sound(SOUND_MOAN);
					break;
				}

				case RBM_SHOW:
				{
#ifdef JP
					abbreviate = -1;
#endif
					if (m_ptr->r_idx == MON_JAIAN)
					{
#ifdef JP
						switch(randint1(15))
						{
						  case 1:
						  case 6:
						  case 11:
							act = "「♪お～れはジャイアン～～ガ～キだいしょう～」";
							break;
						  case 2:
							act = "「♪て～んかむ～てきのお～とこだぜ～～」";
							break;
						  case 3:
							act = "「♪の～び太スネ夫はメじゃないよ～～」";
							break;
						  case 4:
							act = "「♪け～んかスポ～ツ～どんとこい～」";
							break;
						  case 5:
							act = "「♪うた～も～～う～まいぜ～まかしとけ～」";
							break;
						  case 7:
							act = "「♪ま～ちいちば～んのに～んきもの～～」";
							break;
						  case 8:
							act = "「♪べんきょうしゅくだいメじゃないよ～～」";
							break;
						  case 9:
							act = "「♪きはやさし～くて～ち～からもち～」";
							break;
						  case 10:
							act = "「♪かお～も～～スタイルも～バツグンさ～」";
							break;
						  case 12:
							act = "「♪がっこうい～ちの～あ～ばれんぼう～～」";
							break;
						  case 13:
							act = "「♪ド～ラもドラミもメじゃないよ～～」";
							break;
						  case 14:
							act = "「♪よじげんぽけっと～な～くたって～」";
							break;
						  case 15:
							act = "「♪あし～の～～ながさ～は～まけないぜ～」";
							break;
						}
#else
						act = "horribly sings 'I AM GIAAAAAN. THE BOOOSS OF THE KIIIIDS.'";
#endif
					}
					else
					{
						if (one_in_(3))
							act = _("は♪僕らは楽しい家族♪と歌っている。", "sings 'We are a happy family.'");
						else
							act = _("は♪アイ ラブ ユー、ユー ラブ ミー♪と歌っている。", "sings 'I love you, you love me.'");
					}

					sound(SOUND_SHOW);
					break;
				}
			}

			if (act)
			{
				if (do_silly_attack)
				{
#ifdef JP
					abbreviate = -1;
#endif
					act = silly_attacks[randint0(MAX_SILLY_ATTACK)];
				}
#ifdef JP
				if (abbreviate == 0)
					msg_format("%^sに%s", m_name, act);
				else if (abbreviate == 1)
					msg_format("%s", act);
				else /* if (abbreviate == -1) */
					msg_format("%^s%s", m_name, act);
				abbreviate = 1;/*２回目以降は省略 */
#else
				msg_format("%^s %s%s", m_name, act, do_silly_attack ? " you." : "");
#endif
			}

			/* Hack -- assume all attacks are obvious */
			obvious = TRUE;

			/* Roll out the damage */
			damage = damroll(d_dice, d_side);

			/*
			 * Skip the effect when exploding, since the explosion
			 * already causes the effect.
			 */
			if(explode) damage = 0;
			/* Apply appropriate damage */
			switch (effect)
			{
				case 0:
				{
					obvious = TRUE;
					damage = 0;
					break;
				}

				case RBE_SUPERHURT:	/* AC軽減あり / Player armor reduces total damage */
				{
					if (((randint1(rlev*2+300) > (ac+200)) || one_in_(13)) && !CHECK_MULTISHADOW(target_ptr))
					{
						int tmp_damage = damage - (damage * ((ac < 150) ? ac : 150) / 250);
						msg_print(_("痛恨の一撃！", "It was a critical hit!"));
						tmp_damage = MAX(damage, tmp_damage*2);

						get_damage += take_hit(target_ptr, DAMAGE_ATTACK, tmp_damage, ddesc, -1);
						break;
					}
				}
					/* Fall through */
				case RBE_HURT: /* AC軽減あり / Player armor reduces total damage */
				{
					obvious = TRUE;
					damage -= (damage * ((ac < 150) ? ac : 150) / 250);
					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
					break;
				}

				case RBE_POISON:
				{
					if (explode) break;

					/* Take "poison" effect */
					if (!(target_ptr->resist_pois || is_oppose_pois(target_ptr)) && !CHECK_MULTISHADOW(target_ptr))
					{
						if (set_poisoned(target_ptr, target_ptr->poisoned + randint1(rlev) + 5))
						{
							obvious = TRUE;
						}
					}

					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					/* Learn about the player */
					update_smart_learn(target_ptr, m_idx, DRS_POIS);

					break;
				}

				case RBE_UN_BONUS:
				{
					if (explode) break;

					/* Allow complete resist */
					if (!target_ptr->resist_disen && !CHECK_MULTISHADOW(target_ptr))
					{
						/* Apply disenchantment */
						if (apply_disenchant(target_ptr, 0))
						{
							/* Hack -- Update AC */
							update_creature(target_ptr);
							obvious = TRUE;
						}
					}

					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					/* Learn about the player */
					update_smart_learn(target_ptr, m_idx, DRS_DISEN);

					break;
				}

				case RBE_UN_POWER:
				{
					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					if (target_ptr->is_dead || CHECK_MULTISHADOW(target_ptr)) break;

					/* Find an item */
					for (k = 0; k < 10; k++)
					{
						/* Pick an item */
						INVENTORY_IDX i = (INVENTORY_IDX)randint0(INVEN_PACK);

						/* Obtain the item */
						o_ptr = &target_ptr->inventory_list[i];
						if (!o_ptr->k_idx) continue;

						/* Drain charged wands/staffs */
						if (((o_ptr->tval == TV_STAFF) ||
						     (o_ptr->tval == TV_WAND)) &&
						    (o_ptr->pval))
						{
							/* Calculate healed hitpoints */
							int heal=rlev * o_ptr->pval;
							if( o_ptr->tval == TV_STAFF)
							    heal *=  o_ptr->number;

							/* Don't heal more than max hp */
							heal = MIN(heal, m_ptr->maxhp - m_ptr->hp);

							msg_print(_("ザックからエネルギーが吸い取られた！", "Energy drains from your pack!"));

							obvious = TRUE;

							/* Heal the monster */
							m_ptr->hp += (HIT_POINT)heal;

							/* Redraw (later) if needed */
							if (target_ptr->health_who == m_idx) target_ptr->redraw |= (PR_HEALTH);
							if (target_ptr->riding == m_idx) target_ptr->redraw |= (PR_UHEALTH);

							/* Uncharge */
							o_ptr->pval = 0;

							/* Combine / Reorder the pack */
							target_ptr->update |= (PU_COMBINE | PU_REORDER);
							target_ptr->window |= (PW_INVEN);

							break;
						}
					}

					break;
				}

				case RBE_EAT_GOLD:
				{
					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					/* Confused monsters cannot steal successfully. -LM-*/
					if (MON_CONFUSED(m_ptr)) break;

					if (target_ptr->is_dead || CHECK_MULTISHADOW(target_ptr)) break;

					obvious = TRUE;

					/* Saving throw (unless paralyzed) based on dex and level */
					if (!target_ptr->paralyzed &&
					    (randint0(100) < (adj_dex_safe[target_ptr->stat_ind[A_DEX]] +
							      target_ptr->lev)))
					{
						/* Saving throw message */
						msg_print(_("しかし素早く財布を守った！", "You quickly protect your money pouch!"));

						/* Occasional blink anyway */
						if (randint0(3)) blinked = TRUE;
					}

					/* Eat gold */
					else
					{
						gold = (target_ptr->au / 10) + randint1(25);
						if (gold < 2) gold = 2;
						if (gold > 5000) gold = (target_ptr->au / 20) + randint1(3000);
						if (gold > target_ptr->au) gold = target_ptr->au;
						target_ptr->au -= gold;
						if (gold <= 0)
						{
							msg_print(_("しかし何も盗まれなかった。", "Nothing was stolen."));
						}
						else if (target_ptr->au)
						{
							msg_print(_("財布が軽くなった気がする。", "Your purse feels lighter."));
							msg_format(_("$%ld のお金が盗まれた！", "%ld coins were stolen!"), (long)gold);
							chg_virtue(target_ptr, V_SACRIFICE, 1);
						}
						else
						{
							msg_print(_("財布が軽くなった気がする。", "Your purse feels lighter."));
							msg_print(_("お金が全部盗まれた！", "All of your coins were stolen!"));
							chg_virtue(target_ptr, V_SACRIFICE, 2);
						}

						/* Redraw gold */
						target_ptr->redraw |= (PR_GOLD);

						target_ptr->window |= (PW_PLAYER);

						/* Blink away */
						blinked = TRUE;
					}

					break;
				}

				case RBE_EAT_ITEM:
				{
					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					/* Confused monsters cannot steal successfully. -LM-*/
					if (MON_CONFUSED(m_ptr)) break;

					if (target_ptr->is_dead || CHECK_MULTISHADOW(target_ptr)) break;

					/* Saving throw (unless paralyzed) based on dex and level */
					if (!target_ptr->paralyzed && (randint0(100) < (adj_dex_safe[target_ptr->stat_ind[A_DEX]] + target_ptr->lev)))
					{
						/* Saving throw message */
						msg_print(_("しかしあわててザックを取り返した！", "You grab hold of your backpack!"));

						/* Occasional "blink" anyway */
						blinked = TRUE;
						obvious = TRUE;
						break;
					}

					/* Find an item */
					for (k = 0; k < 10; k++)
					{
						OBJECT_IDX o_idx;

						/* Pick an item */
						INVENTORY_IDX i = (INVENTORY_IDX)randint0(INVEN_PACK);

						/* Obtain the item */
						o_ptr = &target_ptr->inventory_list[i];
						if (!o_ptr->k_idx) continue;

						/* Skip artifacts */
						if (object_is_artifact(o_ptr)) continue;

						object_desc(target_ptr, o_name, o_ptr, OD_OMIT_PREFIX);

#ifdef JP
						msg_format("%s(%c)を%s盗まれた！", o_name, index_to_label(i), ((o_ptr->number > 1) ? "一つ" : ""));
#else
						msg_format("%sour %s (%c) was stolen!", ((o_ptr->number > 1) ? "One of y" : "Y"), o_name, index_to_label(i));
#endif
						chg_virtue(target_ptr, V_SACRIFICE, 1);
						o_idx = o_pop(floor_ptr);

						/* Success */
						if (o_idx)
						{
							object_type *j_ptr;
							j_ptr = &floor_ptr->o_list[o_idx];
							object_copy(j_ptr, o_ptr);

							/* Modify number */
							j_ptr->number = 1;

							/* Hack -- If a rod or wand, allocate total
							 * maximum timeouts or charges between those
							 * stolen and those missed. -LM-
							 */
							if ((o_ptr->tval == TV_ROD) || (o_ptr->tval == TV_WAND))
							{
								j_ptr->pval = o_ptr->pval / o_ptr->number;
								o_ptr->pval -= j_ptr->pval;
							}

							/* Forget mark */
							j_ptr->marked = OM_TOUCHED;

							/* Memorize monster */
							j_ptr->held_m_idx = m_idx;

							/* Build stack */
							j_ptr->next_o_idx = m_ptr->hold_o_idx;

							/* Build stack */
							m_ptr->hold_o_idx = o_idx;
						}

						/* Steal the items */
						inven_item_increase(target_ptr, i, -1);
						inven_item_optimize(target_ptr, i);

						obvious = TRUE;

						/* Blink away */
						blinked = TRUE;

						break;
					}

					break;
				}

				case RBE_EAT_FOOD:
				{
					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					if (target_ptr->is_dead || CHECK_MULTISHADOW(target_ptr)) break;

					/* Steal some food */
					for (k = 0; k < 10; k++)
					{
						/* Pick an item from the pack */
						INVENTORY_IDX i = (INVENTORY_IDX)randint0(INVEN_PACK);

						o_ptr = &target_ptr->inventory_list[i];
						if (!o_ptr->k_idx) continue;

						/* Skip non-food objects */
						if ((o_ptr->tval != TV_FOOD) && !((o_ptr->tval == TV_CORPSE) && (o_ptr->sval))) continue;

						object_desc(target_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

#ifdef JP
						msg_format("%s(%c)を%s食べられてしまった！", o_name, index_to_label(i), ((o_ptr->number > 1) ? "一つ" : ""));
#else
						msg_format("%sour %s (%c) was eaten!", ((o_ptr->number > 1) ? "One of y" : "Y"), o_name, index_to_label(i));
#endif

						/* Steal the items */
						inven_item_increase(target_ptr, i, -1);
						inven_item_optimize(target_ptr, i);

						obvious = TRUE;

						break;
					}

					break;
				}

				case RBE_EAT_LITE:
				{
					/* Access the lite */
					o_ptr = &target_ptr->inventory_list[INVEN_LITE];
					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					if (target_ptr->is_dead || CHECK_MULTISHADOW(target_ptr)) break;

					/* Drain fuel */
					if ((o_ptr->xtra4 > 0) && (!object_is_fixed_artifact(o_ptr)))
					{
						/* Reduce fuel */
						o_ptr->xtra4 -= (s16b)(250 + randint1(250));
						if (o_ptr->xtra4 < 1) o_ptr->xtra4 = 1;

						if (!target_ptr->blind)
						{
							msg_print(_("明かりが暗くなってしまった。", "Your light dims."));
							obvious = TRUE;
						}

						target_ptr->window |= (PW_EQUIP);
					}

					break;
				}

				case RBE_ACID:
				{
					if (explode) break;
					obvious = TRUE;
					msg_print(_("酸を浴びせられた！", "You are covered in acid!"));
					get_damage += acid_dam(target_ptr, damage, ddesc, -1, FALSE);
					update_creature(target_ptr);
					update_smart_learn(target_ptr, m_idx, DRS_ACID);
					break;
				}

				case RBE_ELEC:
				{
					if (explode) break;
					obvious = TRUE;
					msg_print(_("電撃を浴びせられた！", "You are struck by electricity!"));
					get_damage += elec_dam(target_ptr, damage, ddesc, -1, FALSE);
					update_smart_learn(target_ptr, m_idx, DRS_ELEC);
					break;
				}

				case RBE_FIRE:
				{
					if (explode) break;
					obvious = TRUE;
					msg_print(_("全身が炎に包まれた！", "You are enveloped in flames!"));
					get_damage += fire_dam(target_ptr, damage, ddesc, -1, FALSE);
					update_smart_learn(target_ptr, m_idx, DRS_FIRE);
					break;
				}

				case RBE_COLD:
				{
					if (explode) break;
					obvious = TRUE;
					msg_print(_("全身が冷気で覆われた！", "You are covered with frost!"));
					get_damage += cold_dam(target_ptr, damage, ddesc, -1, FALSE);
					update_smart_learn(target_ptr, m_idx, DRS_COLD);
					break;
				}

				case RBE_BLIND:
				{
					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);
					if (target_ptr->is_dead) break;

					/* Increase "blind" */
					if (!target_ptr->resist_blind && !CHECK_MULTISHADOW(target_ptr))
					{
						if (set_blind(target_ptr, target_ptr->blind + 10 + randint1(rlev)))
						{
#ifdef JP
							if (m_ptr->r_idx == MON_DIO) msg_print("どうだッ！この血の目潰しはッ！");
#else
							/* nanka */
#endif
							obvious = TRUE;
						}
					}

					/* Learn about the player */
					update_smart_learn(target_ptr, m_idx, DRS_BLIND);

					break;
				}

				case RBE_CONFUSE:
				{
					if (explode) break;
					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					if (target_ptr->is_dead) break;

					/* Increase "confused" */
					if (!target_ptr->resist_conf && !CHECK_MULTISHADOW(target_ptr))
					{
						if (set_confused(target_ptr, target_ptr->confused + 3 + randint1(rlev)))
						{
							obvious = TRUE;
						}
					}

					/* Learn about the player */
					update_smart_learn(target_ptr, m_idx, DRS_CONF);

					break;
				}

				case RBE_TERRIFY:
				{
					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					if (target_ptr->is_dead) break;

					/* Increase "afraid" */
					if (CHECK_MULTISHADOW(target_ptr))
					{
						/* Do nothing */
					}
					else if (target_ptr->resist_fear)
					{
						msg_print(_("しかし恐怖に侵されなかった！", "You stand your ground!"));
						obvious = TRUE;
					}
					else if (randint0(100 + r_ptr->level/2) < target_ptr->skill_sav)
					{
						msg_print(_("しかし恐怖に侵されなかった！", "You stand your ground!"));
						obvious = TRUE;
					}
					else
					{
						if (set_afraid(target_ptr, target_ptr->afraid + 3 + randint1(rlev)))
						{
							obvious = TRUE;
						}
					}

					/* Learn about the player */
					update_smart_learn(target_ptr, m_idx, DRS_FEAR);

					break;
				}

				case RBE_PARALYZE:
				{
					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					if (target_ptr->is_dead) break;

					/* Increase "paralyzed" */
					if (CHECK_MULTISHADOW(target_ptr))
					{
						/* Do nothing */
					}
					else if (target_ptr->free_act)
					{
						msg_print(_("しかし効果がなかった！", "You are unaffected!"));
						obvious = TRUE;
					}
					else if (randint0(100 + r_ptr->level/2) < target_ptr->skill_sav)
					{
						msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
						obvious = TRUE;
					}
					else
					{
						if (!target_ptr->paralyzed)
						{
							if (set_paralyzed(target_ptr, 3 + randint1(rlev)))
							{
								obvious = TRUE;
							}
						}
					}

					/* Learn about the player */
					update_smart_learn(target_ptr, m_idx, DRS_FREE);

					break;
				}

				case RBE_LOSE_STR:
				{
					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					if (target_ptr->is_dead || CHECK_MULTISHADOW(target_ptr)) break;
					if (do_dec_stat(target_ptr, A_STR)) obvious = TRUE;

					break;
				}

				case RBE_LOSE_INT:
				{
					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					if (target_ptr->is_dead || CHECK_MULTISHADOW(target_ptr)) break;
					if (do_dec_stat(target_ptr, A_INT)) obvious = TRUE;

					break;
				}

				case RBE_LOSE_WIS:
				{
					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					if (target_ptr->is_dead || CHECK_MULTISHADOW(target_ptr)) break;
					if (do_dec_stat(target_ptr, A_WIS)) obvious = TRUE;

					break;
				}

				case RBE_LOSE_DEX:
				{
					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					if (target_ptr->is_dead || CHECK_MULTISHADOW(target_ptr)) break;
					if (do_dec_stat(target_ptr, A_DEX)) obvious = TRUE;

					break;
				}

				case RBE_LOSE_CON:
				{
					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					if (target_ptr->is_dead || CHECK_MULTISHADOW(target_ptr)) break;
					if (do_dec_stat(target_ptr, A_CON)) obvious = TRUE;

					break;
				}

				case RBE_LOSE_CHR:
				{
					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					if (target_ptr->is_dead || CHECK_MULTISHADOW(target_ptr)) break;
					if (do_dec_stat(target_ptr, A_CHR)) obvious = TRUE;

					break;
				}

				case RBE_LOSE_ALL:
				{
					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					if (target_ptr->is_dead || CHECK_MULTISHADOW(target_ptr)) break;

					/* Damage (stats) */
					if (do_dec_stat(target_ptr, A_STR)) obvious = TRUE;
					if (do_dec_stat(target_ptr, A_DEX)) obvious = TRUE;
					if (do_dec_stat(target_ptr, A_CON)) obvious = TRUE;
					if (do_dec_stat(target_ptr, A_INT)) obvious = TRUE;
					if (do_dec_stat(target_ptr, A_WIS)) obvious = TRUE;
					if (do_dec_stat(target_ptr, A_CHR)) obvious = TRUE;

					break;
				}

				case RBE_SHATTER:
				{
					obvious = TRUE;

					/* Hack -- Reduce damage based on the player armor class */
					damage -= (damage * ((ac < 150) ? ac : 150) / 250);

					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					/* Radius 8 earthquake centered at the monster */
					if (damage > 23 || explode)
					{
						earthquake(target_ptr, m_ptr->fy, m_ptr->fx, 8, m_idx);
					}

					break;
				}

				case RBE_EXP_10:
				{
					s32b d = damroll(10, 6) + (target_ptr->exp / 100) * MON_DRAIN_LIFE;

					obvious = TRUE;

					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					if (target_ptr->is_dead || CHECK_MULTISHADOW(target_ptr)) break;

					(void)drain_exp(target_ptr, d, d / 10, 95);
					break;
				}

				case RBE_EXP_20:
				{
					s32b d = damroll(20, 6) + (target_ptr->exp / 100) * MON_DRAIN_LIFE;

					obvious = TRUE;

					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					if (target_ptr->is_dead || CHECK_MULTISHADOW(target_ptr)) break;

					(void)drain_exp(target_ptr, d, d / 10, 90);
					break;
				}

				case RBE_EXP_40:
				{
					s32b d = damroll(40, 6) + (target_ptr->exp / 100) * MON_DRAIN_LIFE;

					obvious = TRUE;

					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					if (target_ptr->is_dead || CHECK_MULTISHADOW(target_ptr)) break;

					(void)drain_exp(target_ptr, d, d / 10, 75);
					break;
				}

				case RBE_EXP_80:
				{
					s32b d = damroll(80, 6) + (target_ptr->exp / 100) * MON_DRAIN_LIFE;

					obvious = TRUE;

					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					if (target_ptr->is_dead || CHECK_MULTISHADOW(target_ptr)) break;

					(void)drain_exp(target_ptr, d, d / 10, 50);
					break;
				}

				case RBE_DISEASE:
				{
					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					if (target_ptr->is_dead || CHECK_MULTISHADOW(target_ptr)) break;

					/* Take "poison" effect */
					if (!(target_ptr->resist_pois || is_oppose_pois(target_ptr)))
					{
						if (set_poisoned(target_ptr, target_ptr->poisoned + randint1(rlev) + 5))
						{
							obvious = TRUE;
						}
					}

					/* Damage CON (10% chance)*/
					if ((randint1(100) < 11) && (target_ptr->prace != RACE_ANDROID))
					{
						/* 1% chance for perm. damage */
						bool perm = one_in_(10);
						if (dec_stat(target_ptr, A_CON, randint1(10), perm))
						{
							msg_print(_("病があなたを蝕んでいる気がする。", "You feel sickly."));
							obvious = TRUE;
						}
					}

					break;
				}
				case RBE_TIME:
				{
					if (explode) break;
					if (!target_ptr->resist_time && !CHECK_MULTISHADOW(target_ptr))
					{
						switch (randint1(10))
						{
							case 1: case 2: case 3: case 4: case 5:
							{
								if (target_ptr->prace == RACE_ANDROID) break;
								msg_print(_("人生が逆戻りした気がする。", "You feel like a chunk of the past has been ripped away."));
								lose_exp(target_ptr, 100 + (target_ptr->exp / 100) * MON_DRAIN_LIFE);
								break;
							}

							case 6: case 7: case 8: case 9:
							{
								int stat = randint0(6);

								switch (stat)
								{
#ifdef JP
									case A_STR: act = "強く"; break;
									case A_INT: act = "聡明で"; break;
									case A_WIS: act = "賢明で"; break;
									case A_DEX: act = "器用で"; break;
									case A_CON: act = "健康で"; break;
									case A_CHR: act = "美しく"; break;
#else
									case A_STR: act = "strong"; break;
									case A_INT: act = "bright"; break;
									case A_WIS: act = "wise"; break;
									case A_DEX: act = "agile"; break;
									case A_CON: act = "hale"; break;
									case A_CHR: act = "beautiful"; break;
#endif

								}

								msg_format(_("あなたは以前ほど%sなくなってしまった...。", "You're not as %s as you used to be..."), act);
								target_ptr->stat_cur[stat] = (target_ptr->stat_cur[stat] * 3) / 4;
								if (target_ptr->stat_cur[stat] < 3) target_ptr->stat_cur[stat] = 3;
								target_ptr->update |= (PU_BONUS);
								break;
							}

							case 10:
							{
								msg_print(_("あなたは以前ほど力強くなくなってしまった...。", "You're not as powerful as you used to be..."));

								for (k = 0; k < A_MAX; k++)
								{
									target_ptr->stat_cur[k] = (target_ptr->stat_cur[k] * 7) / 8;
									if (target_ptr->stat_cur[k] < 3) target_ptr->stat_cur[k] = 3;
								}
								target_ptr->update |= (PU_BONUS);
								break;
							}
						}
					}
					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					break;
				}
				case RBE_DR_LIFE:
				{
					s32b d = damroll(60, 6) + (target_ptr->exp / 100) * MON_DRAIN_LIFE;
					bool resist_drain;

					obvious = TRUE;

					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					if (target_ptr->is_dead || CHECK_MULTISHADOW(target_ptr)) break;

					resist_drain = !drain_exp(target_ptr, d, d / 10, 50);

					/* Heal the attacker? */
					if (target_ptr->mimic_form)
					{
						if (mimic_info[target_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_NONLIVING)
							resist_drain = TRUE;
					}
					else
					{
						switch (target_ptr->prace)
						{
						case RACE_ZOMBIE:
						case RACE_VAMPIRE:
						case RACE_SPECTRE:
						case RACE_SKELETON:
						case RACE_DEMON:
						case RACE_GOLEM:
						case RACE_ANDROID:
							resist_drain = TRUE;
							break;
						}
					}

					if ((damage > 5) && !resist_drain)
					{
						bool did_heal = FALSE;

						if (m_ptr->hp < m_ptr->maxhp) did_heal = TRUE;

						/* Heal */
						m_ptr->hp += damroll(4, damage / 6);
						if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

						/* Redraw (later) if needed */
						if (target_ptr->health_who == m_idx) target_ptr->redraw |= (PR_HEALTH);
						if (target_ptr->riding == m_idx) target_ptr->redraw |= (PR_UHEALTH);

						/* Special message */
						if (m_ptr->ml && did_heal)
						{
							msg_format(_("%sは体力を回復したようだ。", "%^s appears healthier."), m_name);
						}
					}

					break;
				}
				case RBE_DR_MANA:
				{
					obvious = TRUE;

					if (CHECK_MULTISHADOW(target_ptr))
					{
						msg_print(_("攻撃は幻影に命中し、あなたには届かなかった。", "The attack hits Shadow, but you are unharmed!"));
					}
					else
					{
						do_cut = 0;

						target_ptr->csp -= damage;
						if (target_ptr->csp < 0)
						{
							target_ptr->csp = 0;
							target_ptr->csp_frac = 0;
						}

						target_ptr->redraw |= (PR_MANA);
					}

					/* Learn about the player */
					update_smart_learn(target_ptr, m_idx, DRS_MANA);

					break;
				}
				case RBE_INERTIA:
				{
					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					if (target_ptr->is_dead) break;

					/* Decrease speed */
					if (CHECK_MULTISHADOW(target_ptr))
					{
						/* Do nothing */
					}
					else
					{
						if (set_slow(target_ptr, (target_ptr->slow + 4 + randint0(rlev / 10)), FALSE))
						{
							obvious = TRUE;
						}
					}

					break;
				}
				case RBE_STUN:
				{
					get_damage += take_hit(target_ptr, DAMAGE_ATTACK, damage, ddesc, -1);

					if (target_ptr->is_dead) break;

					/* Decrease speed */
					if (target_ptr->resist_sound || CHECK_MULTISHADOW(target_ptr))
					{
						/* Do nothing */
					}
					else
					{
						if (set_stun(target_ptr, target_ptr->stun + 10 + randint1(r_ptr->level / 4)))
						{
							obvious = TRUE;
						}
					}

					break;
				}
			}

			/* Hack -- only one of cut or stun */
			if (do_cut && do_stun)
			{
				/* Cancel cut */
				if (randint0(100) < 50)
				{
					do_cut = 0;
				}

				/* Cancel stun */
				else
				{
					do_stun = 0;
				}
			}

			/* Handle cut */
			if (do_cut)
			{
				int cut_plus = 0;

				/* Critical hit (zero if non-critical) */
				tmp = monster_critical(d_dice, d_side, damage);

				/* Roll for damage */
				switch (tmp)
				{
					case 0: cut_plus = 0; break;
					case 1: cut_plus = randint1(5); break;
					case 2: cut_plus = randint1(5) + 5; break;
					case 3: cut_plus = randint1(20) + 20; break;
					case 4: cut_plus = randint1(50) + 50; break;
					case 5: cut_plus = randint1(100) + 100; break;
					case 6: cut_plus = 300; break;
					default: cut_plus = 500; break;
				}

				/* Apply the cut */
				if (cut_plus) (void)set_cut(target_ptr,target_ptr->cut + cut_plus);
			}

			/* Handle stun */
			if (do_stun)
			{
				int stun_plus = 0;

				/* Critical hit (zero if non-critical) */
				tmp = monster_critical(d_dice, d_side, damage);

				/* Roll for damage */
				switch (tmp)
				{
					case 0: stun_plus = 0; break;
					case 1: stun_plus = randint1(5); break;
					case 2: stun_plus = randint1(5) + 10; break;
					case 3: stun_plus = randint1(10) + 20; break;
					case 4: stun_plus = randint1(15) + 30; break;
					case 5: stun_plus = randint1(20) + 40; break;
					case 6: stun_plus = 80; break;
					default: stun_plus = 150; break;
				}

				/* Apply the stun */
				if (stun_plus) (void)set_stun(target_ptr, target_ptr->stun + stun_plus);
			}

			if (explode)
			{
				sound(SOUND_EXPLODE);

				if (mon_take_hit(target_ptr, m_idx, m_ptr->hp + 1, &fear, NULL))
				{
					blinked = FALSE;
					alive = FALSE;
				}
			}

			if (touched)
			{
				if (target_ptr->sh_fire && alive && !target_ptr->is_dead)
				{
					if (!(r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK))
					{
						HIT_POINT dam = damroll(2, 6);

						/* Modify the damage */
						dam = mon_damage_mod(target_ptr, m_ptr, dam, FALSE);

						msg_format(_("%^sは突然熱くなった！", "%^s is suddenly very hot!"), m_name);

						if (mon_take_hit(target_ptr, m_idx, dam, &fear, _("は灰の山になった。", " turns into a pile of ash.")))
						{
							blinked = FALSE;
							alive = FALSE;
						}
					}
					else
					{
						if (is_original_ap_and_seen(target_ptr, m_ptr))
							r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK);
					}
				}

				if (target_ptr->sh_elec && alive && !target_ptr->is_dead)
				{
					if (!(r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK))
					{
						HIT_POINT dam = damroll(2, 6);

						/* Modify the damage */
						dam = mon_damage_mod(target_ptr, m_ptr, dam, FALSE);

						msg_format(_("%^sは電撃をくらった！", "%^s gets zapped!"), m_name);
						if (mon_take_hit(target_ptr, m_idx, dam, &fear, _("は燃え殻の山になった。", " turns into a pile of cinder.")))
						{
							blinked = FALSE;
							alive = FALSE;
						}
					}
					else
					{
						if (is_original_ap_and_seen(target_ptr, m_ptr))
							r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK);
					}
				}

				if (target_ptr->sh_cold && alive && !target_ptr->is_dead)
				{
					if (!(r_ptr->flagsr & RFR_EFF_IM_COLD_MASK))
					{
						HIT_POINT dam = damroll(2, 6);

						/* Modify the damage */
						dam = mon_damage_mod(target_ptr, m_ptr, dam, FALSE);

						msg_format(_("%^sは冷気をくらった！", "%^s is very cold!"), m_name);
						if (mon_take_hit(target_ptr, m_idx, dam, &fear, _("は凍りついた。", " was frozen.")))
						{
							blinked = FALSE;
							alive = FALSE;
						}
					}
					else
					{
						if (is_original_ap_and_seen(target_ptr, m_ptr))
							r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_COLD_MASK);
					}
				}

				/* by henkma */
				if (target_ptr->dustrobe && alive && !target_ptr->is_dead)
				{
					if (!(r_ptr->flagsr & RFR_EFF_RES_SHAR_MASK))
					{
						HIT_POINT dam = damroll(2, 6);

						/* Modify the damage */
						dam = mon_damage_mod(target_ptr, m_ptr, dam, FALSE);

						msg_format(_("%^sは鏡の破片をくらった！", "%^s gets zapped!"), m_name);
						if (mon_take_hit(target_ptr, m_idx, dam, &fear, _("はズタズタになった。", " had torn to pieces.")))
						{
							blinked = FALSE;
							alive = FALSE;
						}
					}
					else
					{
						if (is_original_ap_and_seen(target_ptr, m_ptr))
							r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_RES_SHAR_MASK);
					}

					if (is_mirror_grid(&floor_ptr->grid_array[target_ptr->y][target_ptr->x]))
					{
						teleport_player(target_ptr, 10, TELEPORT_SPONTANEOUS);
					}
				}

				if (target_ptr->tim_sh_holy && alive && !target_ptr->is_dead)
				{
					if (r_ptr->flags3 & RF3_EVIL)
					{
						if (!(r_ptr->flagsr & RFR_RES_ALL))
						{
							HIT_POINT dam = damroll(2, 6);

							/* Modify the damage */
							dam = mon_damage_mod(target_ptr, m_ptr, dam, FALSE);

							msg_format(_("%^sは聖なるオーラで傷ついた！", "%^s is injured by holy power!"), m_name);
							if (mon_take_hit(target_ptr, m_idx, dam, &fear, _("は倒れた。", " is destroyed.")))
							{
								blinked = FALSE;
								alive = FALSE;
							}
							if (is_original_ap_and_seen(target_ptr, m_ptr))
								r_ptr->r_flags3 |= RF3_EVIL;
						}
						else
						{
							if (is_original_ap_and_seen(target_ptr, m_ptr))
								r_ptr->r_flagsr |= RFR_RES_ALL;
						}
					}
				}

				if (target_ptr->tim_sh_touki && alive && !target_ptr->is_dead)
				{
					if (!(r_ptr->flagsr & RFR_RES_ALL))
					{
						HIT_POINT dam = damroll(2, 6);

						/* Modify the damage */
						dam = mon_damage_mod(target_ptr, m_ptr, dam, FALSE);

						msg_format(_("%^sが鋭い闘気のオーラで傷ついた！", "%^s is injured by the Force"), m_name);
						if (mon_take_hit(target_ptr, m_idx, dam, &fear, _("は倒れた。", " is destroyed.")))
						{
							blinked = FALSE;
							alive = FALSE;
						}
					}
					else
					{
						if (is_original_ap_and_seen(target_ptr, m_ptr))
							r_ptr->r_flagsr |= RFR_RES_ALL;
					}
				}

				if (hex_spelling(target_ptr, HEX_SHADOW_CLOAK) && alive && !target_ptr->is_dead)
				{
					HIT_POINT dam = 1;
					object_type *o_armed_ptr = &target_ptr->inventory_list[INVEN_RARM];

					if (!(r_ptr->flagsr & RFR_RES_ALL || r_ptr->flagsr & RFR_RES_DARK))
					{
						if (o_armed_ptr->k_idx)
						{
							int basedam = ((o_armed_ptr->dd + target_ptr->to_dd[0]) * (o_armed_ptr->ds + target_ptr->to_ds[0] + 1));
							dam = basedam / 2 + o_armed_ptr->to_d + target_ptr->to_d[0];
						}

						/* Cursed armor makes damages doubled */
						o_armed_ptr = &target_ptr->inventory_list[INVEN_BODY];
						if ((o_armed_ptr->k_idx) && object_is_cursed(o_armed_ptr)) dam *= 2;

						/* Modify the damage */
						dam = mon_damage_mod(target_ptr, m_ptr, dam, FALSE);

						msg_format(_("影のオーラが%^sに反撃した！", "Enveloping shadows attack %^s."), m_name);
						if (mon_take_hit(target_ptr, m_idx, dam, &fear, _("は倒れた。", " is destroyed.")))
						{
							blinked = FALSE;
							alive = FALSE;
						}
						else /* monster does not dead */
						{
							int j;
							BIT_FLAGS flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
							EFFECT_ID typ[4][2] = {
								{ INVEN_HEAD, GF_OLD_CONF },
								{ INVEN_LARM,  GF_OLD_SLEEP },
								{ INVEN_HANDS, GF_TURN_ALL },
								{ INVEN_FEET, GF_OLD_SLOW }
							};

							/* Some cursed armours gives an extra effect */
							for (j = 0; j < 4; j++)
							{
								o_armed_ptr = &target_ptr->inventory_list[typ[j][0]];
								if ((o_armed_ptr->k_idx) && object_is_cursed(o_armed_ptr) && object_is_armour(o_armed_ptr))
									project(target_ptr, 0, 0, m_ptr->fy, m_ptr->fx, (target_ptr->lev * 2), typ[j][1], flg, -1);
							}
						}
					}
					else
					{
						if (is_original_ap_and_seen(target_ptr, m_ptr))
							r_ptr->r_flagsr |= (RFR_RES_ALL | RFR_RES_DARK);
					}
				}
			}
		}

		/* Monster missed player */
		else
		{
			/* Analyze failed attacks */
			switch (method)
			{
				case RBM_HIT:
				case RBM_TOUCH:
				case RBM_PUNCH:
				case RBM_KICK:
				case RBM_CLAW:
				case RBM_BITE:
				case RBM_STING:
				case RBM_SLASH:
				case RBM_BUTT:
				case RBM_CRUSH:
				case RBM_ENGULF:
				case RBM_CHARGE:

				/* Visible monsters */
				if (m_ptr->ml)
				{
					disturb(target_ptr, TRUE, TRUE);

#ifdef JP
					if (abbreviate)
					    msg_format("%sかわした。", (target_ptr->special_attack & ATTACK_SUIKEN) ? "奇妙な動きで" : "");
					else
					    msg_format("%s%^sの攻撃をかわした。", (target_ptr->special_attack & ATTACK_SUIKEN) ? "奇妙な動きで" : "", m_name);
					abbreviate = 1;/*２回目以降は省略 */
#else
					msg_format("%^s misses you.", m_name);
#endif

				}

				/* Gain shield experience */
				if (object_is_armour(&target_ptr->inventory_list[INVEN_RARM]) || object_is_armour(&target_ptr->inventory_list[INVEN_LARM]))
				{
					int cur = target_ptr->skill_exp[GINOU_SHIELD];
					int max = s_info[target_ptr->pclass].s_max[GINOU_SHIELD];

					if (cur < max)
					{
						DEPTH targetlevel = r_ptr->level;
						int inc = 0;


						/* Extra experience */
						if ((cur / 100) < targetlevel)
						{
							if ((cur / 100 + 15) < targetlevel)
								inc += 1 + (targetlevel - (cur / 100 + 15));
							else
								inc += 1;
						}

						target_ptr->skill_exp[GINOU_SHIELD] = MIN(max, cur + inc);
						target_ptr->update |= (PU_BONUS);
					}
				}

				damage = 0;

				break;
			}
		}

		/* Analyze "visible" monsters only */
		if (is_original_ap_and_seen(target_ptr, m_ptr) && !do_silly_attack)
		{
			/* Count "obvious" attacks (and ones that cause damage) */
			if (obvious || damage || (r_ptr->r_blows[ap_cnt] > 10))
			{
				/* Count attacks of this type */
				if (r_ptr->r_blows[ap_cnt] < MAX_UCHAR)
				{
					r_ptr->r_blows[ap_cnt]++;
				}
			}
		}

		if (target_ptr->riding && damage)
		{
			char m_steed_name[MAX_NLEN];
			monster_desc(target_ptr, m_steed_name, &floor_ptr->m_list[target_ptr->riding], 0);
			if (rakuba(target_ptr, (damage > 200) ? 200 : damage, FALSE))
			{
				msg_format(_("%^sから落ちてしまった！", "You have fallen from %s."), m_steed_name);
			}
		}

		if (target_ptr->special_defense & NINJA_KAWARIMI)
		{
			if (kawarimi(target_ptr, FALSE)) return TRUE;
		}
	}

	/* Hex - revenge damage stored */
	revenge_store(target_ptr, get_damage);

	if ((target_ptr->tim_eyeeye || hex_spelling(target_ptr, HEX_EYE_FOR_EYE))
		&& get_damage > 0 && !target_ptr->is_dead)
	{
#ifdef JP
		msg_format("攻撃が%s自身を傷つけた！", m_name);
#else
		GAME_TEXT m_name_self[80];

		/* hisself */
		monster_desc(target_ptr, m_name_self, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE | MD_OBJECTIVE);

		msg_format("The attack of %s has wounded %s!", m_name, m_name_self);
#endif
		project(target_ptr, 0, 0, m_ptr->fy, m_ptr->fx, get_damage, GF_MISSILE, PROJECT_KILL, -1);
		if (target_ptr->tim_eyeeye) set_tim_eyeeye(target_ptr, target_ptr->tim_eyeeye-5, TRUE);
	}

	if ((target_ptr->counter || (target_ptr->special_defense & KATA_MUSOU)) && alive && !target_ptr->is_dead && m_ptr->ml && (target_ptr->csp > 7))
	{
		char m_target_name[MAX_NLEN];
		monster_desc(target_ptr, m_target_name, m_ptr, 0);

		target_ptr->csp -= 7;
		msg_format(_("%^sに反撃した！", "You counterattacked %s!"), m_target_name);
		py_attack(target_ptr, m_ptr->fy, m_ptr->fx, HISSATSU_COUNTER);
		fear = FALSE;
		target_ptr->redraw |= (PR_MANA);
	}

	/* Blink away */
	if (blinked && alive && !target_ptr->is_dead)
	{
		if (teleport_barrier(target_ptr, m_idx))
		{
			msg_print(_("泥棒は笑って逃げ...ようとしたがバリアに防がれた。", "The thief flees laughing...? But a magic barrier obstructs it."));
		}
		else
		{
			msg_print(_("泥棒は笑って逃げた！", "The thief flees laughing!"));
			teleport_away(target_ptr, m_idx, MAX_SIGHT * 2 + 5, TELEPORT_SPONTANEOUS);
		}
	}

	/* Always notice cause of death */
	if (target_ptr->is_dead && (r_ptr->r_deaths < MAX_SHORT) && !floor_ptr->inside_arena)
	{
		r_ptr->r_deaths++;
	}

	if (m_ptr->ml && fear && alive && !target_ptr->is_dead)
	{
		sound(SOUND_FLEE);
		msg_format(_("%^sは恐怖で逃げ出した！", "%^s flees in terror!"), m_name);
	}

	if (target_ptr->special_defense & KATA_IAI)
	{
		set_action(target_ptr, ACTION_NONE);
	}

	return TRUE;
}


/*!
 * @brief モンスターから敵モンスターへの打撃攻撃処理
 * @param m_idx 攻撃側モンスターの参照ID
 * @param t_idx 目標側モンスターの参照ID
 * @return 実際に打撃処理が行われた場合TRUEを返す
 */
bool monst_attack_monst(player_type *subject_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx)
{
	monster_type *m_ptr = &subject_ptr->current_floor_ptr->m_list[m_idx];
	monster_type *t_ptr = &subject_ptr->current_floor_ptr->m_list[t_idx];

	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_race *tr_ptr = &r_info[t_ptr->r_idx];

	int pt;
	GAME_TEXT m_name[MAX_NLEN], t_name[MAX_NLEN];
	char temp[MAX_NLEN];
	bool explode = FALSE, touched = FALSE, fear = FALSE, dead = FALSE;
	POSITION y_saver = t_ptr->fy;
	POSITION x_saver = t_ptr->fx;
	int effect_type;

	bool see_m = is_seen(m_ptr);
	bool see_t = is_seen(t_ptr);
	bool see_either = see_m || see_t;

	/* Can the player be aware of this attack? */
	bool known = (m_ptr->cdis <= MAX_SIGHT) || (t_ptr->cdis <= MAX_SIGHT);
	bool do_silly_attack = (one_in_(2) && subject_ptr->image);

	if (m_idx == t_idx) return FALSE;
	if (r_ptr->flags1 & RF1_NEVER_BLOW) return FALSE;
	if (d_info[subject_ptr->dungeon_idx].flags1 & DF1_NO_MELEE) return FALSE;

	/* Total armor */
	ARMOUR_CLASS ac = tr_ptr->ac;

	/* Extract the effective monster level */
	DEPTH rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);

	monster_desc(subject_ptr, m_name, m_ptr, 0);
	monster_desc(subject_ptr, t_name, t_ptr, 0);

	/* Assume no blink */
	bool blinked = FALSE;

	if (!see_either && known)
	{
		subject_ptr->current_floor_ptr->monster_noise = TRUE;
	}

	if (subject_ptr->riding && (m_idx == subject_ptr->riding)) disturb(subject_ptr, TRUE, TRUE);

	/* Scan through all four blows */
	for (ARMOUR_CLASS ap_cnt = 0; ap_cnt < 4; ap_cnt++)
	{
		bool obvious = FALSE;

		HIT_POINT power = 0;
		HIT_POINT damage = 0;

		concptr act = NULL;

		/* Extract the attack infomation */
		int effect = r_ptr->blow[ap_cnt].effect;
		int method = r_ptr->blow[ap_cnt].method;
		int d_dice = r_ptr->blow[ap_cnt].d_dice;
		int d_side = r_ptr->blow[ap_cnt].d_side;

		if (!monster_is_valid(m_ptr)) break;

		/* Stop attacking if the target dies! */
		if (t_ptr->fx != x_saver || t_ptr->fy != y_saver)
			break;

		/* Hack -- no more attacks */
		if (!method) break;

		if (method == RBM_SHOOT) continue;

		/* Extract the attack "power" */
		power = mbe_info[effect].power;

		/* Monster hits */
		if (!effect || check_hit2(power, rlev, ac, MON_STUNNED(m_ptr)))
		{
			(void)set_monster_csleep(subject_ptr, t_idx, 0);

			if (t_ptr->ml)
			{
				/* Redraw the health bar */
				if (subject_ptr->health_who == t_idx) subject_ptr->redraw |= (PR_HEALTH);
				if (subject_ptr->riding == t_idx) subject_ptr->redraw |= (PR_UHEALTH);
			}

			/* Describe the attack method */
			switch (method)
			{
			case RBM_HIT:
			{
				act = _("%sを殴った。", "hits %s.");
				touched = TRUE;
				break;
			}

			case RBM_TOUCH:
			{
				act = _("%sを触った。", "touches %s.");
				touched = TRUE;
				break;
			}

			case RBM_PUNCH:
			{
				act = _("%sをパンチした。", "punches %s.");
				touched = TRUE;
				break;
			}

			case RBM_KICK:
			{
				act = _("%sを蹴った。", "kicks %s.");
				touched = TRUE;
				break;
			}

			case RBM_CLAW:
			{
				act = _("%sをひっかいた。", "claws %s.");
				touched = TRUE;
				break;
			}

			case RBM_BITE:
			{
				act = _("%sを噛んだ。", "bites %s.");
				touched = TRUE;
				break;
			}

			case RBM_STING:
			{
				act = _("%sを刺した。", "stings %s.");
				touched = TRUE;
				break;
			}

			case RBM_SLASH:
			{
				act = _("%sを斬った。", "slashes %s.");
				break;
			}

			case RBM_BUTT:
			{
				act = _("%sを角で突いた。", "butts %s.");
				touched = TRUE;
				break;
			}

			case RBM_CRUSH:
			{
				act = _("%sに体当りした。", "crushes %s.");
				touched = TRUE;
				break;
			}

			case RBM_ENGULF:
			{
				act = _("%sを飲み込んだ。", "engulfs %s.");
				touched = TRUE;
				break;
			}

			case RBM_CHARGE:
			{
				act = _("%sに請求書をよこした。", "charges %s.");
				touched = TRUE;
				break;
			}

			case RBM_CRAWL:
			{
				act = _("%sの体の上を這い回った。", "crawls on %s.");
				touched = TRUE;
				break;
			}

			case RBM_DROOL:
			{
				act = _("%sによだれをたらした。", "drools on %s.");
				touched = FALSE;
				break;
			}

			case RBM_SPIT:
			{
				act = _("%sに唾を吐いた。", "spits on %s.");
				touched = FALSE;
				break;
			}

			case RBM_EXPLODE:
			{
				if (see_either) disturb(subject_ptr, TRUE, TRUE);
				act = _("爆発した。", "explodes.");
				explode = TRUE;
				touched = FALSE;
				break;
			}

			case RBM_GAZE:
			{
				act = _("%sをにらんだ。", "gazes at %s.");
				touched = FALSE;
				break;
			}

			case RBM_WAIL:
			{
				act = _("%sに泣きついた。", "wails at %s.");
				touched = FALSE;
				break;
			}

			case RBM_SPORE:
			{
				act = _("%sに胞子を飛ばした。", "releases spores at %s.");
				touched = FALSE;
				break;
			}

			case RBM_XXX4:
			{
				act = _("%sにXXX4を飛ばした。", "projects XXX4's at %s.");
				touched = FALSE;
				break;
			}

			case RBM_BEG:
			{
				act = _("%sに金をせがんだ。", "begs %s for money.");
				touched = FALSE;
				break;
			}

			case RBM_INSULT:
			{
				act = _("%sを侮辱した。", "insults %s.");
				touched = FALSE;
				break;
			}

			case RBM_MOAN:
			{
				act = _("%sにむかってうめいた。", "moans at %s.");
				touched = FALSE;
				break;
			}

			case RBM_SHOW:
			{
				act = _("%sにむかって歌った。", "sings to %s.");
				touched = FALSE;
				break;
			}
			}

			if (act && see_either)
			{
#ifdef JP
				if (do_silly_attack) act = silly_attacks2[randint0(MAX_SILLY_ATTACK)];
				strfmt(temp, act, t_name);
				msg_format("%^sは%s", m_name, temp);
#else
				if (do_silly_attack)
				{
					act = silly_attacks[randint0(MAX_SILLY_ATTACK)];
					strfmt(temp, "%s %s.", act, t_name);
				}
				else strfmt(temp, act, t_name);
				msg_format("%^s %s", m_name, temp);
#endif
			}

			/* Hack -- assume all attacks are obvious */
			obvious = TRUE;

			/* Roll out the damage */
			damage = damroll(d_dice, d_side);

			/* Assume no effect */
			effect_type = BLOW_EFFECT_TYPE_NONE;

			pt = GF_MISSILE;

			/* Apply appropriate damage */
			switch (effect)
			{
			case 0:
			case RBE_DR_MANA:
				damage = pt = 0;
				break;

			case RBE_SUPERHURT:
				if ((randint1(rlev * 2 + 250) > (ac + 200)) || one_in_(13))
				{
					int tmp_damage = damage - (damage * ((ac < 150) ? ac : 150) / 250);
					damage = MAX(damage, tmp_damage * 2);
					break;
				}

				/* Fall through */

			case RBE_HURT:
				damage -= (damage * ((ac < 150) ? ac : 150) / 250);
				break;

			case RBE_POISON:
			case RBE_DISEASE:
				pt = GF_POIS;
				break;

			case RBE_UN_BONUS:
			case RBE_UN_POWER:
				pt = GF_DISENCHANT;
				break;

			case RBE_EAT_ITEM:
			case RBE_EAT_GOLD:
				if ((subject_ptr->riding != m_idx) && one_in_(2)) blinked = TRUE;
				break;

			case RBE_EAT_FOOD:
			case RBE_EAT_LITE:
			case RBE_BLIND:
			case RBE_LOSE_STR:
			case RBE_LOSE_INT:
			case RBE_LOSE_WIS:
			case RBE_LOSE_DEX:
			case RBE_LOSE_CON:
			case RBE_LOSE_CHR:
			case RBE_LOSE_ALL:
				break;

			case RBE_ACID:
				pt = GF_ACID;
				break;

			case RBE_ELEC:
				pt = GF_ELEC;
				break;

			case RBE_FIRE:
				pt = GF_FIRE;
				break;

			case RBE_COLD:
				pt = GF_COLD;
				break;

			case RBE_CONFUSE:
				pt = GF_CONFUSION;
				break;

			case RBE_TERRIFY:
				effect_type = BLOW_EFFECT_TYPE_FEAR;
				break;

			case RBE_PARALYZE:
				effect_type = BLOW_EFFECT_TYPE_SLEEP;
				break;

			case RBE_SHATTER:
				damage -= (damage * ((ac < 150) ? ac : 150) / 250);
				if (damage > 23) earthquake(subject_ptr, m_ptr->fy, m_ptr->fx, 8, m_idx);
				break;

			case RBE_EXP_10:
			case RBE_EXP_20:
			case RBE_EXP_40:
			case RBE_EXP_80:
				pt = GF_NETHER;
				break;

			case RBE_TIME:
				pt = GF_TIME;
				break;

			case RBE_DR_LIFE:
				pt = GF_HYPODYNAMIA;
				effect_type = BLOW_EFFECT_TYPE_HEAL;
				break;

			case RBE_INERTIA:
				pt = GF_INERTIAL;
				break;

			case RBE_STUN:
				pt = GF_SOUND;
				break;

			default:
				pt = 0;
				break;
			}

			if (pt)
			{
				/* Do damage if not exploding */
				if (!explode)
				{
					project(subject_ptr, m_idx, 0, t_ptr->fy, t_ptr->fx,
						damage, pt, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
				}

				switch (effect_type)
				{
				case BLOW_EFFECT_TYPE_FEAR:
					project(subject_ptr, m_idx, 0, t_ptr->fy, t_ptr->fx,
						damage, GF_TURN_ALL, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
					break;

				case BLOW_EFFECT_TYPE_SLEEP:
					project(subject_ptr, m_idx, 0, t_ptr->fy, t_ptr->fx,
						r_ptr->level, GF_OLD_SLEEP, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
					break;

				case BLOW_EFFECT_TYPE_HEAL:
					if ((monster_living(m_idx)) && (damage > 2))
					{
						bool did_heal = FALSE;

						if (m_ptr->hp < m_ptr->maxhp) did_heal = TRUE;

						/* Heal */
						m_ptr->hp += damroll(4, damage / 6);
						if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

						/* Redraw (later) if needed */
						if (subject_ptr->health_who == m_idx) subject_ptr->redraw |= (PR_HEALTH);
						if (subject_ptr->riding == m_idx) subject_ptr->redraw |= (PR_UHEALTH);

						/* Special message */
						if (see_m && did_heal)
						{
							msg_format(_("%sは体力を回復したようだ。", "%^s appears healthier."), m_name);
						}
					}
					break;
				}

				if (touched)
				{
					/* Aura fire */
					if ((tr_ptr->flags2 & RF2_AURA_FIRE) && m_ptr->r_idx)
					{
						if (!(r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK))
						{
							if (see_either)
							{
								msg_format(_("%^sは突然熱くなった！", "%^s is suddenly very hot!"), m_name);
							}
							if (m_ptr->ml && is_original_ap_and_seen(subject_ptr, t_ptr)) tr_ptr->r_flags2 |= RF2_AURA_FIRE;
							project(subject_ptr, t_idx, 0, m_ptr->fy, m_ptr->fx,
								damroll(1 + ((tr_ptr->level) / 26),
									1 + ((tr_ptr->level) / 17)),
								GF_FIRE, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
						}
						else
						{
							if (is_original_ap_and_seen(subject_ptr, m_ptr)) r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK);
						}
					}

					/* Aura cold */
					if ((tr_ptr->flags3 & RF3_AURA_COLD) && m_ptr->r_idx)
					{
						if (!(r_ptr->flagsr & RFR_EFF_IM_COLD_MASK))
						{
							if (see_either)
							{
								msg_format(_("%^sは突然寒くなった！", "%^s is suddenly very cold!"), m_name);
							}
							if (m_ptr->ml && is_original_ap_and_seen(subject_ptr, t_ptr)) tr_ptr->r_flags3 |= RF3_AURA_COLD;
							project(subject_ptr, t_idx, 0, m_ptr->fy, m_ptr->fx,
								damroll(1 + ((tr_ptr->level) / 26),
									1 + ((tr_ptr->level) / 17)),
								GF_COLD, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
						}
						else
						{
							if (is_original_ap_and_seen(subject_ptr, m_ptr)) r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_COLD_MASK);
						}
					}

					/* Aura elec */
					if ((tr_ptr->flags2 & RF2_AURA_ELEC) && m_ptr->r_idx)
					{
						if (!(r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK))
						{
							if (see_either)
							{
								msg_format(_("%^sは電撃を食らった！", "%^s gets zapped!"), m_name);
							}
							if (m_ptr->ml && is_original_ap_and_seen(subject_ptr, t_ptr)) tr_ptr->r_flags2 |= RF2_AURA_ELEC;
							project(subject_ptr, t_idx, 0, m_ptr->fy, m_ptr->fx,
								damroll(1 + ((tr_ptr->level) / 26),
									1 + ((tr_ptr->level) / 17)),
								GF_ELEC, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
						}
						else
						{
							if (is_original_ap_and_seen(subject_ptr, m_ptr)) r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK);
						}
					}
				}
			}
		}

		/* Monster missed player */
		else
		{
			/* Analyze failed attacks */
			switch (method)
			{
			case RBM_HIT:
			case RBM_TOUCH:
			case RBM_PUNCH:
			case RBM_KICK:
			case RBM_CLAW:
			case RBM_BITE:
			case RBM_STING:
			case RBM_SLASH:
			case RBM_BUTT:
			case RBM_CRUSH:
			case RBM_ENGULF:
			case RBM_CHARGE:
			{
				(void)set_monster_csleep(subject_ptr, t_idx, 0);

				/* Visible monsters */
				if (see_m)
				{
#ifdef JP
					msg_format("%sは%^sの攻撃をかわした。", t_name, m_name);
#else
					msg_format("%^s misses %s.", m_name, t_name);
#endif
				}

				break;
			}
			}
		}


		/* Analyze "visible" monsters only */
		if (is_original_ap_and_seen(subject_ptr, m_ptr) && !do_silly_attack)
		{
			/* Count "obvious" attacks (and ones that cause damage) */
			if (obvious || damage || (r_ptr->r_blows[ap_cnt] > 10))
			{
				/* Count attacks of this type */
				if (r_ptr->r_blows[ap_cnt] < MAX_UCHAR)
				{
					r_ptr->r_blows[ap_cnt]++;
				}
			}
		}
	}

	if (explode)
	{
		sound(SOUND_EXPLODE);

		/* Cancel Invulnerability */
		(void)set_monster_invulner(subject_ptr, m_idx, 0, FALSE);
		mon_take_hit_mon(subject_ptr, m_idx, m_ptr->hp + 1, &dead, &fear, _("は爆発して粉々になった。", " explodes into tiny shreds."), m_idx);
		blinked = FALSE;
	}

	if (!blinked || m_ptr->r_idx == 0) return TRUE;

	if (teleport_barrier(subject_ptr, m_idx))
	{
		if (see_m)
		{
			msg_print(_("泥棒は笑って逃げ...ようとしたがバリアに防がれた。", "The thief flees laughing...? But a magic barrier obstructs it."));
		}
		else if (known)
		{
			subject_ptr->current_floor_ptr->monster_noise = TRUE;
		}
	}
	else
	{
		if (see_m)
		{
			msg_print(_("泥棒は笑って逃げた！", "The thief flees laughing!"));
		}
		else if (known)
		{
			subject_ptr->current_floor_ptr->monster_noise = TRUE;
		}

		teleport_away(subject_ptr, m_idx, MAX_SIGHT * 2 + 5, TELEPORT_SPONTANEOUS);
	}

	return TRUE;
}



/*!
 * @brief モンスターが敵モンスターに行う打撃処理 /
 * Hack, based on mon_take_hit... perhaps all monster attacks on other monsters should use this?
 * @param m_idx 目標となるモンスターの参照ID
 * @param dam ダメージ量
 * @param dead 目標となったモンスターの死亡状態を返す参照ポインタ
 * @param fear 目標となったモンスターの恐慌状態を返す参照ポインタ
 * @param note 目標モンスターが死亡した場合の特別メッセージ(NULLならば標準表示を行う)
 * @param who 打撃を行ったモンスターの参照ID
 * @return なし
 */
void mon_take_hit_mon(player_type *player_ptr, MONSTER_IDX m_idx, HIT_POINT dam, bool *dead, bool *fear, concptr note, MONSTER_IDX who)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	monster_type *m_ptr = &floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	GAME_TEXT m_name[160];
	bool seen = is_seen(m_ptr);

	/* Can the player be aware of this attack? */
	bool known = (m_ptr->cdis <= MAX_SIGHT);

	monster_desc(player_ptr, m_name, m_ptr, 0);

	/* Redraw (later) if needed */
	if (m_ptr->ml)
	{
		if (player_ptr->health_who == m_idx) player_ptr->redraw |= (PR_HEALTH);
		if (player_ptr->riding == m_idx) player_ptr->redraw |= (PR_UHEALTH);
	}

	(void)set_monster_csleep(player_ptr, m_idx, 0);

	if (player_ptr->riding && (m_idx == player_ptr->riding)) disturb(player_ptr, TRUE, TRUE);

	if (MON_INVULNER(m_ptr) && randint0(PENETRATE_INVULNERABILITY))
	{
		if (seen)
		{
			msg_format(_("%^sはダメージを受けない。", "%^s is unharmed."), m_name);
		}
		return;
	}

	if (r_ptr->flagsr & RFR_RES_ALL)
	{
		if (dam > 0)
		{
			dam /= 100;
			if ((dam == 0) && one_in_(3)) dam = 1;
		}
		if (dam == 0)
		{
			if (seen)
			{
				msg_format(_("%^sはダメージを受けない。", "%^s is unharmed."), m_name);
			}
			return;
		}
	}

	/* Hurt it */
	m_ptr->hp -= dam;

	/* It is dead now... or is it? */
	if (m_ptr->hp < 0)
	{
		if (((r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) ||
			(r_ptr->flags7 & RF7_NAZGUL)) &&
			!player_ptr->phase_out)
		{
			m_ptr->hp = 1;
		}
		else
		{
			/* Make a sound */
			if (!monster_living(m_ptr->r_idx))
			{
				sound(SOUND_N_KILL);
			}
			else
			{
				sound(SOUND_KILL);
			}

			*dead = TRUE;

			if (known)
			{
				monster_desc(player_ptr, m_name, m_ptr, MD_TRUE_NAME);
				/* Unseen death by normal attack */
				if (!seen)
				{
					floor_ptr->monster_noise = TRUE;
				}
				/* Death by special attack */
				else if (note)
				{
					msg_format(_("%^s%s", "%^s%s"), m_name, note);
				}
				/* Death by normal attack -- nonliving monster */
				else if (!monster_living(m_ptr->r_idx))
				{
					msg_format(_("%^sは破壊された。", "%^s is destroyed."), m_name);
				}
				/* Death by normal attack -- living monster */
				else
				{
					msg_format(_("%^sは殺された。", "%^s is killed."), m_name);
				}
			}

			monster_gain_exp(player_ptr, who, m_ptr->r_idx);
			monster_death(player_ptr, m_idx, FALSE);
			delete_monster_idx(player_ptr, m_idx);

			/* Not afraid */
			(*fear) = FALSE;

			/* Monster is dead */
			return;
		}
	}

	*dead = FALSE;

	/* Mega-Hack -- Pain cancels fear */
	if (MON_MONFEAR(m_ptr) && (dam > 0))
	{
		/* Cure fear */
		if (set_monster_monfear(player_ptr, m_idx, MON_MONFEAR(m_ptr) - randint1(dam / 4)))
		{
			/* No more fear */
			(*fear) = FALSE;
		}
	}

	/* Sometimes a monster gets scared by damage */
	if (!MON_MONFEAR(m_ptr) && !(r_ptr->flags3 & RF3_NO_FEAR))
	{
		/* Percentage of fully healthy */
		int percentage = (100L * m_ptr->hp) / m_ptr->maxhp;

		/*
		* Run (sometimes) if at 10% or less of max hit points,
		* or (usually) when hit for half its current hit points
		 */
		if (((percentage <= 10) && (randint0(10) < percentage)) ||
			((dam >= m_ptr->hp) && (randint0(100) < 80)))
		{
			/* Hack -- note fear */
			(*fear) = TRUE;

			/* Hack -- Add some timed fear */
			(void)set_monster_monfear(player_ptr, m_idx, (randint1(10) +
				(((dam >= m_ptr->hp) && (percentage > 7)) ?
					20 : ((11 - percentage) * 5))));
		}
	}

	if ((dam > 0) && !is_pet(m_ptr) && !is_friendly(m_ptr) && (who != m_idx))
	{
		if (is_pet(&floor_ptr->m_list[who]) && !player_bold(player_ptr, m_ptr->target_y, m_ptr->target_x))
		{
			set_target(m_ptr, floor_ptr->m_list[who].fy, floor_ptr->m_list[who].fx);
		}
	}

	if (player_ptr->riding && (player_ptr->riding == m_idx) && (dam > 0))
	{
		monster_desc(player_ptr, m_name, m_ptr, 0);

		if (m_ptr->hp > m_ptr->maxhp / 3) dam = (dam + 1) / 2;
		if (rakuba(player_ptr, (dam > 200) ? 200 : dam, FALSE))
		{
			msg_format(_("%^sに振り落とされた！", "You have been thrown off from %s!"), m_name);
		}
	}
}
