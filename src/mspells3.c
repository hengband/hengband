/*!
 * @file mspells3.c
 * @brief 青魔法の処理実装 / Blue magic
 * @date 2014/01/15
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen.\n
 */

#include "angband.h"
#include "core/stuff-handler.h"
#include "util.h"
#include "main/sound-definitions-table.h"

#include "floor.h"
#include "grid.h"
#include "spells-summon.h"
#include "avatar.h"
#include "spells-status.h"
#include "cmd-spell.h"
#include "player-status.h"
#include "monster-spell.h"
#include "monster-status.h"
#include "spell/spells-type.h"
#include "cmd-basic.h"
#include "player-effects.h"
#include "targeting.h"
#include "view/display-main-window.h"
#include "spell/spells2.h"

 /*!
  * @brief モンスター魔法をプレイヤーが使用する場合の換算レベル
  * @param caster_ptr プレーヤーへの参照ポインタ
  * @param 換算レベル
  */
PLAYER_LEVEL get_pseudo_monstetr_level(player_type *caster_ptr)
{
	PLAYER_LEVEL monster_level = caster_ptr->lev + 40;
	return (monster_level * monster_level - 1550) / 130;
}


 /*!
  * @brief 青魔法テーブル
  * @details
  * level,  smana,  %fail,  manedam,  %manefail,  use_stat, name
  */
const monster_power monster_powers[MAX_MONSPELLS] =
{
#ifdef JP
{  1,   1,  10,    0,  15, A_CON,  "叫ぶ"},
{ 10,   4,  35,   89,  40, A_INT,  "何か"},
{ 40,  35,  85,    0,  40, A_INT,  "魔力消去"},
{ 35,  30,  80,  800,  70, A_STR,  "ロケット"},
{  5,   1,  20,   18,  15, A_DEX,  "射撃"},
{ 10,   4,  35,   89,  40, A_INT,  "何か"},
{ 10,   4,  35,   89,  40, A_INT,  "何か"},
{ 10,   4,  35,   89,  40, A_INT,  "何か"},
{ 20,  15,  55, 1600,  95, A_CON,  "酸のブレス"},
{ 20,  15,  55, 1600,  95, A_CON,  "電撃のブレス"},
{ 20,  15,  55, 1600,  95, A_CON,  "炎のブレス"},
{ 20,  15,  55, 1600,  95, A_CON,  "冷気のブレス"},
{ 20,  15,  55,  800,  95, A_CON,  "毒のブレス"},
{ 20,  15,  70,  550,  95, A_CON,  "地獄のブレス"},
{ 20,  16,  70,  400,  95, A_CON,  "閃光のブレス"},
{ 20,  16,  70,  400,  95, A_CON,  "暗黒のブレス"},
{ 20,  20,  70,  450,  95, A_CON,  "混乱のブレス"},
{ 20,  20,  70,  450,  95, A_CON,  "轟音のブレス"},
{ 20,  20,  70,  600,  95, A_CON,  "カオスのブレス"},
{ 20,  16,  70,  500,  95, A_CON,  "劣化のブレス"},
{ 30,  25,  80,  250,  95, A_CON,  "因果混乱のブレス"},
{ 35,  18,  80,  150,  95, A_CON,  "時間逆転のブレス"},
{ 30,  25,  80,  200,  95, A_CON,  "遅鈍のブレス"},
{ 30,  28,  90,  200,  95, A_CON,  "重力のブレス"},
{ 20,  15,  70,  500,  95, A_CON,  "破片のブレス"},
{ 35,  15,  80,  150,  95, A_CON,  "プラズマのブレス"},
{ 30,  18,  70,  200,  95, A_CON,  "フォースのブレス"},
{ 30,  28,  80,  250,  95, A_CON,  "魔力のブレス"},
{ 25,  20,  95,  320,  80, A_INT,  "放射能球"},
{ 25,  15,  70,  800,  95, A_CON,  "放射性廃棄物のブレス"},
{ 30,  32,  85,  400,  80, A_INT,  "純ログルス"},
{ 35,  40,  95,  150,  95, A_CON,  "分解のブレス"},
{ 18,  13,  55,  630,  80, A_INT,  "アシッド・ボール"},
{ 14,  10,  45,  316,  60, A_INT,  "サンダー・ボール"},
{ 20,  14,  60,  720,  80, A_INT,  "ファイア・ボール"},
{ 15,  11,  50,  320,  60, A_INT,  "アイス・ボール"},
{  5,   3,  40,   48,  20, A_INT,  "悪臭雲"},
{ 25,  18,  70,  350,  80, A_INT,  "地獄球"},
{ 30,  22,  75,  350,  80, A_INT,  "ウォーター・ボール"},
{ 44,  45,  85,  550,  95, A_INT,  "魔力の嵐"},
{ 40,  42,  90,  550,  95, A_INT,  "暗黒の嵐"},
{ 10,   5,  50,    0,  25, A_INT,  "魔力吸収"},
{ 25,  10,  60,    0,  30, A_INT,  "精神攻撃"},
{ 30,  14,  65,    0,  30, A_INT,  "脳攻撃"},
{  3,   1,  25,   24,  20, A_INT,  "軽傷"},
{ 12,   2,  35,   64,  25, A_INT,  "重傷"},
{ 22,   6,  50,  150,  30, A_INT,  "致命傷"},
{ 32,  10,  70,  225,  35, A_INT,  "秘孔を突く"},
{ 13,   7,  40,  178,  40, A_INT,  "アシッド・ボルト"},
{ 10,   5,  35,  130,  35, A_INT,  "サンダー・ボルト"},
{ 15,   9,  50,  210,  45, A_INT,  "ファイア・ボルト"},
{ 12,   6,  35,  162,  40, A_INT,  "アイス・ボルト"},
{ 40,  42,  90,  550,  95, A_INT,  "スター・バースト"},
{ 25,  17,  60,  255,  60, A_INT,  "地獄の矢"},
{ 25,  20,  65,  250,  60, A_INT,  "ウォーター・ボルト"},
{ 25,  24,  90,  400,  80, A_INT,  "魔力の矢"},
{ 25,  20,  80,  216,  60, A_INT,  "プラズマ・ボルト"},
{ 25,  16,  60,  186,  60, A_INT,  "極寒の矢"},
{  3,   1,  25,   12,  20, A_INT,  "マジック・ミサイル"},
{  5,   3,  35,    0,  20, A_INT,  "恐慌"},
{ 10,   5,  40,    0,  20, A_INT,  "盲目"},
{ 10,   5,  40,    0,  20, A_INT,  "パニック・モンスター"},
{ 10,   5,  40,    0,  20, A_INT,  "スロウ・モンスター"},
{ 10,   5,  40,    0,  20, A_INT,  "スリープ・モンスター"},
{ 20,  10,  70,    0,  40, A_INT,  "スピード"},
{ 45, 120,  95,    0,  60, A_INT,  "破滅の手"},
{ 20,  15,  70,    0,  20, A_WIS,  "体力回復"},
{ 45,  65,  80,    0,  60, A_INT,  "無傷の球"},
{  5,   1,  30,    0,  20, A_INT,  "ショート・テレポート"},
{ 15,   8,  40,    0,  30, A_INT,  "テレポート"},
{ 40, 999,  99,    0,  80, A_INT,  "ザ・ワールド"},
{  1,   0,   0,    0,  15, A_INT,  "何か"},
{ 15,   8,  50,    0,  30, A_INT,  "引きよせる"},
{ 20,  13,  80,    0,  30, A_INT,  "テレポート・アウェイ"},
{ 30,  40,  95,    0,  40, A_INT,  "テレポート・レベル"},
{ 35,  30,  80,  350,  70, A_INT,  "光の剣"},
{  5,   1,  20,    0,  15, A_INT,  "暗闇"},
{  5,   1,  20,    0,  15, A_DEX,  "トラップ創造"},
{ 15,   3,  40,    0,  30, A_INT,  "記憶喪失"},
{ 30,  30,  70,    0,  40, A_INT,  "死者復活"},
{ 40,  70,  85,    0,  45, A_INT,  "援軍を呼ぶ"},
{ 45,  90,  90,    0,  50, A_INT,  "サイバーデーモンの召喚"},
{ 25,  20,  65,    0,  30, A_INT,  "モンスターの召喚"},
{ 35,  30,  75,    0,  40, A_INT,  "複数のモンスターの召喚"},
{ 25,  25,  65,    0,  25, A_INT,  "アリの召喚"},
{ 25,  20,  60,    0,  25, A_INT,  "蜘蛛の召喚"},
{ 35,  26,  75,    0,  40, A_INT,  "ハウンドの召喚"},
{ 30,  23,  70,    0,  35, A_INT,  "ヒドラの召喚"},
{ 40,  50,  85,    0,  40, A_INT,  "天使の召喚"},
{ 35,  50,  80,    0,  35, A_INT,  "デーモンの召喚"},
{ 30,  30,  75,    0,  35, A_INT,  "アンデッドの召喚"},
{ 39,  70,  80,    0,  40, A_INT,  "ドラゴンの召喚"},
{ 43,  85,  85,    0,  45, A_INT,  "上級アンデッドの召喚"},
{ 46,  90,  85,    0,  45, A_INT,  "古代ドラゴンの召喚"},
{ 48, 120,  90,    0,  50, A_INT,  "アンバーの王族の召喚"},
{ 50, 150,  95,    0,  50, A_INT,  "ユニークモンスターの召喚"},
#else
{  1,   1,  10,    0,  15, A_CON,  "shriek"},
{ 10,   4,  35,   89,  40, A_INT,  "something"},
{ 40,  35,  85,    0,  40, A_INT,  "dispel-magic"},
{ 35,  30,  80,  800,  70, A_STR,  "rocket"},
{  2,   1,  15,   10,  15, A_DEX,  "arrow"},
{  5,   2,  20,   18,  20, A_DEX,  "arrows"},
{ 12,   3,  25,   30,  25, A_DEX,  "missile"},
{ 16,   4,  30,   42,  30, A_DEX,  "missiles"},
{ 20,  15,  55, 1600,  95, A_CON,  "breath acid"},
{ 20,  15,  55, 1600,  95, A_CON,  "breath lightning"},
{ 20,  15,  55, 1600,  95, A_CON,  "breath fire"},
{ 20,  15,  55, 1600,  95, A_CON,  "breath cold"},
{ 20,  15,  55,  800,  95, A_CON,  "breath poison"},
{ 20,  15,  70,  550,  95, A_CON,  "breath nether"},
{ 20,  16,  70,  400,  95, A_CON,  "breath light"},
{ 20,  16,  70,  400,  95, A_CON,  "breath dark"},
{ 20,  20,  70,  450,  95, A_CON,  "breath confusion"},
{ 20,  20,  70,  450,  95, A_CON,  "breath sound"},
{ 20,  20,  70,  600,  95, A_CON,  "breath chaos"},
{ 20,  16,  70,  500,  95, A_CON,  "breath disenchantment"},
{ 30,  25,  80,  250,  95, A_CON,  "breath nexus"},
{ 35,  18,  80,  150,  95, A_CON,  "breath time"},
{ 30,  25,  80,  200,  95, A_CON,  "breath inertia"},
{ 30,  28,  90,  200,  95, A_CON,  "breath gravity"},
{ 20,  15,  70,  500,  95, A_CON,  "breath shards"},
{ 35,  15,  80,  150,  95, A_CON,  "breath plasma"},
{ 30,  18,  70,  200,  95, A_CON,  "breath force"},
{ 30,  28,  80,  250,  95, A_CON,  "breath mana"},
{ 25,  20,  95,  320,  80, A_INT,  "nuke ball"},
{ 25,  15,  70,  800,  95, A_CON,  "breath nuke"},
{ 30,  32,  85,  400,  80, A_INT,  "raw Logrus"},
{ 35,  40,  95,  150,  95, A_CON,  "breath disintegrate"},
{ 18,  13,  55,  630,  80, A_INT,  "acid ball"},
{ 14,  10,  45,  316,  60, A_INT,  "lightning ball"},
{ 20,  14,  60,  720,  80, A_INT,  "fire ball"},
{ 15,  11,  50,  320,  60, A_INT,  "frost ball"},
{  5,   3,  40,   48,  20, A_INT,  "stinking cloud"},
{ 25,  18,  70,  350,  80, A_INT,  "nether ball"},
{ 30,  22,  75,  350,  80, A_INT,  "water ball"},
{ 44,  45,  85,  550,  95, A_INT,  "mana storm"},
{ 40,  42,  90,  550,  95, A_INT,  "darkness storm"},
{ 10,   5,  50,    0,  25, A_INT,  "drain mana"},
{ 25,  10,  60,    0,  30, A_INT,  "mind blast"},
{ 30,  14,  65,    0,  30, A_INT,  "brain smash"},
{  3,   1,  25,   24,  20, A_INT,  "cause light wounds"},
{ 12,   2,  35,   64,  25, A_INT,  "cause serious wounds"},
{ 22,   6,  50,  150,  30, A_INT,  "cause critical wounds"},
{ 32,  10,  70,  225,  35, A_INT,  "cause mortal wounds"},
{ 13,   7,  40,  178,  40, A_INT,  "acid bolt"},
{ 10,   5,  35,  130,  35, A_INT,  "lightning bolt"},
{ 15,   9,  50,  210,  45, A_INT,  "fire bolt"},
{ 12,   6,  35,  162,  40, A_INT,  "frost bolt"},
{ 40,  42,  90,  550,  95, A_INT,  "starburst"},
{ 25,  17,  60,  255,  60, A_INT,  "nether bolt"},
{ 25,  20,  65,  250,  60, A_INT,  "water bolt"},
{ 25,  24,  90,  400,  80, A_INT,  "mana bolt"},
{ 25,  20,  80,  216,  60, A_INT,  "plasma bolt"},
{ 25,  16,  60,  186,  60, A_INT,  "ice bolt"},
{  3,   1,  25,   12,  20, A_INT,  "magic missile"},
{  5,   3,  35,    0,  20, A_INT,  "scare"},
{ 10,   5,  40,    0,  20, A_INT,  "blind"},
{ 10,   5,  40,    0,  20, A_INT,  "confuse"},
{ 10,   5,  40,    0,  20, A_INT,  "slow"},
{ 10,   5,  40,    0,  20, A_INT,  "sleep"},
{ 20,  10,  70,    0,  40, A_INT,  "speed"},
{ 45, 120,  95,    0,  60, A_INT,  "the Hand of Doom"},
{ 20,  15,  70,    0,  20, A_WIS,  "heal-self"},
{ 45,  65,  80,    0,  60, A_INT,  "make invulnerable"},
{  5,   1,  30,    0,  20, A_INT,  "blink-self"},
{ 15,   8,  40,    0,  30, A_INT,  "teleport-self"},
{ 40, 999,  99,    0,  80, A_INT,  "The world"},
{  1,   0,   0,    0,  15, A_INT,  "something"},
{ 15,   8,  50,    0,  30, A_INT,  "teleport to"},
{ 20,  13,  80,    0,  30, A_INT,  "teleport away"},
{ 30,  40,  95,    0,  40, A_INT,  "teleport level"},
{ 35,  30,  80,  350,  70, A_INT,  "psycho-spear"},
{  5,   1,  20,    0,  15, A_INT,  "create darkness"},
{  5,   1,  20,    0,  15, A_DEX,  "create traps"},
{ 15,   3,  40,    0,  30, A_INT,  "cause amnesia"},
{ 30,  30,  70,    0,  40, A_INT,  "raise dead"},
{ 40,  70,  85,    0,  45, A_INT,  "summon aid"},
{ 45,  90,  90,    0,  50, A_INT,  "summon Cyberdemons"},
{ 25,  20,  65,    0,  30, A_INT,  "summon a monster"},
{ 35,  30,  75,    0,  40, A_INT,  "summon monsters"},
{ 25,  25,  65,    0,  25, A_INT,  "summon ants"},
{ 25,  20,  60,    0,  25, A_INT,  "summon spiders"},
{ 35,  26,  75,    0,  40, A_INT,  "summon hounds"},
{ 30,  23,  70,    0,  35, A_INT,  "summon hydras"},
{ 40,  50,  85,    0,  40, A_INT,  "summon an angel"},
{ 35,  50,  80,    0,  35, A_INT,  "summon a daemon"},
{ 30,  30,  75,    0,  35, A_INT,  "summon an undead"},
{ 39,  70,  80,    0,  40, A_INT,  "summon a dragon"},
{ 43,  85,  85,    0,  45, A_INT,  "summon Greater Undead"},
{ 46,  90,  85,    0,  45, A_INT,  "summon Ancient Dragon"},
{ 48, 120,  90,    0,  50, A_INT,  "summon Lords of Amber"},
{ 50, 150,  95,    0,  50, A_INT,  "summon Unique Monsters"},
#endif

};


/*!
 * @brief モンスター魔法名テーブル
 */
const concptr monster_powers_short[MAX_MONSPELLS] = {
#ifdef JP

	"叫ぶ", "何か", "魔力消去", "ロケット", "射撃", "何か", "何か", "何か",
	"酸", "電撃", "火炎", "冷気", "毒", "地獄", "閃光", "暗黒",
	"混乱", "轟音", "カオス", "劣化", "因果混乱", "時間逆転", "遅鈍", "重力",
	"破片", "プラズマ", "フォース", "魔力", "放射能球", "放射性廃棄物", "純ログルス", "分解",

	"酸", "電撃", "火炎", "冷気", "悪臭雲", "地獄球", "ウォーター", "魔力の嵐",
	"暗黒の嵐", "魔力吸収", "精神攻撃", "脳攻撃", "軽傷", "重傷", "致命傷", "秘孔を突く",
	"酸", "電撃", "火炎", "冷気", "スターバースト", "地獄の矢", "ウォーター", "魔力の矢",
	"プラズマ", "極寒", "マジックミサイル", "恐慌", "盲目", "混乱", "減速", "睡眠",

	"加速", "破滅の手", "体力回復", "無傷の球", "ショートテレポート", "テレポート", "時を止める", "何か",
	"引きよせる", "テレポートアウェイ", "テレポートレベル", "光の剣", "暗闇", "トラップ創造", "記憶喪失", "死者復活",
	"援軍", "サイバーデーモン", "モンスター", "複数のモンスター", "蟻", "蜘蛛", "ハウンド", "ヒドラ",
	"天使", "悪魔", "アンデッド", "ドラゴン", "上級アンデッド", "古代ドラゴン", "アンバーの王族", "ユニーク"

#else

	"Shriek", "Something", "Dispel-magic", "Rocket", "Arrow", "Arrows", "Missile", "Missiles",
	"Acid", "Lightning", "Fire", "Cold", "Poison", "Nether", "Light", "Dark",
	"Confusion", "Sound", "Chaos", "Disenchantment", "Nexus", "Time", "Inertia", "Gravity",
	"Shards", "Plasma", "Force", "Mana", "Nuke", "Nuke", "Logrus", "Disintergrate",

	"Acid", "Lightning", "Fire", "Frost", "Stinking Cloud", "Nether", "Water", "Mana storm",
	"Darkness storm", "Drain mana", "Mind blast", "Brain smash", "Cause Light Wound", "Cause Serious Wound", "Cause Critical Wound", "Cause Mortal Wound",
	"Acid", "Lightning", "Fire", "Frost", "Starburst", "Nether", "Water", "Mana",
	"Plasm", "Ice", "Magic missile", "Scare", "Blind", "Confuse", "Slow", "Sleep",

	"Speed", "Hand of doom", "Heal-self", "Invulnerable", "Blink", "Teleport", "The world", "Something",
	"Teleport to", "Teleport away", "Teleport level", "Psycho-spear", "Create darkness", "Create traps", "Amnesia", "Raise dead",
	"Aid", "Cyberdeamons", "A monster", "Monsters", "Ants", "Spiders", "Hounds", "Hydras",
	"Angel", "Daemon", "Undead", "Dragon", "Greater Undead", "Ancient Dragon", "Lords of Amber", "Unique monsters"

#endif
};


/*!
* @brief 文字列に青魔導師の呪文の攻撃力を加える
* @param SPELL_NUM 呪文番号
* @param plev プレイヤーレベル
* @param msg 表示する文字列
* @param tmp 返すメッセージを格納する配列
* @return なし
*/
void set_bluemage_damage(player_type *learner_type, int SPELL_NUM, PLAYER_LEVEL plev, concptr msg, char* tmp)
{
    int base_damage = monspell_bluemage_damage(learner_type, SPELL_NUM, plev, BASE_DAM);
    int dice_num = monspell_bluemage_damage(learner_type, SPELL_NUM, plev, DICE_NUM);
    int dice_side = monspell_bluemage_damage(learner_type, SPELL_NUM, plev, DICE_SIDE);
    int dice_mult = monspell_bluemage_damage(learner_type, SPELL_NUM, plev, DICE_MULT);
    int dice_div = monspell_bluemage_damage(learner_type, SPELL_NUM, plev, DICE_DIV);
    char dmg_str[80];
    dice_to_string(base_damage, dice_num, dice_side, dice_mult, dice_div, dmg_str);
    sprintf(tmp, " %s %s", msg, dmg_str);
}


/*!
 * @brief 受け取ったモンスター魔法のIDに応じて青魔法の効果情報をまとめたフォーマットを返す
 * @param learner_ptr プレーヤーへの参照ポインタ
 * @param p 情報を返す文字列参照ポインタ
 * @param power モンスター魔法のID
 * @return なし
 */
static void learned_info(player_type *learner_ptr, char *p, int power)
{
	PLAYER_LEVEL plev = get_pseudo_monstetr_level(learner_ptr);

	strcpy(p, "");

	switch (power)
	{
		case MS_SHRIEK:
		case MS_XXX1:
		case MS_XXX2:
		case MS_XXX3:
		case MS_XXX4:
		case MS_SCARE:
		case MS_BLIND:
		case MS_CONF:
		case MS_SLOW:
		case MS_SLEEP:
		case MS_HAND_DOOM:
		case MS_WORLD:
		case MS_SPECIAL:
		case MS_TELE_TO:
		case MS_TELE_AWAY:
		case MS_TELE_LEVEL:
		case MS_DARKNESS:
		case MS_MAKE_TRAP:
		case MS_FORGET:
		case MS_S_KIN:
		case MS_S_CYBER:
		case MS_S_MONSTER:
		case MS_S_MONSTERS:
		case MS_S_ANT:
		case MS_S_SPIDER:
		case MS_S_HOUND:
		case MS_S_HYDRA:
		case MS_S_ANGEL:
		case MS_S_DEMON:
		case MS_S_UNDEAD:
		case MS_S_DRAGON:
		case MS_S_HI_UNDEAD:
		case MS_S_HI_DRAGON:
		case MS_S_AMBERITE:
		case MS_S_UNIQUE:
			break;
        case MS_BALL_MANA:
        case MS_BALL_DARK:
        case MS_STARBURST: 
            set_bluemage_damage(learner_ptr, power, plev, KWD_DAM, p); break;
		case MS_DISPEL:
			break;
        case MS_ROCKET:
        case MS_SHOOT:
        case MS_BR_ACID:
        case MS_BR_ELEC:
        case MS_BR_FIRE:
        case MS_BR_COLD:
        case MS_BR_POIS:
        case MS_BR_NUKE: 
        case MS_BR_NEXUS:
        case MS_BR_TIME:
        case MS_BR_GRAVITY:
        case MS_BR_MANA:
        case MS_BR_NETHER:
        case MS_BR_LITE:
        case MS_BR_DARK:
        case MS_BR_CONF:
        case MS_BR_SOUND:
        case MS_BR_CHAOS:
        case MS_BR_DISEN:
        case MS_BR_SHARDS:
        case MS_BR_PLASMA:
        case MS_BR_INERTIA:
        case MS_BR_FORCE:
        case MS_BR_DISI:
        case MS_BALL_NUKE:
        case MS_BALL_CHAOS:
        case MS_BALL_ACID:
        case MS_BALL_ELEC:
        case MS_BALL_FIRE:
        case MS_BALL_COLD:
        case MS_BALL_POIS:
        case MS_BALL_NETHER:
        case MS_BALL_WATER:
            set_bluemage_damage(learner_ptr, power, plev, KWD_DAM, p); break;
        case MS_DRAIN_MANA:
            set_bluemage_damage(learner_ptr, power, plev, KWD_HEAL, p); break;
        case MS_MIND_BLAST:
        case MS_BRAIN_SMASH:
        case MS_CAUSE_1:
        case MS_CAUSE_2:
        case MS_CAUSE_3:
        case MS_CAUSE_4:
        case MS_BOLT_ACID:
        case MS_BOLT_ELEC:
        case MS_BOLT_FIRE:
        case MS_BOLT_COLD:
        case MS_BOLT_NETHER:
        case MS_BOLT_WATER:
        case MS_BOLT_MANA:
        case MS_BOLT_PLASMA:
        case MS_BOLT_ICE: 
        case MS_MAGIC_MISSILE: 
            set_bluemage_damage(learner_ptr, power, plev, KWD_DAM, p); break;
		case MS_SPEED:
			sprintf(p, " %sd%d+%d", KWD_DURATION, 20+plev, plev);
			break;
        case MS_HEAL:
            set_bluemage_damage(learner_ptr, power, plev, KWD_HEAL, p); break;
		case MS_INVULNER:
			sprintf(p, " %sd7+7", KWD_DURATION);
			break;
		case MS_BLINK:
			sprintf(p, " %s10", KWD_SPHERE);
			break;
		case MS_TELEPORT:
			sprintf(p, " %s%d", KWD_SPHERE, plev * 5);
			break;
        case MS_PSY_SPEAR:
            set_bluemage_damage(learner_ptr, power, plev, KWD_DAM, p); break;
			break;
		case MS_RAISE_DEAD:
			sprintf(p, " %s5", KWD_SPHERE);
			break;
		default:
			break;
	}
}


/*!
 * @brief 使用可能な青魔法を選択する /
 * Allow user to choose a imitation.
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param sn 選択したモンスター攻撃ID、キャンセルの場合-1、不正な選択の場合-2を返す
 * @return 発動可能な魔法を選択した場合TRUE、キャンセル処理か不正な選択が行われた場合FALSEを返す。
 * @details
 * If a valid spell is chosen, saves it in '*sn' and returns TRUE\n
 * If the user hits escape, returns FALSE, and set '*sn' to -1\n
 * If there are no legal choices, returns FALSE, and sets '*sn' to -2\n
 *\n
 * The "prompt" should be "cast", "recite", or "study"\n
 * The "known" should be TRUE for cast/pray, FALSE for study\n
 *\n
 * nb: This function has a (trivial) display bug which will be obvious\n
 * when you run it. It's probably easy to fix but I haven't tried,\n
 * sorry.\n
 */
static bool get_learned_power(player_type *caster_ptr, SPELL_IDX *sn)
{
	int             i = 0;
	int             num = 0;
	TERM_LEN y = 1;
	TERM_LEN x = 18;
	PERCENTAGE minfail = 0;
	PLAYER_LEVEL plev = caster_ptr->lev;
	PERCENTAGE chance = 0;
	int             ask = TRUE, mode = 0;
	int             spellnum[MAX_MONSPELLS];
	char            ch;
	char            choice;
	char            out_val[160];
	char            comment[80];
	BIT_FLAGS f4 = 0L, f5 = 0L, f6 = 0L;
	concptr p = _("魔法", "magic");
	COMMAND_CODE code;
	monster_power   spell;
	bool            flag, redraw;
	int menu_line = (use_menu ? 1 : 0);

	/* Assume cancelled */
	*sn = (-1);

	flag = FALSE;
	redraw = FALSE;

	/* Get the spell, if available */
	
	if (repeat_pull(&code))
	{
		*sn = (SPELL_IDX)code;
		return TRUE;
	}

	if (use_menu)
	{
		screen_save();

		while(!mode)
		{
			prt(format(_(" %s ボルト", " %s bolt"), (menu_line == 1) ? _("》", "> ") : "  "), 2, 14);
			prt(format(_(" %s ボール", " %s ball"), (menu_line == 2) ? _("》", "> ") : "  "), 3, 14);
			prt(format(_(" %s ブレス", " %s breath"), (menu_line == 3) ? _("》", "> ") : "  "), 4, 14);
			prt(format(_(" %s 召喚", " %s sommoning"), (menu_line == 4) ? _("》", "> ") : "  "), 5, 14);
			prt(format(_(" %s その他", " %s others"), (menu_line == 5) ? _("》", "> ") : "  "), 6, 14);
			prt(_("どの種類の魔法を使いますか？", "use which type of magic? "), 0, 0);

			choice = inkey();
			switch(choice)
			{
			case ESCAPE:
			case 'z':
			case 'Z':
				screen_load();
				return FALSE;
			case '2':
			case 'j':
			case 'J':
				menu_line++;
				break;
			case '8':
			case 'k':
			case 'K':
				menu_line+= 4;
				break;
			case '\r':
			case 'x':
			case 'X':
				mode = menu_line;
				break;
			}
			if (menu_line > 5) menu_line -= 5;
		}
		screen_load();
	}
	else
	{
	sprintf(comment, _("[A]ボルト, [B]ボール, [C]ブレス, [D]召喚, [E]その他:", "[A] bolt, [B] ball, [C] breath, [D] summoning, [E] others:"));
	while (TRUE)
	{
		if (!get_com(comment, &ch, TRUE))
		{
			return FALSE;
		}
		if (ch == 'A' || ch == 'a')
		{
			mode = 1;
			break;
		}
		if (ch == 'B' || ch == 'b')
		{
			mode = 2;
			break;
		}
		if (ch == 'C' || ch == 'c')
		{
			mode = 3;
			break;
		}
		if (ch == 'D' || ch == 'd')
		{
			mode = 4;
			break;
		}
		if (ch == 'E' || ch == 'e')
		{
			mode = 5;
			break;
		}
	}
	}

	set_rf_masks(&f4, &f5, &f6, mode);

	for (i = 0, num = 0; i < 32; i++)
	{
		if ((0x00000001 << i) & f4) spellnum[num++] = i;
	}
	for (; i < 64; i++)
	{
		if ((0x00000001 << (i - 32)) & f5) spellnum[num++] = i;
	}
	for (; i < 96; i++)
	{
		if ((0x00000001 << (i - 64)) & f6) spellnum[num++] = i;
	}
	for (i = 0; i < num; i++)
	{
		if (caster_ptr->magic_num2[spellnum[i]])
		{
			if (use_menu) menu_line = i+1;
			break;
		}
	}
	if (i == num)
	{
		msg_print(_("その種類の魔法は覚えていない！", "You don't know any spell of this type."));
		return FALSE;
	}

	/* Build a prompt (accept all spells) */
	(void)strnfmt(out_val, 78, _("(%c-%c, '*'で一覧, ESC) どの%sを唱えますか？", "(%c-%c, *=List, ESC=exit) Use which %s? "),
		I2A(0), I2A(num - 1), p);

	if (use_menu) screen_save();

	choice= (always_show_list || use_menu) ? ESCAPE:1 ;
	while (!flag)
	{
		if(choice==ESCAPE) choice = ' '; 
		else if( !get_com(out_val, &choice, TRUE) )break; 

		if (use_menu && choice != ' ')
		{
			switch(choice)
			{
				case '0':
				{
					screen_load();
					return FALSE;
				}

				case '8':
				case 'k':
				case 'K':
				{
					do
					{
						menu_line += (num-1);
						if (menu_line > num) menu_line -= num;
					} while(!caster_ptr->magic_num2[spellnum[menu_line-1]]);
					break;
				}

				case '2':
				case 'j':
				case 'J':
				{
					do
					{
						menu_line++;
						if (menu_line > num) menu_line -= num;
					} while(!caster_ptr->magic_num2[spellnum[menu_line-1]]);
					break;
				}

				case '6':
				case 'l':
				case 'L':
				{
					menu_line=num;
					while(!caster_ptr->magic_num2[spellnum[menu_line-1]]) menu_line--;
					break;
				}

				case '4':
				case 'h':
				case 'H':
				{
					menu_line=1;
					while(!caster_ptr->magic_num2[spellnum[menu_line-1]]) menu_line++;
					break;
				}

				case 'x':
				case 'X':
				case '\r':
				{
					i = menu_line - 1;
					ask = FALSE;
					break;
				}
			}
		}
		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?') || (use_menu && ask))
		{
			/* Show the list */
			if (!redraw || use_menu)
			{
				char psi_desc[80];
				redraw = TRUE;
				if (!use_menu) screen_save();

				/* Display a list of spells */
				prt("", y, x);
				put_str(_("名前", "Name"), y, x + 5);
				put_str(_("MP 失率 効果", "SP Fail Info"), y, x + 33);


				/* Dump the spells */
				for (i = 0; i < num; i++)
				{
					int need_mana;

					prt("", y + i + 1, x);
					if (!caster_ptr->magic_num2[spellnum[i]]) continue;

					/* Access the spell */
					spell = monster_powers[spellnum[i]];

					chance = spell.fail;

					/* Reduce failure rate by "effective" level adjustment */
					if (plev > spell.level) chance -= 3 * (plev - spell.level);
					else chance += (spell.level - plev);

					/* Reduce failure rate by INT/WIS adjustment */
					chance -= 3 * (adj_mag_stat[caster_ptr->stat_ind[A_INT]] - 1);

					chance = mod_spell_chance_1(caster_ptr, chance);

					need_mana = mod_need_mana(caster_ptr, monster_powers[spellnum[i]].smana, 0, REALM_NONE);

					/* Not enough mana to cast */
					if (need_mana > caster_ptr->csp)
					{
						chance += 5 * (need_mana - caster_ptr->csp);
					}

					/* Extract the minimum failure rate */
					minfail = adj_mag_fail[caster_ptr->stat_ind[A_INT]];

					/* Minimum failure rate */
					if (chance < minfail) chance = minfail;

					/* Stunning makes spells harder */
					if (caster_ptr->stun > 50) chance += 25;
					else if (caster_ptr->stun) chance += 15;

					/* Always a 5 percent chance of working */
					if (chance > 95) chance = 95;

					chance = mod_spell_chance_2(caster_ptr, chance);

					/* Get info */
					learned_info(caster_ptr, comment, spellnum[i]);

					if (use_menu)
					{
						if (i == (menu_line-1)) strcpy(psi_desc, _("  》", "  > "));
						else strcpy(psi_desc, "    ");
					}
					else sprintf(psi_desc, "  %c)", I2A(i));

					/* Dump the spell --(-- */
					strcat(psi_desc, format(" %-26s %3d %3d%%%s",
						spell.name, need_mana,
						chance, comment));
					prt(psi_desc, y + i + 1, x);
				}

				/* Clear the bottom line */
				if (y < 22) prt("", y + i + 1, x);
			}

			/* Hide the list */
			else
			{
				/* Hide list */
				redraw = FALSE;
				screen_load();
			}

			/* Redo asking */
			continue;
		}

		if (!use_menu)
		{
			/* Note verify */
			ask = isupper(choice);

			/* Lowercase */
			if (ask) choice = (char)tolower(choice);

			/* Extract request */
			i = (islower(choice) ? A2I(choice) : -1);
		}

		/* Totally Illegal */
		if ((i < 0) || (i >= num) || !caster_ptr->magic_num2[spellnum[i]])
		{
			bell();
			continue;
		}

		/* Save the spell index */
		spell = monster_powers[spellnum[i]];

		/* Verify it */
		if (ask)
		{
			char tmp_val[160];

			/* Prompt */
			(void) strnfmt(tmp_val, 78, _("%sの魔法を唱えますか？", "Use %s? "), monster_powers[spellnum[i]].name);

			/* Belay that order */
			if (!get_check(tmp_val)) continue;
		}

		/* Stop the loop */
		flag = TRUE;
	}

	if (redraw) screen_load();

	caster_ptr->window |= (PW_SPELL);
	handle_stuff(caster_ptr);

	/* Abort if needed */
	if (!flag) return FALSE;

	/* Save the choice */
	(*sn) = spellnum[i];

	repeat_push((COMMAND_CODE)spellnum[i]);

	/* Success */
	return TRUE;
}


/*!
 * @brief 青魔法の発動 /
 * do_cmd_cast calls this function if the player's class is 'blue-mage'.
 * @param spell 発動するモンスター攻撃のID
 * @param success TRUEは成功時、FALSEは失敗時の処理を行う
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
static bool cast_learned_spell(player_type *caster_ptr, int spell, bool success)
{
	DIRECTION dir;
	PLAYER_LEVEL plev = get_pseudo_monstetr_level(caster_ptr);
	PLAYER_LEVEL summon_lev = caster_ptr->lev * 2 / 3 + randint1(caster_ptr->lev/2);
	HIT_POINT damage = 0;
	bool pet = success;
	bool no_trump = FALSE;
	BIT_FLAGS p_mode, u_mode = 0L, g_mode;

	if (pet)
	{
		p_mode = PM_FORCE_PET;
		g_mode = 0;
	}
	else
	{
		p_mode = PM_NO_PET;
		g_mode = PM_ALLOW_GROUP;
	}

	if (!success || (randint1(50+plev) < plev/10)) u_mode = PM_ALLOW_UNIQUE;

	/* spell code */
	floor_type *floor_ptr = caster_ptr->current_floor_ptr;
	switch (spell)
	{
	case MS_SHRIEK:
        msg_print(_("かん高い金切り声をあげた。", "You make a high pitched shriek."));
		aggravate_monsters(caster_ptr, 0);
		break;
	case MS_XXX1:
		break;
	case MS_DISPEL:
	{
		MONSTER_IDX m_idx;

		if (!target_set(caster_ptr, TARGET_KILL)) return FALSE;
		m_idx = floor_ptr->grid_array[target_row][target_col].m_idx;
		if (!m_idx) break;
		if (!player_has_los_bold(caster_ptr, target_row, target_col)) break;
		if (!projectable(caster_ptr, caster_ptr->y, caster_ptr->x, target_row, target_col)) break;
		dispel_monster_status(caster_ptr, m_idx);
		break;
	}
	case MS_ROCKET:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;
		
        msg_print(_("ロケットを発射した。", "You fire a rocket."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_ROCKET), plev, DAM_ROLL);
		fire_rocket(caster_ptr, GF_ROCKET, dir, damage, 2);
		break;
	case MS_SHOOT:
	{
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;
		
        msg_print(_("矢を放った。", "You fire an arrow."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_SHOOT), plev, DAM_ROLL);
		fire_bolt(caster_ptr, GF_ARROW, dir, damage);
		break;
	}
	case MS_XXX2:
		break;
	case MS_XXX3:
		break;
	case MS_XXX4:
		break;
	case MS_BR_ACID:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("酸のブレスを吐いた。", "You breathe acid."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_ACID), plev, DAM_ROLL);
		fire_breath(caster_ptr, GF_ACID, dir, damage, (plev > 40 ? 3 : 2));
		break;
	case MS_BR_ELEC:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("稲妻のブレスを吐いた。", "You breathe lightning."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_ELEC), plev, DAM_ROLL);
		fire_breath(caster_ptr, GF_ELEC, dir, damage, (plev > 40 ? 3 : 2));
		break;
	case MS_BR_FIRE:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("火炎のブレスを吐いた。", "You breathe fire."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_FIRE), plev, DAM_ROLL);
		fire_breath(caster_ptr, GF_FIRE, dir, damage, (plev > 40 ? 3 : 2));
		break;
	case MS_BR_COLD:
        if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("冷気のブレスを吐いた。", "You breathe frost."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_COLD), plev, DAM_ROLL);
		fire_breath(caster_ptr, GF_COLD, dir, damage, (plev > 40 ? 3 : 2));
		break;
	case MS_BR_POIS:
        if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("ガスのブレスを吐いた。", "You breathe gas."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_POIS), plev, DAM_ROLL);
		fire_breath(caster_ptr, GF_POIS, dir, damage, (plev > 40 ? 3 : 2));
		break;
	case MS_BR_NETHER:
        if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("地獄のブレスを吐いた。", "You breathe nether."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_NETHER), plev, DAM_ROLL);
		fire_breath(caster_ptr, GF_NETHER, dir, damage, (plev > 40 ? 3 : 2));
		break;
	case MS_BR_LITE:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("閃光のブレスを吐いた。", "You breathe light."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_LITE), plev, DAM_ROLL);
		fire_breath(caster_ptr, GF_LITE, dir, damage, (plev > 40 ? 3 : 2));
		break;
	case MS_BR_DARK:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("暗黒のブレスを吐いた。", "You breathe darkness."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_DARK), plev, DAM_ROLL);
		fire_breath(caster_ptr, GF_DARK, dir, damage, (plev > 40 ? 3 : 2));
		break;
	case MS_BR_CONF:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("混乱のブレスを吐いた。", "You breathe confusion."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_CONF), plev, DAM_ROLL);
		fire_breath(caster_ptr, GF_CONFUSION, dir, damage, (plev > 40 ? 3 : 2));
		break;
	case MS_BR_SOUND:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("轟音のブレスを吐いた。", "You breathe sound."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_SOUND), plev, DAM_ROLL);
		fire_breath(caster_ptr, GF_SOUND, dir, damage, (plev > 40 ? 3 : 2));
		break;
	case MS_BR_CHAOS:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("カオスのブレスを吐いた。", "You breathe chaos."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_CHAOS), plev, DAM_ROLL);
		fire_breath(caster_ptr, GF_CHAOS, dir, damage, (plev > 40 ? 3 : 2));
		break;
	case MS_BR_DISEN:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("劣化のブレスを吐いた。", "You breathe disenchantment."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_DISEN), plev, DAM_ROLL);
		fire_breath(caster_ptr, GF_DISENCHANT, dir, damage, (plev > 40 ? 3 : 2));
		break;
	case MS_BR_NEXUS:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("因果混乱のブレスを吐いた。", "You breathe nexus."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_NEXUS), plev, DAM_ROLL);
		fire_breath(caster_ptr, GF_NEXUS, dir, damage, (plev > 40 ? 3 : 2));
		break;
	case MS_BR_TIME:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("時間逆転のブレスを吐いた。", "You breathe time."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_TIME), plev, DAM_ROLL);
		fire_breath(caster_ptr, GF_TIME, dir, damage, (plev > 40 ? 3 : 2));
		break;
	case MS_BR_INERTIA:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("遅鈍のブレスを吐いた。", "You breathe inertia."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_INERTIA), plev, DAM_ROLL);
		fire_breath(caster_ptr, GF_INERTIAL, dir, damage, (plev > 40 ? 3 : 2));
		break;
	case MS_BR_GRAVITY:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("重力のブレスを吐いた。", "You breathe gravity."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_GRAVITY), plev, DAM_ROLL);
		fire_breath(caster_ptr, GF_GRAVITY, dir, damage, (plev > 40 ? 3 : 2));
		break;
	case MS_BR_SHARDS:
        if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("破片のブレスを吐いた。", "You breathe shards."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_SHARDS), plev, DAM_ROLL);
		fire_breath(caster_ptr, GF_SHARDS, dir, damage, (plev > 40 ? 3 : 2));
		break;
	case MS_BR_PLASMA:
        if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("プラズマのブレスを吐いた。", "You breathe plasma."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_PLASMA), plev, DAM_ROLL);
		fire_breath(caster_ptr, GF_PLASMA, dir, damage, (plev > 40 ? 3 : 2));
		break;
	case MS_BR_FORCE:
        if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("フォースのブレスを吐いた。", "You breathe force."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_FORCE), plev, DAM_ROLL);
		fire_breath(caster_ptr, GF_FORCE, dir, damage, (plev > 40 ? 3 : 2));
		break;
	case MS_BR_MANA:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("魔力のブレスを吐いた。", "You breathe mana."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_MANA), plev, DAM_ROLL);
		fire_breath(caster_ptr, GF_MANA, dir, damage, (plev > 40 ? 3 : 2));
		break;
	case MS_BALL_NUKE:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("放射能球を放った。", "You cast a ball of radiation."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_NUKE), plev, DAM_ROLL);
		fire_ball(caster_ptr, GF_NUKE, dir, damage, 2);
		break;
	case MS_BR_NUKE:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("放射性廃棄物のブレスを吐いた。", "You breathe toxic waste."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_NUKE), plev, DAM_ROLL);
		fire_breath(caster_ptr, GF_NUKE, dir, damage, (plev > 40 ? 3 : 2));
		break;
	case MS_BALL_CHAOS:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("純ログルスを放った。", "You invoke a raw Logrus."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_CHAOS), plev, DAM_ROLL);
		fire_ball(caster_ptr, GF_CHAOS, dir, damage, 4);
		break;
	case MS_BR_DISI:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("分解のブレスを吐いた。", "You breathe disintegration."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BR_DISI), plev, DAM_ROLL);
		fire_breath(caster_ptr, GF_DISINTEGRATE, dir, damage, (plev > 40 ? 3 : 2));
		break;
	case MS_BALL_ACID:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("アシッド・ボールの呪文を唱えた。", "You cast an acid ball."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_ACID), plev, DAM_ROLL);
		fire_ball(caster_ptr, GF_ACID, dir, damage, 2);
		break;
	case MS_BALL_ELEC:
        if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("サンダー・ボールの呪文を唱えた。", "You cast a lightning ball."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_ELEC), plev, DAM_ROLL);
		fire_ball(caster_ptr, GF_ELEC, dir, damage, 2);
		break;
	case MS_BALL_FIRE:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("ファイア・ボールの呪文を唱えた。", "You cast a fire ball."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_FIRE), plev, DAM_ROLL);
		fire_ball(caster_ptr, GF_FIRE, dir, damage, 2);
		break;
	case MS_BALL_COLD:
        if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("アイス・ボールの呪文を唱えた。", "You cast a frost ball."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_COLD), plev, DAM_ROLL);
		fire_ball(caster_ptr, GF_COLD, dir, damage, 2);
		break;
	case MS_BALL_POIS:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("悪臭雲の呪文を唱えた。", "You cast a stinking cloud."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_POIS), plev, DAM_ROLL);
		fire_ball(caster_ptr, GF_POIS, dir, damage, 2);
		break;
	case MS_BALL_NETHER:
        if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("地獄球の呪文を唱えた。", "You cast a nether ball."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_NETHER), plev, DAM_ROLL);
		fire_ball(caster_ptr, GF_NETHER, dir, damage, 2);
		break;
	case MS_BALL_WATER:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("流れるような身振りをした。", "You gesture fluidly."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_WATER), plev, DAM_ROLL);
		fire_ball(caster_ptr, GF_WATER, dir, damage, 4);
		break;
	case MS_BALL_MANA:
        if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("魔力の嵐の呪文を念じた。", "You invoke a mana storm."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_MANA), plev, DAM_ROLL);
		fire_ball(caster_ptr, GF_MANA, dir, damage, 4);
		break;
	case MS_BALL_DARK:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("暗黒の嵐の呪文を念じた。", "You invoke a darkness storm."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BALL_DARK), plev, DAM_ROLL);
		fire_ball(caster_ptr, GF_DARK, dir, damage, 4);
		break;
	case MS_DRAIN_MANA:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        damage = monspell_bluemage_damage(caster_ptr, (MS_DRAIN_MANA), plev, DAM_ROLL);
        fire_ball_hide(caster_ptr, GF_DRAIN_MANA, dir, damage, 0);
		break;
	case MS_MIND_BLAST:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        damage = monspell_bluemage_damage(caster_ptr, (MS_MIND_BLAST), plev, DAM_ROLL);
		fire_ball_hide(caster_ptr, GF_MIND_BLAST, dir, damage, 0);
		break;
	case MS_BRAIN_SMASH:
        if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        damage = monspell_bluemage_damage(caster_ptr, (MS_BRAIN_SMASH), plev, DAM_ROLL);
		fire_ball_hide(caster_ptr, GF_BRAIN_SMASH, dir, damage, 0);
		break;
	case MS_CAUSE_1:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        damage = monspell_bluemage_damage(caster_ptr, (MS_CAUSE_1), plev, DAM_ROLL);
		fire_ball_hide(caster_ptr, GF_CAUSE_1, dir, damage, 0);
		break;
	case MS_CAUSE_2:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        damage = monspell_bluemage_damage(caster_ptr, (MS_CAUSE_2), plev, DAM_ROLL);
		fire_ball_hide(caster_ptr, GF_CAUSE_2, dir, damage, 0);
		break;
	case MS_CAUSE_3:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        damage = monspell_bluemage_damage(caster_ptr, (MS_CAUSE_3), plev, DAM_ROLL);
		fire_ball_hide(caster_ptr, GF_CAUSE_3, dir, damage, 0);
		break;
	case MS_CAUSE_4:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        damage = monspell_bluemage_damage(caster_ptr, (MS_CAUSE_4), plev, DAM_ROLL);
		fire_ball_hide(caster_ptr, GF_CAUSE_4, dir, damage, 0);
		break;
	case MS_BOLT_ACID:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("アシッド・ボルトの呪文を唱えた。", "You cast an acid bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_ACID), plev, DAM_ROLL);
        fire_bolt(caster_ptr, GF_ACID, dir, damage);
		break;
	case MS_BOLT_ELEC:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("サンダー・ボルトの呪文を唱えた。", "You cast a lightning bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_ELEC), plev, DAM_ROLL);
		fire_bolt(caster_ptr, GF_ELEC, dir, damage);
		break;
	case MS_BOLT_FIRE:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("ファイア・ボルトの呪文を唱えた。", "You cast a fire bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_FIRE), plev, DAM_ROLL);
		fire_bolt(caster_ptr, GF_FIRE, dir, damage);
		break;
	case MS_BOLT_COLD:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("アイス・ボルトの呪文を唱えた。", "You cast a frost bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_COLD), plev, DAM_ROLL);
		fire_bolt(caster_ptr, GF_COLD, dir, damage);
		break;
	case MS_STARBURST:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("スターバーストの呪文を念じた。", "You invoke a starburst."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_STARBURST), plev, DAM_ROLL);
		fire_ball(caster_ptr, GF_LITE, dir, damage, 4);
		break;
	case MS_BOLT_NETHER:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("地獄の矢の呪文を唱えた。", "You cast a nether bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_NETHER), plev, DAM_ROLL);
		fire_bolt(caster_ptr, GF_NETHER, dir, damage);
		break;
	case MS_BOLT_WATER:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("ウォーター・ボルトの呪文を唱えた。", "You cast a water bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_WATER), plev, DAM_ROLL);
		fire_bolt(caster_ptr, GF_WATER, dir, damage);
		break;
	case MS_BOLT_MANA:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("魔力の矢の呪文を唱えた。", "You cast a mana bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_MANA), plev, DAM_ROLL);
		fire_bolt(caster_ptr, GF_MANA, dir, damage);
		break;
	case MS_BOLT_PLASMA:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("プラズマ・ボルトの呪文を唱えた。", "You cast a plasma bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_PLASMA), plev, DAM_ROLL);
		fire_bolt(caster_ptr, GF_PLASMA, dir, damage);
		break;
	case MS_BOLT_ICE:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("極寒の矢の呪文を唱えた。", "You cast a ice bolt."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_BOLT_ICE), plev, DAM_ROLL);
		fire_bolt(caster_ptr, GF_ICE, dir, damage);
		break;
	case MS_MAGIC_MISSILE:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("マジック・ミサイルの呪文を唱えた。", "You cast a magic missile."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_MAGIC_MISSILE), plev, DAM_ROLL);
		fire_bolt(caster_ptr, GF_MISSILE, dir, damage);
		break;
	case MS_SCARE:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("恐ろしげな幻覚を作り出した。", "You cast a fearful illusion."));
		fear_monster(caster_ptr, dir, plev+10);
		break;
	case MS_BLIND:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;
		confuse_monster(caster_ptr, dir, plev * 2);
		break;
	case MS_CONF:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("誘惑的な幻覚をつくり出した。", "You cast a mesmerizing illusion."));
		confuse_monster(caster_ptr, dir, plev * 2);
		break;
	case MS_SLOW:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;
		slow_monster(caster_ptr, dir, plev);
		break;
	case MS_SLEEP:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;
		sleep_monster(caster_ptr, dir, plev);
		break;
	case MS_SPEED:
		(void)set_fast(caster_ptr, randint1(20 + plev) + plev, FALSE);
		break;
	case MS_HAND_DOOM:
	{
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("<破滅の手>を放った！", "You invoke the Hand of Doom!"));
		fire_ball_hide(caster_ptr, GF_HAND_DOOM, dir, plev * 3, 0);
		break;
	}
	case MS_HEAL:
        msg_print(_("自分の傷に念を集中した。", "You concentrate on your wounds!"));
		(void)hp_player(caster_ptr, plev*4);
		(void)set_stun(caster_ptr, 0);
		(void)set_cut(caster_ptr,0);
		break;
	case MS_INVULNER:
        msg_print(_("無傷の球の呪文を唱えた。", "You cast a Globe of Invulnerability."));
		(void)set_invuln(caster_ptr, randint1(4) + 4, FALSE);
		break;
	case MS_BLINK:
		teleport_player(caster_ptr, 10, TELEPORT_SPONTANEOUS);
		break;
	case MS_TELEPORT:
		teleport_player(caster_ptr, plev * 5, TELEPORT_SPONTANEOUS);
		break;
	case MS_WORLD:
		(void)time_walk(caster_ptr);
		break;
	case MS_SPECIAL:
		break;
	case MS_TELE_TO:
	{
		monster_type *m_ptr;
		monster_race *r_ptr;
		GAME_TEXT m_name[MAX_NLEN];

		if (!target_set(caster_ptr, TARGET_KILL)) return FALSE;
		if (!floor_ptr->grid_array[target_row][target_col].m_idx) break;
		if (!player_has_los_bold(caster_ptr, target_row, target_col)) break;
		if (!projectable(caster_ptr, caster_ptr->y, caster_ptr->x, target_row, target_col)) break;
		m_ptr = &floor_ptr->m_list[floor_ptr->grid_array[target_row][target_col].m_idx];
		r_ptr = &r_info[m_ptr->r_idx];
		monster_desc(caster_ptr, m_name, m_ptr, 0);
		if (r_ptr->flagsr & RFR_RES_TELE)
		{
			if ((r_ptr->flags1 & (RF1_UNIQUE)) || (r_ptr->flagsr & RFR_RES_ALL))
			{
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
                msg_format(_("%sには効果がなかった！", "%s is unaffected!"), m_name);
				break;
			}
			else if (r_ptr->level > randint1(100))
			{
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
                msg_format(_("%sには耐性がある！", "%s resists!"), m_name);
				break;
			}
		}

        msg_format(_("%sを引き戻した。", "You command %s to return."), m_name);
		teleport_monster_to(caster_ptr, floor_ptr->grid_array[target_row][target_col].m_idx, caster_ptr->y, caster_ptr->x, 100, TELEPORT_PASSIVE);
		break;
	}
	case MS_TELE_AWAY:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

		(void)fire_beam(caster_ptr, GF_AWAY_ALL, dir, 100);
		break;

	case MS_TELE_LEVEL:
		return teleport_level_other(caster_ptr);
		break;

	case MS_PSY_SPEAR:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

        msg_print(_("光の剣を放った。", "You throw a psycho-spear."));
        damage = monspell_bluemage_damage(caster_ptr, (MS_PSY_SPEAR), plev, DAM_ROLL);
		(void)fire_beam(caster_ptr, GF_PSY_SPEAR, dir, damage);
		break;
	case MS_DARKNESS:

        msg_print(_("暗闇の中で手を振った。", "You gesture in shadow."));
		(void)unlite_area(caster_ptr, 10, 3);
		break;
	case MS_MAKE_TRAP:
		if (!target_set(caster_ptr, TARGET_KILL)) return FALSE;

        msg_print(_("呪文を唱えて邪悪に微笑んだ。", "You cast a spell and cackle evilly."));
		trap_creation(caster_ptr, target_row, target_col);
		break;
	case MS_FORGET:
        msg_print(_("しかし何も起きなかった。", "Nothing happen."));
		break;
    case MS_RAISE_DEAD:
        msg_print(_("死者復活の呪文を唱えた。", "You animate the dead."));
		(void)animate_dead(caster_ptr, 0, caster_ptr->y, caster_ptr->x);
		break;
	case MS_S_KIN:
	{
        msg_print(_("援軍を召喚した。", "You summon one of your kin."));
		for (int k = 0;k < 1; k++)
		{
			if (summon_kin_player(caster_ptr, summon_lev, caster_ptr->y, caster_ptr->x, (pet ? PM_FORCE_PET : 0L)))
			{
				if (!pet) msg_print(_("召喚された仲間は怒っている！", "The summoned companion is angry!"));
			}
			else
			{
				no_trump = TRUE;
			}
		}

		break;
	}
	case MS_S_CYBER:
	{
        msg_print(_("サイバーデーモンを召喚した！", "You summon a Cyberdemon!"));
		for (int k = 0; k < 1; k++)
		{
			if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_CYBER, p_mode))
			{
				if (!pet)
					msg_print(_("召喚されたサイバーデーモンは怒っている！", "The summoned Cyberdemon are angry!"));
			}
			else
			{
				no_trump = TRUE;
			}
		}
		break;
	}
	case MS_S_MONSTER:
	{
        msg_print(_("仲間を召喚した。", "You summon help."));
		for (int k = 0; k < 1; k++)
		{
			if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, 0, p_mode))
			{
				if (!pet)
					msg_print(_("召喚されたモンスターは怒っている！", "The summoned monster is angry!"));
			}
			else
			{
				no_trump = TRUE;
			}
		}
		
		break;
	}
	case MS_S_MONSTERS:
	{
        msg_print(_("モンスターを召喚した！", "You summon monsters!"));
		for (int k = 0; k < plev / 15 + 2; k++)
		{
			if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, 0, (p_mode | u_mode)))
			{
				if (!pet)
					msg_print(_("召喚されたモンスターは怒っている！", "The summoned monsters are angry!"));
			}
			else
			{
				no_trump = TRUE;
			}
		}
		
		break;
	}
	case MS_S_ANT:
	{
        msg_print(_("アリを召喚した。", "You summon ants."));
		if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_ANT, (PM_ALLOW_GROUP | p_mode)))
		{
			if (!pet)
				msg_print(_("召喚されたアリは怒っている！", "The summoned ants are angry!"));
		}
		else
		{
			no_trump = TRUE;
		}
		break;
	}
	case MS_S_SPIDER:
	{
        msg_print(_("蜘蛛を召喚した。", "You summon spiders."));
		if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_SPIDER, (PM_ALLOW_GROUP | p_mode)))
		{
			if (!pet)
				msg_print(_("召喚された蜘蛛は怒っている！", "Summoned spiders are angry!"));
		}
		else
		{
			no_trump = TRUE;
		}

		break;
	}
	case MS_S_HOUND:
	{
        msg_print(_("ハウンドを召喚した。", "You summon hounds."));
		if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_HOUND, (PM_ALLOW_GROUP | p_mode)))
		{
			if (!pet)
				msg_print(_("召喚されたハウンドは怒っている！", "Summoned hounds are angry!"));
		}
		else
		{
			no_trump = TRUE;
		}

		break;
	}
	case MS_S_HYDRA:
	{
        msg_print(_("ヒドラを召喚した。", "You summon a hydras."));
		if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_HYDRA, (g_mode | p_mode)))
		{
			if (!pet)
				msg_print(_("召喚されたヒドラは怒っている！", "Summoned hydras are angry!"));
		}
		else
		{
			no_trump = TRUE;
		}

		break;
	}
	case MS_S_ANGEL:
	{
        msg_print(_("天使を召喚した！", "You summon an angel!"));
		if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_ANGEL, (g_mode | p_mode)))
		{
			if (!pet)
				msg_print(_("召喚された天使は怒っている！", "The summoned angel is angry!"));
		}
		else
		{
			no_trump = TRUE;
		}
		break;
	}
	case MS_S_DEMON:
	{
        msg_print(_("混沌の宮廷から悪魔を召喚した！", "You summon a demon from the Courts of Chaos!"));
		if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_DEMON, (g_mode | p_mode)))
		{
			if (!pet)
				msg_print(_("召喚されたデーモンは怒っている！", "The summoned demon is angry!"));
		}
		else
		{
			no_trump = TRUE;
		}
		break;
	}
	case MS_S_UNDEAD:
	{
        msg_print(_("アンデッドの強敵を召喚した！", "You summon an undead adversary!"));
		if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_UNDEAD, (g_mode | p_mode)))
		{
			if (!pet)
				msg_print(_("召喚されたアンデッドは怒っている！", "The summoned undead is angry!"));
		}
		else
		{
			no_trump = TRUE;
		}
		break;
	}
	case MS_S_DRAGON:
	{
        msg_print(_("ドラゴンを召喚した！", "You summon a dragon!"));
		if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_DRAGON, (g_mode | p_mode)))
		{
			if (!pet)
				msg_print(_("召喚されたドラゴンは怒っている！", "The summoned dragon is angry!"));
		}
		else
		{
			no_trump = TRUE;
		}
				break;
	}
	case MS_S_HI_UNDEAD:
	{
        msg_print(_("強力なアンデッドを召喚した！", "You summon a greater undead!"));
		if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_HI_UNDEAD, (g_mode | p_mode | u_mode)))
		{
			if (!pet)
				msg_print(_("召喚された上級アンデッドは怒っている！", "The summoned greater undead is angry!"));
		}
		else
		{
			no_trump = TRUE;
		}
		break;
	}
	case MS_S_HI_DRAGON:
	{
		msg_print(_("古代ドラゴンを召喚した！", "You summon an ancient dragon!"));
		if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_HI_DRAGON, (g_mode | p_mode | u_mode)))
		{
			if (!pet)
				msg_print(_("召喚された古代ドラゴンは怒っている！", "The summoned ancient dragon is angry!"));
		}
		else
		{
			no_trump = TRUE;
		}
		break;
	}
	case MS_S_AMBERITE:
	{
        msg_print(_("アンバーの王族を召喚した！", "You summon a Lord of Amber!"));
		if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_AMBERITES, (g_mode | p_mode | u_mode)))
		{
			if (!pet)
				msg_print(_("召喚されたアンバーの王族は怒っている！", "The summoned Lord of Amber is angry!"));
		}
		else
		{
			no_trump = TRUE;
		}
		break;
	}
	case MS_S_UNIQUE:
	{
		int k, count = 0;
		msg_print(_("特別な強敵を召喚した！", "You summon a special opponent!"));
		for (k = 0; k < 1; k++)
		{
			if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_UNIQUE, (g_mode | p_mode | PM_ALLOW_UNIQUE)))
			{
				count++;
				if (!pet)
					msg_print(_("召喚されたユニーク・モンスターは怒っている！", "The summoned special opponent is angry!"));
			}
		}

		for (k = count; k < 1; k++)
		{
			if (summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x, summon_lev, SUMMON_HI_UNDEAD, (g_mode | p_mode | PM_ALLOW_UNIQUE)))
			{
				count++;
				if (!pet)
					msg_print(_("召喚された上級アンデッドは怒っている！", "The summoned greater undead is angry!"));
			}
		}

		if (!count)
		{
			no_trump = TRUE;
		}

		break;
	}
	default:
		msg_print("hoge?");
	}

	if (no_trump)
    {
        msg_print(_("何も現れなかった。", "No one appeared."));
	}

	return TRUE;
}


/*!
 * @brief 青魔法コマンドのメインルーチン /
 * do_cmd_cast calls this function if the player's class is 'Blue-Mage'.
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
bool do_cmd_cast_learned(player_type *caster_ptr)
{
	SPELL_IDX n = 0;
	PERCENTAGE chance;
	PERCENTAGE minfail = 0;
	PLAYER_LEVEL plev = caster_ptr->lev;
	monster_power spell;
	bool cast;
	MANA_POINT need_mana;

	if (cmd_limit_confused(caster_ptr)) return FALSE;

	if (!get_learned_power(caster_ptr, &n)) return FALSE;

	spell = monster_powers[n];

	need_mana = mod_need_mana(caster_ptr, spell.smana, 0, REALM_NONE);

	/* Verify "dangerous" spells */
	if (need_mana > caster_ptr->csp)
	{
		/* Warning */
		msg_print(_("ＭＰが足りません。", "You do not have enough mana to use this power."));

		if (!over_exert) return FALSE;

		/* Verify */
		if (!get_check(_("それでも挑戦しますか? ", "Attempt it anyway? "))) return FALSE;
	}

	/* Spell failure chance */
	chance = spell.fail;

	/* Reduce failure rate by "effective" level adjustment */
	if (plev > spell.level) chance -= 3 * (plev - spell.level);
	else chance += (spell.level - plev);

	/* Reduce failure rate by INT/WIS adjustment */
	chance -= 3 * (adj_mag_stat[caster_ptr->stat_ind[A_INT]] - 1);

	chance = mod_spell_chance_1(caster_ptr, chance);

	/* Not enough mana to cast */
	if (need_mana > caster_ptr->csp)
	{
		chance += 5 * (need_mana - caster_ptr->csp);
	}

	/* Extract the minimum failure rate */
	minfail = adj_mag_fail[caster_ptr->stat_ind[A_INT]];

	/* Minimum failure rate */
	if (chance < minfail) chance = minfail;

	/* Stunning makes spells harder */
	if (caster_ptr->stun > 50) chance += 25;
	else if (caster_ptr->stun) chance += 15;

	/* Always a 5 percent chance of working */
	if (chance > 95) chance = 95;

	chance = mod_spell_chance_2(caster_ptr, chance);

	/* Failed spell */
	if (randint0(100) < chance)
	{
		if (flush_failure) flush();
		msg_print(_("魔法をうまく唱えられなかった。", "You failed to concentrate hard enough!"));

		sound(SOUND_FAIL);

		if (n >= MS_S_KIN)
			cast = cast_learned_spell(caster_ptr, n, FALSE);
	}
	else
	{
		sound(SOUND_ZAP);
		cast = cast_learned_spell(caster_ptr, n, TRUE);
		if (!cast) return FALSE;
	}

	/* Sufficient mana */
	if (need_mana <= caster_ptr->csp)
	{
		/* Use some mana */
		caster_ptr->csp -= need_mana;
	}
	else
	{
		int oops = need_mana;

		/* No mana left */
		caster_ptr->csp = 0;
		caster_ptr->csp_frac = 0;

		msg_print(_("精神を集中しすぎて気を失ってしまった！", "You faint from the effort!"));

		/* Hack -- Bypass free action */
		(void)set_paralyzed(caster_ptr, caster_ptr->paralyzed + randint1(5 * oops + 1));

		chg_virtue(caster_ptr, V_KNOWLEDGE, -10);

		/* Damage CON (possibly permanently) */
		if (randint0(100) < 50)
		{
			bool perm = (randint0(100) < 25);

			msg_print(_("体を悪くしてしまった！", "You have damaged your health!"));

			/* Reduce constitution */
			(void)dec_stat(caster_ptr, A_CON, 15 + randint1(10), perm);
		}
	}

	take_turn(caster_ptr, 100);

	caster_ptr->redraw |= (PR_MANA);
	caster_ptr->window |= (PW_PLAYER | PW_SPELL);

	return TRUE;
}


/*!
 * @brief 青魔法のラーニング判定と成功した場合のラーニング処理
 * @param monspell ラーニングを試みるモンスター攻撃のID
 * @return なし
 */
void learn_spell(player_type *learner_ptr, int monspell)
{
	if (learner_ptr->action != ACTION_LEARN) return;
	if (monspell < 0) return;
	if (learner_ptr->magic_num2[monspell]) return;
	if (learner_ptr->confused || learner_ptr->blind || learner_ptr->image || learner_ptr->stun || learner_ptr->paralyzed) return;
	if (randint1(learner_ptr->lev + 70) > monster_powers[monspell].level + 40)
	{
		learner_ptr->magic_num2[monspell] = 1;
		msg_format(_("%sを学習した！", "You have learned %s!"), monster_powers[monspell].name);
		gain_exp(learner_ptr, monster_powers[monspell].level * monster_powers[monspell].smana);

		sound(SOUND_STUDY);

		learner_ptr->new_mane = TRUE;
		learner_ptr->redraw |= (PR_STATE);
	}
}


/*!
 * todo f4, f5, f6を構造体にまとめ直す
 * @brief モンスター特殊能力のフラグ配列から特定条件の魔法だけを抜き出す処理
 * Extract monster spells mask for the given mode
 * @param f4 モンスター特殊能力の4番目のフラグ配列
 * @param f5 モンスター特殊能力の5番目のフラグ配列
 * @param f6 モンスター特殊能力の6番目のフラグ配列
 * @param mode 抜き出したい条件
 * @return なし
 */
/*
 */
void set_rf_masks(BIT_FLAGS *f4, BIT_FLAGS *f5, BIT_FLAGS *f6, BIT_FLAGS mode)
{
	switch (mode)
	{
		case MONSPELL_TYPE_BOLT:
			*f4 = ((RF4_BOLT_MASK | RF4_BEAM_MASK) & ~(RF4_ROCKET));
			*f5 = RF5_BOLT_MASK | RF5_BEAM_MASK;
			*f6 = RF6_BOLT_MASK | RF6_BEAM_MASK;
			break;

		case MONSPELL_TYPE_BALL:
			*f4 = (RF4_BALL_MASK & ~(RF4_BREATH_MASK));
			*f5 = (RF5_BALL_MASK & ~(RF5_BREATH_MASK));
			*f6 = (RF6_BALL_MASK & ~(RF6_BREATH_MASK));
			break;

		case MONSPELL_TYPE_BREATH:
			*f4 = RF4_BREATH_MASK;
			*f5 = RF5_BREATH_MASK;
			*f6 = RF6_BREATH_MASK;
			break;

		case MONSPELL_TYPE_SUMMON:
			*f4 = RF4_SUMMON_MASK;
			*f5 = RF5_SUMMON_MASK;
			*f6 = RF6_SUMMON_MASK;
			break;

		case MONSPELL_TYPE_OTHER:
			*f4 = RF4_ATTACK_MASK & ~(RF4_BOLT_MASK | RF4_BEAM_MASK | RF4_BALL_MASK | RF4_INDIRECT_MASK);
			*f5 = RF5_ATTACK_MASK & ~(RF5_BOLT_MASK | RF5_BEAM_MASK | RF5_BALL_MASK | RF5_INDIRECT_MASK);
			*f6 = RF6_ATTACK_MASK & ~(RF6_BOLT_MASK | RF6_BEAM_MASK | RF6_BALL_MASK | RF6_INDIRECT_MASK);
			break;
	}
}
