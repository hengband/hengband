/*!
 * @file store.c
 * @brief 店の処理 / Store commands
 * @date 2014/02/02
 * @author
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research, and\n
 * not for profit purposes provided that this copyright and statement are\n
 * included in all such copies.\n
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "angband.h"
#include "core.h"
#include "util.h"
#include "term.h"

#include "floor.h"
#include "cmd-basic.h"
#include "cmd-dump.h"
#include "cmd-item.h"
#include "cmd-smith.h"
#include "cmd-zapwand.h"
#include "cmd-magiceat.h"
#include "spells.h"
#include "store.h"
#include "avatar.h"
#include "cmd-spell.h"
#include "rumor.h"
#include "player-status.h"
#include "player-class.h"
#include "player-inventory.h"
#include "object-flavor.h"
#include "object-hook.h"
#include "floor-events.h"
#include "snipe.h"
#include "files.h"
#include "player-effects.h"
#include "player-race.h"
#include "mind.h"
#include "world.h"
#include "objectkind.h"
#include "autopick.h"
#include "floor-town.h"
#include "japanese.h"
#include "view-mainwindow.h"
#include "wild.h"

#define MIN_STOCK 12

 /*!
  * @brief 闘技場のモンスターID及び報酬アイテムテーブル /
  * Store owners (exactly four "possible" owners per store, chosen randomly)
  * @details
  * <pre>
  * { name, purse, max greed, min greed, haggle_per, tolerance, race, unused }
  *
  * Lifted extra shopkeepers from CthAngband (don't you just love open source
  * development? ;-)). Since this gave less than 32 unique names for some
  * shops, those have their first x names copied to reach 32.
  *
  * For the weapon and armour shops, several owners have a limit of 5k.
  *
  * I want to do 50k owners, but the purse is currently s16b. Perhaps
  * we should just store 1/10th of the purse?
  * </pre>
  */
const owner_type owners[MAX_STORES][MAX_OWNERS] =
{
	{
		/* General store - 32 unique names */
		/*
		  Raistlin は dragonlance の powerful wizard 。
		  Rincewind the Chicken は Terry Pratchett の Discworld の登場人物 上記のパロディ？、
		  { "憶病者ラストリン",       200,    175,  108,   4,  12,  RACE_HUMAN},
		  { "Raistlin the Chicken",       200,    175, 108,  4, 12, RACE_HUMAN},
		*/

#ifdef JP
		{ "フレンドリーなビルボ",       200,    170,  108,   5,  15,  RACE_HOBBIT},
		{ "憶病者リンスウィンド",       200,    175,  108,   4,  12,  RACE_HUMAN},
		{ "背の低いサルタン",             300,    170,  107,   5,  15,  RACE_GNOME},
		{ "ハンサムなライア=エル",      300,    165,  107,   6,  18,  RACE_ELF},
		{ "親切なファリルマウエン",         250,    170, 108,  5, 15, RACE_HOBBIT},
		{ "臆病者ヴォワラン",       500,    175, 108,  4, 12, RACE_HUMAN},
		{ "チビのエラシュナク",          750,    170, 107,  5, 15, RACE_BEASTMAN},
		{ "ハンサムなグラッグ",        1000,    165, 107,  6, 18, RACE_HALF_TITAN},
		{ "ケチなフォロビア",         250,    170, 108,  5, 15, RACE_HUMAN},
		{ "馬鹿のエリス",       500,    175, 108,  4, 12, RACE_HUMAN},
		{ "腹ペコのフィルバート",          750,    170, 107,  5, 15, RACE_VAMPIRE},
		{ "スナーグル・サシグア",        1000,    165, 107,  6, 18, RACE_MIND_FLAYER},
		{ "長死きエロワーズ",         250,    170, 108,  5, 15, RACE_SPECTRE},
		{ "ノロマのフンディ",       500,    175, 108,  4, 12, RACE_ZOMBIE},
		{ "グランサス",          750,    170, 107,  5, 15, RACE_SKELETON},
		{ "丁寧なロラックス",        1000,    165, 107,  6, 18, RACE_VAMPIRE},
		{ "ブッチ",         250,    170, 108,  5, 15, RACE_HALF_ORC},
		{ "美しきエルベレス",       500,    175, 108,  4, 12, RACE_HIGH_ELF},
		{ "こそこそサーレス",          750,    170, 107,  5, 15, RACE_GNOME},
		{ "ナーロック",        1000,    165, 107,  6, 18, RACE_DWARF},
		{ "チビのヘイネッカ",         250,    170, 108,  5, 15, RACE_GNOME},
		{ "きちがいロワラン",       500,    175, 108,  4, 12, RACE_HALF_GIANT},
		{ "毒息のウート",          750,    170, 107,  5, 15, RACE_DRACONIAN},
		{ "でぶっちょアラァカ",        1000,    165, 107,  6, 18, RACE_DRACONIAN},
		{ "低能なプーゴー",         250,    170, 108,  5, 15, RACE_BEASTMAN},
		{ "フェロールフィリアン",       500,    175, 108,  4, 12, RACE_ELF},
		{ "年寄りマロカ",          750,    170, 107,  5, 15, RACE_GNOME},
		{ "勇敢なサシン",        1000,    165, 107,  6, 18, RACE_HALF_GIANT},
		{ "田舎者アビエマール",         250,    170, 108,  5, 15, RACE_HUMAN},
		{ "貧乏なハーク",       500,    175, 108,  4, 12, RACE_HALF_ORC},
		{ "みじめなソアリン",          750,    170, 107,  5, 15, RACE_ZOMBIE},
		{ "質素なメルラ",        1000,    165, 107,  6, 18, RACE_ELF},
#else
		{ "Bilbo the Friendly",         200,    170, 108,  5, 15, RACE_HOBBIT},
		{ "Rincewind the Chicken",       200,    175, 108,  4, 12, RACE_HUMAN},
		{ "Sultan the Midget",          300,    170, 107,  5, 15, RACE_GNOME},
		{ "Lyar-el the Comely",         300,    165, 107,  6, 18, RACE_ELF},
		{ "Falilmawen the Friendly",         250,    170, 108,  5, 15, RACE_HOBBIT},
		{ "Voirin the Cowardly",       500,    175, 108,  4, 12, RACE_HUMAN},
		{ "Erashnak the Midget",          750,    170, 107,  5, 15, RACE_BEASTMAN},
		{ "Grug the Comely",        1000,    165, 107,  6, 18, RACE_HALF_TITAN},
		{ "Forovir the Cheap",         250,    170, 108,  5, 15, RACE_HUMAN},
		{ "Ellis the Fool",       500,    175, 108,  4, 12, RACE_HUMAN},
		{ "Filbert the Hungry",          750,    170, 107,  5, 15, RACE_VAMPIRE},
		{ "Fthnargl Psathiggua",        1000,    165, 107,  6, 18, RACE_MIND_FLAYER},
		{ "Eloise Long-Dead",         250,    170, 108,  5, 15, RACE_SPECTRE},
		{ "Fundi the Slow",       500,    175, 108,  4, 12, RACE_ZOMBIE},
		{ "Granthus",          750,    170, 107,  5, 15, RACE_SKELETON},
		{ "Lorax the Suave",        1000,    165, 107,  6, 18, RACE_VAMPIRE},
		{ "Butch",         250,    170, 108,  5, 15, RACE_HALF_ORC},
		{ "Elbereth the Beautiful",       500,    175, 108,  4, 12, RACE_HIGH_ELF},
		{ "Sarleth the Sneaky",          750,    170, 107,  5, 15, RACE_GNOME},
		{ "Narlock",        1000,    165, 107,  6, 18, RACE_DWARF},
		{ "Haneka the Small",         250,    170, 108,  5, 15, RACE_GNOME},
		{ "Loirin the Mad",       500,    175, 108,  4, 12, RACE_HALF_GIANT},
		{ "Wuto Poisonbreath",          750,    170, 107,  5, 15, RACE_DRACONIAN},
		{ "Araaka the Rotund",        1000,    165, 107,  6, 18, RACE_DRACONIAN},
		{ "Poogor the Dumb",         250,    170, 108,  5, 15, RACE_BEASTMAN},
		{ "Felorfiliand",       500,    175, 108,  4, 12, RACE_ELF},
		{ "Maroka the Aged",          750,    170, 107,  5, 15, RACE_GNOME},
		{ "Sasin the Bold",        1000,    165, 107,  6, 18, RACE_HALF_GIANT},
		{ "Abiemar the Peasant",         250,    170, 108,  5, 15, RACE_HUMAN},
		{ "Hurk the Poor",       500,    175, 108,  4, 12, RACE_HALF_ORC},
		{ "Soalin the Wretched",          750,    170, 107,  5, 15, RACE_ZOMBIE},
		{ "Merulla the Humble",        1000,    165, 107,  6, 18, RACE_ELF},
#endif
	},
	{
		/* Armoury - 28 unique names */
#ifdef JP
		{ "醜悪コン=ダー",      5000,   210,  115,   5,   7,  RACE_HALF_ORC},
		{ "頑固者ダーグ=ロウ",  10000,  190,  111,   4,   9,  RACE_HUMAN},
		{ "賢者デカド",                 25000,  200,  112,   4,  10,  RACE_DUNADAN},
		{ "鍛冶屋のウィーランド",   30000,  200,  112,   4,   5,  RACE_DWARF},
		{ "醜悪コン=ダー",           10000,   210, 115,  5,  7, RACE_HALF_ORC},
		{ "頑固者ダーグ=ロウ",          15000,  190, 111,  4,  9, RACE_HUMAN},
		{ "ハンサムなデカド",            25000,  200, 112,  4, 10, RACE_AMBERITE},
		{ "エロー・ドラゴンスケイル",          30000,  200, 112,  4,  5, RACE_ELF},
		{ "デリカトス",           10000,   210, 115,  5,  7, RACE_SPRITE},
		{ "巨大なグルース",          15000,  190, 111,  4,  9, RACE_HALF_GIANT},
		{ "アニムス",            25000,  200, 112,  4, 10, RACE_GOLEM},
		{ "マルヴァス",          30000,  200, 112,  4,  5, RACE_HALF_TITAN},
		{ "セラクシス",           10000,   210, 115,  5,  7, RACE_ZOMBIE},
		{ "デス・チル",          5000,  190, 111,  4,  9, RACE_SPECTRE},
		{ "微かなドリオス",            25000,  200, 112,  4, 10, RACE_SPECTRE},
		{ "冷たいバスリック",          30000,  200, 112,  4,  5, RACE_VAMPIRE},
		{ "冷酷ヴェンジェラ",           10000,   210, 115,  5,  7, RACE_HALF_TROLL},
		{ "強者ウィラナ",          15000,  190, 111,  4,  9, RACE_HUMAN},
		{ "ヨジョ二世",            25000,  200, 112,  4, 10, RACE_DWARF},
		{ "優しいラナラー",          30000,  200, 112,  4,  5, RACE_AMBERITE},
		{ "不浄のホルバグ",           5000,   210, 115,  5,  7, RACE_HALF_ORC},
		{ "テレパスのエレレン",          15000,  190, 111,  4,  9, RACE_DARK_ELF},
		{ "イスドリリアス",            25000,  200, 112,  4, 10, RACE_SPRITE},
		{ "一つ目ヴェグナー",          5000,  200, 112,  4,  5, RACE_CYCLOPS},
		{ "混沌のロディッシュ",           10000,   210, 115,  5,  7, RACE_BEASTMAN},
		{ "剣豪ヘジン",          15000,  190, 111,  4,  9, RACE_NIBELUNG},
		{ "ずる屋のエルベレリス",           10000,  200, 112,  4, 10, RACE_DARK_ELF},
		{ "インプのザサス",          30000,  200, 112,  4,  5, RACE_IMP},
		{ "醜悪コン=ダー",           5000,   210, 115,  5,  7, RACE_HALF_ORC},
		{ "頑固者ダーグ=ロウ",          10000,  190, 111,  4,  9, RACE_HUMAN},
		{ "ハンサムなデカド",            25000,  200, 112,  4, 10, RACE_AMBERITE},
		{ "鍛冶屋のウィーランド",          30000,  200, 112,  4,  5, RACE_DWARF},
#else
		{ "Kon-Dar the Ugly",           5000,   210, 115,  5,  7, RACE_HALF_ORC},
		{ "Darg-Low the Grim",          10000,  190, 111,  4,  9, RACE_HUMAN},
		{ "Decado the Handsome",            25000,  200, 112,  4, 10, RACE_DUNADAN},
		{ "Wieland the Smith",          30000,  200, 112,  4,  5, RACE_DWARF},
		{ "Kon-Dar the Ugly",           10000,   210, 115,  5,  7, RACE_HALF_ORC},
		{ "Darg-Low the Grim",          15000,  190, 111,  4,  9, RACE_HUMAN},
		{ "Decado the Handsome",            25000,  200, 112,  4, 10, RACE_AMBERITE},
		{ "Elo Dragonscale",          30000,  200, 112,  4,  5, RACE_ELF},
		{ "Delicatus",           10000,   210, 115,  5,  7, RACE_SPRITE},
		{ "Gruce the Huge",          15000,  190, 111,  4,  9, RACE_HALF_GIANT},
		{ "Animus",            25000,  200, 112,  4, 10, RACE_GOLEM},
		{ "Malvus",          30000,  200, 112,  4,  5, RACE_HALF_TITAN},
		{ "Selaxis",           10000,   210, 115,  5,  7, RACE_ZOMBIE},
		{ "Deathchill",          5000,  190, 111,  4,  9, RACE_SPECTRE},
		{ "Drios the Faint",            25000,  200, 112,  4, 10, RACE_SPECTRE},
		{ "Bathric the Cold",          30000,  200, 112,  4,  5, RACE_VAMPIRE},
		{ "Vengella the Cruel",           10000,   210, 115,  5,  7, RACE_HALF_TROLL},
		{ "Wyrana the Mighty",          15000,  190, 111,  4,  9, RACE_HUMAN},
		{ "Yojo II",            25000,  200, 112,  4, 10, RACE_DWARF},
		{ "Ranalar the Sweet",          30000,  200, 112,  4,  5, RACE_AMBERITE},
		{ "Horbag the Unclean",           5000,   210, 115,  5,  7, RACE_HALF_ORC},
		{ "Elelen the Telepath",          15000,  190, 111,  4,  9, RACE_DARK_ELF},
		{ "Isedrelias",            25000,  200, 112,  4, 10, RACE_SPRITE},
		{ "Vegnar One-eye",          5000,  200, 112,  4,  5, RACE_CYCLOPS},
		{ "Rodish the Chaotic",           10000,   210, 115,  5,  7, RACE_BEASTMAN},
		{ "Hesin Swordmaster",          15000,  190, 111,  4,  9, RACE_NIBELUNG},
		{ "Elvererith the Cheat",           10000,  200, 112,  4, 10, RACE_DARK_ELF},
		{ "Zzathath the Imp",          30000,  200, 112,  4,  5, RACE_IMP},
		{ "Kon-Dar the Ugly",           5000,   210, 115,  5,  7, RACE_HALF_ORC},
		{ "Darg-Low the Grim",          10000,  190, 111,  4,  9, RACE_HUMAN},
		{ "Decado the Handsome",            25000,  200, 112,  4, 10, RACE_AMBERITE},
		{ "Wieland the Smith",          30000,  200, 112,  4,  5, RACE_DWARF},
#endif
	},

	{
		/* Weapon Smith - 28 unique names */
#ifdef JP
		{ "残忍なるアーノルド",        5000,   210,  115,   6,   6,  RACE_HALF_TROLL},
		{ "獣殺しのアーンダル", 10000,  185,  110,   5,   9,  RACE_HALF_ELF},
		{ "獣マスターのエディー", 25000,  190,  115,   5,   7,  RACE_HOBBIT},
		{ "竜殺しのオグライン", 30000,  195,  112,   4,   8,  RACE_DWARF},
		{ "熟練者ドリュー",      10000,   210, 115,  6,  6, RACE_HUMAN},
		{ "龍の子オラックス",        15000,  185, 110,  5,  9, RACE_DRACONIAN},
		{ "病気持ちのアンスラックス",         25000,  190, 115,  5,  7, RACE_BEASTMAN},
		{ "頑丈者アルコス",       30000,  195, 112,  4,  8, RACE_DWARF},
		{ "腐れ者のサリアス",      5000,   210, 115,  6,  6, RACE_ZOMBIE},
		{ "晒し骨のトゥエシク",        15000,  185, 110,  5,  9, RACE_SKELETON},
		{ "ビリオス",         25000,  190, 115,  5,  7, RACE_BEASTMAN},
		{ "ファスガル",       30000,  195, 112,  4,  8, RACE_ZOMBIE},
		{ "パラディンのエレフリス",      10000,   210, 115,  6,  6, RACE_BARBARIAN},
		{ "キ'トリッ'ク",        15000,  185, 110,  5,  9, RACE_KLACKON},
		{ "蜘蛛の友ドゥロカス",         25000,  190, 115,  5,  7, RACE_DARK_ELF},
		{ "巨人殺しのフングス",       30000,  195, 112,  4,  8, RACE_DWARF},
		{ "デランサ",      10000,   210, 115,  6,  6, RACE_ELF},
		{ "レンジャーのソルビスタニ",        15000,  185, 110,  5,  9, RACE_HALF_ELF},
		{ "ノロマのゾリル",         25000,  190, 115,  5,  7, RACE_GOLEM},
		{ "イーオン・フラックス",       20000,  195, 112,  4,  8, RACE_HALF_ELF},
		{ "強者ナドック",      10000,   210, 115,  6,  6, RACE_HOBBIT},
		{ "弱虫エラモグ",        15000,  185, 110,  5,  9, RACE_KOBOLD},
		{ "公正なエオウィリス",         25000,  190, 115,  5,  7, RACE_VAMPIRE},
		{ "バルログ殺しのヒュイモグ",       30000,  195, 112,  4,  8, RACE_HALF_ORC},
		{ "冷酷ピーダス",      5000,   210, 115,  6,  6, RACE_HUMAN},
		{ "ヴァモグ スレイヤー",        15000,  185, 110,  5,  9, RACE_HALF_OGRE},
		{ "性悪フーシュナク",         25000,  190, 115,  5,  7, RACE_BEASTMAN},
		{ "舞闘バレン",       30000,  195, 112,  4,  8, RACE_BARBARIAN},
		{ "残忍なるアーノルド",      5000,   210, 115,  6,  6, RACE_BARBARIAN},
		{ "獣殺しのアーンダル",        10000,  185, 110,  5,  9, RACE_HALF_ELF},
		{ "ビーストマスター・エディー",         25000,  190, 115,  5,  7, RACE_HALF_ORC},
		{ "竜殺しのオグライン",       30000,  195, 112,  4,  8, RACE_DWARF},
#else
		{ "Arnold the Beastly",      5000,   210, 115,  6,  6, RACE_BARBARIAN},
		{ "Arndal Beast-Slayer",        10000,  185, 110,  5,  9, RACE_HALF_ELF},
		{ "Eddie Beast-Master",         25000,  190, 115,  5,  7, RACE_HALF_ORC},
		{ "Oglign Dragon-Slayer",       30000,  195, 112,  4,  8, RACE_DWARF},
		{ "Drew the Skilled",      10000,   210, 115,  6,  6, RACE_HUMAN},
		{ "Orrax Dragonson",        15000,  185, 110,  5,  9, RACE_DRACONIAN},
		{ "Anthrax Disease-Carrier",         25000,  190, 115,  5,  7, RACE_BEASTMAN},
		{ "Arkhoth the Stout",       30000,  195, 112,  4,  8, RACE_DWARF},
		{ "Sarlyas the Rotten",      5000,   210, 115,  6,  6, RACE_ZOMBIE},
		{ "Tuethic Bare-Bones",        15000,  185, 110,  5,  9, RACE_SKELETON},
		{ "Bilious",         25000,  190, 115,  5,  7, RACE_BEASTMAN},
		{ "Fasgul",       30000,  195, 112,  4,  8, RACE_ZOMBIE},
		{ "Ellefris the Paladin",      10000,   210, 115,  6,  6, RACE_BARBARIAN},
		{ "K'trrik'k",        15000,  185, 110,  5,  9, RACE_KLACKON},
		{ "Drocus Spiderfriend",         25000,  190, 115,  5,  7, RACE_DARK_ELF},
		{ "Fungus Giant-Slayer",       30000,  195, 112,  4,  8, RACE_DWARF},
		{ "Delantha",      10000,   210, 115,  6,  6, RACE_ELF},
		{ "Solvistani the Ranger",        15000,  185, 110,  5,  9, RACE_HALF_ELF},
		{ "Xoril the Slow",         25000,  190, 115,  5,  7, RACE_GOLEM},
		{ "Aeon Flux",       20000,  195, 112,  4,  8, RACE_HALF_ELF},
		{ "Nadoc the Strong",      10000,   210, 115,  6,  6, RACE_HOBBIT},
		{ "Eramog the Weak",        15000,  185, 110,  5,  9, RACE_KOBOLD},
		{ "Eowilith the Fair",         25000,  190, 115,  5,  7, RACE_VAMPIRE},
		{ "Huimog Balrog-Slayer",       30000,  195, 112,  4,  8, RACE_HALF_ORC},
		{ "Peadus the Cruel",      5000,   210, 115,  6,  6, RACE_HUMAN},
		{ "Vamog Slayer",        15000,  185, 110,  5,  9, RACE_HALF_OGRE},
		{ "Hooshnak the Vicious",         25000,  190, 115,  5,  7, RACE_BEASTMAN},
		{ "Balenn War-Dancer",       30000,  195, 112,  4,  8, RACE_BARBARIAN},
		{ "Arnold the Beastly",      5000,   210, 115,  6,  6, RACE_BARBARIAN},
		{ "Arndal Beast-Slayer",        10000,  185, 110,  5,  9, RACE_HALF_ELF},
		{ "Eddie Beast-Master",         25000,  190, 115,  5,  7, RACE_HALF_ORC},
		{ "Oglign Dragon-Slayer",       30000,  195, 112,  4,  8, RACE_DWARF},
#endif
	},
	{
		/* Temple - 22 unique names */
#ifdef JP
		{ "質素なルードヴィヒ",         5000,   175,  109,   6,  15,  RACE_HUMAN},
		{ "パラディンのガンナー",       10000,  185,  110,   5,  23,  RACE_HUMAN},
		{ "選ばれしトリン",                     25000,  180,  107,   6,  20,  RACE_ELF},
		{ "賢明なるサラストロ",                     30000,  185,  109,   5,  15,  RACE_DWARF},
		{ "パーシヴァル卿",           25000,  180, 107,  6, 20, RACE_HIGH_ELF},
		{ "神聖なるアセナス",          30000,  185, 109,  5, 15, RACE_HUMAN},
		{ "マッキノン",         10000,   175, 109,  6, 15, RACE_HUMAN},
		{ "謹み婦人",         15000,  185, 110,  5, 23, RACE_HIGH_ELF},
		{ "ドルイドのハシュニック",           25000,  180, 107,  6, 20, RACE_HOBBIT},
		{ "フィナク",          30000,  185, 109,  5, 15, RACE_YEEK},
		{ "クリキック",         10000,   175, 109,  6, 15, RACE_KLACKON},
		{ "荒くれ者モリヴァル",         15000,  185, 110,  5, 23, RACE_ELF},
		{ "暗きホシャック",           25000,  180, 107,  6, 20, RACE_IMP},
		{ "賢者アタール",          30000,  185, 109,  5, 15, RACE_HUMAN},
		{ "清きイベニッド",         10000,   175, 109,  6, 15, RACE_HUMAN},
		{ "エリディシュ",         15000,  185, 110,  5, 23, RACE_HALF_TROLL},
		{ "呪術師ヴルドゥシュ",           25000,  180, 107,  6, 20, RACE_HALF_OGRE},
		{ "狂戦士ハオブ",          30000,  185, 109,  5, 15, RACE_BARBARIAN},
		{ "若きプルーグディシュ",         10000,   175, 109,  6, 15, RACE_HALF_OGRE},
		{ "きちがいラムワイズ",         15000,  185, 110,  5, 23, RACE_YEEK},
		{ "有徳者ムワート",           25000,  180, 107,  6, 20, RACE_KOBOLD},
		{ "弱虫ダードバード",          30000,  185, 109,  5, 15, RACE_SPECTRE},
		{ "質素なルードヴィヒ",         5000,   175,  109,   6,  15,  RACE_HUMAN},
		{ "パラディンのガンナー",       10000,  185,  110,   5,  23,  RACE_HUMAN},
		{ "選ばれしトリン",                     25000,  180,  107,   6,  20,  RACE_ELF},
		{ "賢明なるサラストロ",                     30000,  185,  109,   5,  15,  RACE_DWARF},
		{ "パーシヴァル卿",           25000,  180, 107,  6, 20, RACE_HIGH_ELF},
		{ "神聖なるアセナス",          30000,  185, 109,  5, 15, RACE_HUMAN},
		{ "マッキノン",         10000,   175, 109,  6, 15, RACE_HUMAN},
		{ "謹み婦人",         15000,  185, 110,  5, 23, RACE_HIGH_ELF},
		{ "ドルイドのハシュニック",           25000,  180, 107,  6, 20, RACE_HOBBIT},
		{ "フィナク",          30000,  185, 109,  5, 15, RACE_YEEK},
#else
		{ "Ludwig the Humble",          5000,   175, 109,  6, 15, RACE_DWARF},
		{ "Gunnar the Paladin",         10000,  185, 110,  5, 23, RACE_HALF_TROLL},
		{ "Torin the Chosen",           25000,  180, 107,  6, 20, RACE_HIGH_ELF},
		{ "Sarastro the Wise",          30000,  185, 109,  5, 15, RACE_HUMAN},
		{ "Sir Parsival the Pure",           25000,  180, 107,  6, 20, RACE_HIGH_ELF},
		{ "Asenath the Holy",          30000,  185, 109,  5, 15, RACE_HUMAN},
		{ "McKinnon",         10000,   175, 109,  6, 15, RACE_HUMAN},
		{ "Mistress Chastity",         15000,  185, 110,  5, 23, RACE_HIGH_ELF},
		{ "Hashnik the Druid",           25000,  180, 107,  6, 20, RACE_HOBBIT},
		{ "Finak",          30000,  185, 109,  5, 15, RACE_YEEK},
		{ "Krikkik",         10000,   175, 109,  6, 15, RACE_KLACKON},
		{ "Morival the Wild",         15000,  185, 110,  5, 23, RACE_ELF},
		{ "Hoshak the Dark",           25000,  180, 107,  6, 20, RACE_IMP},
		{ "Atal the Wise",          30000,  185, 109,  5, 15, RACE_HUMAN},
		{ "Ibenidd the Chaste",         10000,   175, 109,  6, 15, RACE_HUMAN},
		{ "Eridish",         15000,  185, 110,  5, 23, RACE_HALF_TROLL},
		{ "Vrudush the Shaman",           25000,  180, 107,  6, 20, RACE_HALF_OGRE},
		{ "Haob the Berserker",          30000,  185, 109,  5, 15, RACE_BARBARIAN},
		{ "Proogdish the Youthfull",         10000,   175, 109,  6, 15, RACE_HALF_OGRE},
		{ "Lumwise the Mad",         15000,  185, 110,  5, 23, RACE_YEEK},
		{ "Muirt the Virtuous",           25000,  180, 107,  6, 20, RACE_KOBOLD},
		{ "Dardobard the Weak",          30000,  185, 109,  5, 15, RACE_SPECTRE},
		{ "Ludwig the Humble",          5000,   175, 109,  6, 15, RACE_DWARF},
		{ "Gunnar the Paladin",         10000,  185, 110,  5, 23, RACE_HALF_TROLL},
		{ "Torin the Chosen",           25000,  180, 107,  6, 20, RACE_HIGH_ELF},
		{ "Sarastro the Wise",          30000,  185, 109,  5, 15, RACE_HUMAN},
		{ "Sir Parsival the Pure",           25000,  180, 107,  6, 20, RACE_HIGH_ELF},
		{ "Asenath the Holy",          30000,  185, 109,  5, 15, RACE_HUMAN},
		{ "McKinnon",         10000,   175, 109,  6, 15, RACE_HUMAN},
		{ "Mistress Chastity",         15000,  185, 110,  5, 23, RACE_HIGH_ELF},
		{ "Hashnik the Druid",           25000,  180, 107,  6, 20, RACE_HOBBIT},
		{ "Finak",          30000,  185, 109,  5, 15, RACE_YEEK},
#endif
	},
	{
		/* Alchemist - 26 unique names */
#ifdef JP
		{ "化学者マウザー",             10000,  190,  111,   5,   8,  RACE_HALF_ELF},
		{ "カオスのウィズル",   10000,  190,  110,   6,   8,  RACE_HOBBIT},
		{ "強欲ミダス",              15000,  200,  116,   6,   9,  RACE_GNOME},
		{ "貧弱ジャ=ファー",                   15000,  220,  111,   4,   9,  RACE_ELF},/*FIRST*/
		{ "カカルラカカル",           15000,  200, 116,  6,  9, RACE_KLACKON},
		{ "錬金術師ジャル=エス",       15000,  220, 111,  4,  9, RACE_ELF},
		{ "用心深いファネラス",         10000,  190, 111,  5,  8, RACE_DWARF},
		{ "キチガイのルンシー",         10000,  190, 110,  6,  8, RACE_HUMAN},
		{ "グランブルワース",           15000,  200, 116,  6,  9, RACE_GNOME},
		{ "フリッター",       15000,  220, 111,  4,  9, RACE_SPRITE},
		{ "ザリルス",         10000,  190, 111,  5,  8, RACE_HUMAN},
		{ "古きエグバート",         10000,  190, 110,  6,  8, RACE_DWARF},
		{ "誇り高きヴァリンドラ",           15000,  200, 116,  6,  9, RACE_HIGH_ELF},
		{ "錬金術師タエン",       15000,  220, 111,  4,  9, RACE_HUMAN},
		{ "巧言カイド",         10000,  190, 111,  5,  8, RACE_VAMPIRE},
		{ "暗きフリア",         10000,  190, 110,  6,  8, RACE_NIBELUNG},
		{ "質素なドムリ",           15000,  200, 116,  6,  9, RACE_DWARF},
		{ "魔の子ヤァジュッカ",       15000,  220, 111,  4,  9, RACE_IMP},
		{ "薬草師ジェララルドール",         10000,  190, 111,  5,  8, RACE_HIGH_ELF},
		{ "賢者オレラルダン",         10000,  190, 110,  6,  8, RACE_BARBARIAN},
		{ "デモニシストのフゾグロ",           15000,  200, 116,  6,  9, RACE_IMP},
		{ "錬金術師ドゥリアシュ",       15000,  220, 111,  4,  9, RACE_HALF_ORC},
		{ "強者ネリア",         10000,  190, 111,  5,  8, RACE_CYCLOPS},
		{ "辛口リグナス",         10000,  190, 110,  6,  8, RACE_HALF_ORC},
		{ "ティルバ",           15000,  200, 116,  6,  9, RACE_HOBBIT},
		{ "金持ちミリルドリック",       15000,  220, 111,  4,  9, RACE_HUMAN},

		{ "科学者マウザー",         10000,  190, 111,  5,  8, RACE_HALF_ELF},
		{ "カオスのウィズル",         10000,  190, 110,  6,  8, RACE_HOBBIT},
		{ "強欲ミダス",           15000,  200, 116,  6,  9, RACE_GNOME},
		{ "錬金術師ジャ=ファー",       15000,  220, 111,  4,  9, RACE_ELF},
		{ "カカルラカカル",           15000,  200, 116,  6,  9, RACE_KLACKON},
		{ "錬金術師ジャル=エス",       15000,  220, 111,  4,  9, RACE_ELF},
#else
		{ "Mauser the Chemist",         10000,  190, 111,  5,  8, RACE_HALF_ELF},
		{ "Wizzle the Chaotic",         10000,  190, 110,  6,  8, RACE_HOBBIT},
		{ "Midas the Greedy",           15000,  200, 116,  6,  9, RACE_GNOME},
		{ "Ja-Far the Alchemist",       15000,  220, 111,  4,  9, RACE_ELF},
		{ "Kakalrakakal",           15000,  200, 116,  6,  9, RACE_KLACKON},
		{ "Jal-Eth the Alchemist",       15000,  220, 111,  4,  9, RACE_ELF},
		{ "Fanelath the Cautious",         10000,  190, 111,  5,  8, RACE_DWARF},
		{ "Runcie the Insane",         10000,  190, 110,  6,  8, RACE_HUMAN},
		{ "Grumbleworth",           15000,  200, 116,  6,  9, RACE_GNOME},
		{ "Flitter",       15000,  220, 111,  4,  9, RACE_SPRITE},
		{ "Xarillus",         10000,  190, 111,  5,  8, RACE_HUMAN},
		{ "Egbert the Old",         10000,  190, 110,  6,  8, RACE_DWARF},
		{ "Valindra the Proud",           15000,  200, 116,  6,  9, RACE_HIGH_ELF},
		{ "Taen the Alchemist",       15000,  220, 111,  4,  9, RACE_HUMAN},
		{ "Cayd the Sweet",         10000,  190, 111,  5,  8, RACE_VAMPIRE},
		{ "Fulir the Dark",         10000,  190, 110,  6,  8, RACE_NIBELUNG},
		{ "Domli the Humble",           15000,  200, 116,  6,  9, RACE_DWARF},
		{ "Yaarjukka Demonspawn",       15000,  220, 111,  4,  9, RACE_IMP},
		{ "Gelaraldor the Herbmaster",         10000,  190, 111,  5,  8, RACE_HIGH_ELF},
		{ "Olelaldan the Wise",         10000,  190, 110,  6,  8, RACE_BARBARIAN},
		{ "Fthoglo the Demonicist",           15000,  200, 116,  6,  9, RACE_IMP},
		{ "Dridash the Alchemist",       15000,  220, 111,  4,  9, RACE_HALF_ORC},
		{ "Nelir the Strong",         10000,  190, 111,  5,  8, RACE_CYCLOPS},
		{ "Lignus the Pungent",         10000,  190, 110,  6,  8, RACE_HALF_ORC},
		{ "Tilba",           15000,  200, 116,  6,  9, RACE_HOBBIT},
		{ "Myrildric the Wealthy",       15000,  220, 111,  4,  9, RACE_HUMAN},

		{ "Mauser the Chemist",         10000,  190, 111,  5,  8, RACE_HALF_ELF},
		{ "Wizzle the Chaotic",         10000,  190, 110,  6,  8, RACE_HOBBIT},
		{ "Midas the Greedy",           15000,  200, 116,  6,  9, RACE_GNOME},
		{ "Ja-Far the Alchemist",       15000,  220, 111,  4,  9, RACE_ELF},
		{ "Kakalrakakal",           15000,  200, 116,  6,  9, RACE_KLACKON},
		{ "Jal-Eth the Alchemist",       15000,  220, 111,  4,  9, RACE_ELF},
#endif
	},

	{
		/* Magic Shop - 23 unique names */
#ifdef JP
		{ "ソーサラーのロ=パン",       20000,  200,  110,   7,   8,  RACE_HALF_ELF},
		{ "偉大なるブガービイ",         20000,  215,  113,   6,  10,  RACE_GNOME},
		{ "イェンダーの魔法使い",     30000,  200,  110,   7,  10,  RACE_HUMAN},
		{ "死霊使いリャク",30000,      175,  110,   5,  11,  RACE_HIGH_ELF},
		{ "魔術師スキドゥニー",        15000,  200, 110,  7,  8, RACE_HALF_ELF},
		{ "幻術師キリア",       30000,  200, 110,  7, 10, RACE_HUMAN},
		{ "死霊術師ニッキ",       30000,  175, 110,  5, 11, RACE_DARK_ELF},
		{ "ソロストラン",        15000,  200, 110,  7,  8, RACE_SPRITE},
		{ "烏賊口アチシェ",         20000,  215, 113,  6, 10, RACE_MIND_FLAYER},
		{ "貴族のカザ",       30000,  200, 110,  7, 10, RACE_HIGH_ELF},
		{ "暗きファジル",       30000,  175, 110,  5, 11, RACE_DARK_ELF},
		{ "偉大なるケルドーン",        15000,  200, 110,  7,  8, RACE_DWARF},
		{ "フィランスロプス",         20000,  215, 113,  6, 10, RACE_HOBBIT},
		{ "魔女のアグナー",       30000,  200, 110,  7, 10, RACE_HUMAN},
		{ "死霊術師ビュリアンス",       30000,  175, 110,  5, 11, RACE_BEASTMAN},
		{ "ハイメイジのヴイラク",        15000,  200, 110,  7,  8, RACE_BEASTMAN},
		{ "知恵者マディッシュ",         20000,  215, 113,  6, 10, RACE_BEASTMAN},
		{ "ファレブリンボール",       30000,  200, 110,  7, 10, RACE_HIGH_ELF},
		{ "陰険フェリル=ガンド",       30000,  175, 110,  5, 11, RACE_DARK_ELF},
		{ "呪術師サレゴード",        15000,  200, 110,  7,  8, RACE_BARBARIAN},
		{ "神秘家クトゥアロス",         20000,  215, 113,  6, 10, RACE_MIND_FLAYER},
		{ "幻術師イベリ",       30000,  200, 110,  7, 10, RACE_SKELETON},
		{ "死霊術師ヘトー",       30000,  175, 110,  5, 11, RACE_YEEK},
		{ "魔術師ロ=パン",        20000,  200, 110,  7,  8, RACE_HALF_ELF},
		{ "偉大なるブガービイ",         20000,  215, 113,  6, 10, RACE_GNOME},
		{ "イェンダーの魔法使い",       30000,  200, 110,  7, 10, RACE_HUMAN},
		{ "死霊術師リャク",       30000,  175, 110,  5, 11, RACE_DARK_ELF},
		{ "魔術師スキドゥニー",        15000,  200, 110,  7,  8, RACE_HALF_ELF},
		{ "幻術師キリア",       30000,  200, 110,  7, 10, RACE_HUMAN},
		{ "死霊術師ニッキ",       30000,  175, 110,  5, 11, RACE_DARK_ELF},
		{ "ソロストラン",        15000,  200, 110,  7,  8, RACE_SPRITE},
		{ "烏賊口アチシェ",         20000,  215, 113,  6, 10, RACE_MIND_FLAYER},
#else
		{ "Lo Pan the Sorcerer",        20000,  200, 110,  7,  8, RACE_HALF_ELF},
		{ "Buggerby the Great",         20000,  215, 113,  6, 10, RACE_GNOME},
		{ "The Wizard of Yendor",       30000,  200, 110,  7, 10, RACE_HUMAN},
		{ "Rjak the Necromancer",       30000,  175, 110,  5, 11, RACE_DARK_ELF},
		{ "Skidney the Sorcerer",        15000,  200, 110,  7,  8, RACE_HALF_ELF},
		{ "Kyria the Illusionist",       30000,  200, 110,  7, 10, RACE_HUMAN},
		{ "Nikki the Necromancer",       30000,  175, 110,  5, 11, RACE_DARK_ELF},
		{ "Solostoran",        15000,  200, 110,  7,  8, RACE_SPRITE},
		{ "Achshe the Tentacled",         20000,  215, 113,  6, 10, RACE_MIND_FLAYER},
		{ "Kaza the Noble",       30000,  200, 110,  7, 10, RACE_HIGH_ELF},
		{ "Fazzil the Dark",       30000,  175, 110,  5, 11, RACE_DARK_ELF},
		{ "Keldorn the Grand",        15000,  200, 110,  7,  8, RACE_DWARF},
		{ "Philanthropus",         20000,  215, 113,  6, 10, RACE_HOBBIT},
		{ "Agnar the Enchantress",       30000,  200, 110,  7, 10, RACE_HUMAN},
		{ "Buliance the Necromancer",       30000,  175, 110,  5, 11, RACE_BEASTMAN},
		{ "Vuirak the High-Mage",        15000,  200, 110,  7,  8, RACE_BEASTMAN},
		{ "Madish the Smart",         20000,  215, 113,  6, 10, RACE_BEASTMAN},
		{ "Falebrimbor",       30000,  200, 110,  7, 10, RACE_HIGH_ELF},
		{ "Felil-Gand the Subtle",       30000,  175, 110,  5, 11, RACE_DARK_ELF},
		{ "Thalegord the Shaman",        15000,  200, 110,  7,  8, RACE_BARBARIAN},
		{ "Cthoaloth the Mystic",         20000,  215, 113,  6, 10, RACE_MIND_FLAYER},
		{ "Ibeli the Illusionist",       30000,  200, 110,  7, 10, RACE_SKELETON},
		{ "Heto the Necromancer",       30000,  175, 110,  5, 11, RACE_YEEK},
		{ "Lo Pan the Sorcerer",        20000,  200, 110,  7,  8, RACE_HALF_ELF},
		{ "Buggerby the Great",         20000,  215, 113,  6, 10, RACE_GNOME},
		{ "The Wizard of Yendor",       30000,  200, 110,  7, 10, RACE_HUMAN},
		{ "Rjak the Necromancer",       30000,  175, 110,  5, 11, RACE_DARK_ELF},
		{ "Skidney the Sorcerer",        15000,  200, 110,  7,  8, RACE_HALF_ELF},
		{ "Kyria the Illusionist",       30000,  200, 110,  7, 10, RACE_HUMAN},
		{ "Nikki the Necromancer",       30000,  175, 110,  5, 11, RACE_DARK_ELF},
		{ "Solostoran",        15000,  200, 110,  7,  8, RACE_SPRITE},
		{ "Achshe the Tentacled",         20000,  215, 113,  6, 10, RACE_MIND_FLAYER},
#endif
	},
	{
		/* Black Market - 32 unique names */
#ifdef JP
		{ "ガリー=ギガズ",            20000,  250,  150,  10,   5,  RACE_HALF_TROLL},
		{ "ゴブリンのヒストーア",       20000,  250,  150,  10,   5,  RACE_HALF_ORC},
		{ "フェレンギ人クアーク",           30000,  250,  150,  10,   5,  RACE_HUMAN},
		{ "公正なる(?)トッピ",                     30000,  250,  150,  10,   5,  RACE_ELF},
		{ "死人ヴァッサ",             20000,  250, 150, 10,  5, RACE_ZOMBIE},
		{ "裏切り者カイン",          20000,  250, 150, 10,  5, RACE_VAMPIRE},
		{ "ブボニカス",          30000,  250, 150, 10,  5, RACE_BEASTMAN},
		{ "コープスライト",           30000,  250, 150, 10,  5, RACE_SPECTRE},
		{ "血に飢えしパリッシュ",                 20000,  250, 150, 10,  5, RACE_VAMPIRE},
		{ "ヴァイル",          20000,  250, 150, 10,  5, RACE_SKELETON},
		{ "信頼のプレンティス",          30000,  250, 150, 10,  5, RACE_SKELETON},
		{ "人間殺しのグリエラ",           30000,  250, 150, 10,  5, RACE_IMP},
		{ "エンジェル",                 20000,  250, 150, 10,  5, RACE_VAMPIRE},
		{ "水膨れフロツァム",          20000,  250, 150, 10,  5, RACE_ZOMBIE},
		{ "ニーヴァル",          30000,  250, 150, 10,  5, RACE_VAMPIRE},
		{ "明るいアナスタシア",           30000,  250, 150, 10,  5, RACE_SPECTRE},
		{ "死霊術師チャリティー", 20000,  250, 150, 10,  5, RACE_DARK_ELF},
		{ "ボクサーのプグナシオス",          20000,  250, 150, 10,  5, RACE_HALF_ORC},
		{ "幸運なフットソア",          30000,  250, 150, 10,  5, RACE_BEASTMAN},
		{ "光指のシドリア",           30000,  250, 150, 10,  5, RACE_HUMAN},
		{ "手品師リアソー",                 20000,  250, 150, 10,  5, RACE_HOBBIT},
		{ "やりくり上手のジャナッカ",          20000,  250, 150, 10,  5, RACE_GNOME},
		{ "悪党シーナ",          30000,  250, 150, 10,  5, RACE_GNOME},
		{ "大爪アルニッキ",           30000,  250, 150, 10,  5, RACE_DRACONIAN},
		{ "貧乏チャエアンド",                 20000,  250, 150, 10,  5, RACE_HUMAN},
		{ "山賊アファードーフ",          20000,  250, 150, 10,  5, RACE_BARBARIAN},
		{ "強欲ラザクスル",          30000,  250, 150, 10,  5, RACE_MIND_FLAYER},
		{ "ファラレウィン",           30000,  250, 150, 10,  5, RACE_SPRITE},
		{ "しわしわヴォスール",                 20000,  250, 150, 10,  5, RACE_NIBELUNG},
		{ "ハンサムなアラオード",          20000,  250, 150, 10,  5, RACE_AMBERITE},
		{ "負け犬セラドフリド",          30000,  250, 150, 10,  5, RACE_HUMAN},
		{ "片足のエルーロ",           30000,  250, 150, 10,  5, RACE_HALF_OGRE},
#else
		{ "Gary Gygaz",                 20000,  250, 150, 10,  5, RACE_HALF_TROLL},
		{ "Histor the Goblin",          20000,  250, 150, 10,  5, RACE_HALF_ORC},
		{ "Quark the Ferengi",          30000,  250, 150, 10,  5, RACE_DWARF},
		{ "Topi the Fair(?)",           30000,  250, 150, 10,  5, RACE_HUMAN},
		{ "Vhassa the Dead",             20000,  250, 150, 10,  5, RACE_ZOMBIE},
		{ "Kyn the Treacherous",          20000,  250, 150, 10,  5, RACE_VAMPIRE},
		{ "Bubonicus",          30000,  250, 150, 10,  5, RACE_BEASTMAN},
		{ "Corpselight",           30000,  250, 150, 10,  5, RACE_SPECTRE},
		{ "Parrish the Bloodthirsty",                 20000,  250, 150, 10,  5, RACE_VAMPIRE},
		{ "Vile",          20000,  250, 150, 10,  5, RACE_SKELETON},
		{ "Prentice the Trusted",          30000,  250, 150, 10,  5, RACE_SKELETON},
		{ "Griella Humanslayer",           30000,  250, 150, 10,  5, RACE_IMP},
		{ "Angel",                 20000,  250, 150, 10,  5, RACE_VAMPIRE},
		{ "Flotsam the Bloated",          20000,  250, 150, 10,  5, RACE_ZOMBIE},
		{ "Nieval",          30000,  250, 150, 10,  5, RACE_VAMPIRE},
		{ "Anastasia the Luminous",           30000,  250, 150, 10,  5, RACE_SPECTRE},
		{ "Charity the Necromancer", 20000,  250, 150, 10,  5, RACE_DARK_ELF},
		{ "Pugnacious the Pugilist",          20000,  250, 150, 10,  5, RACE_HALF_ORC},
		{ "Footsore the Lucky",          30000,  250, 150, 10,  5, RACE_BEASTMAN},
		{ "Sidria Lighfingered",           30000,  250, 150, 10,  5, RACE_HUMAN},
		{ "Riatho the Juggler",                 20000,  250, 150, 10,  5, RACE_HOBBIT},
		{ "Janaaka the Shifty",          20000,  250, 150, 10,  5, RACE_GNOME},
		{ "Cina the Rogue",          30000,  250, 150, 10,  5, RACE_GNOME},
		{ "Arunikki Greatclaw",           30000,  250, 150, 10,  5, RACE_DRACONIAN},
		{ "Chaeand the Poor",                 20000,  250, 150, 10,  5, RACE_HUMAN},
		{ "Afardorf the Brigand",          20000,  250, 150, 10,  5, RACE_BARBARIAN},
		{ "Lathaxl the Greedy",          30000,  250, 150, 10,  5, RACE_MIND_FLAYER},
		{ "Falarewyn",           30000,  250, 150, 10,  5, RACE_SPRITE},
		{ "Vosur the Wrinkled",                 20000,  250, 150, 10,  5, RACE_NIBELUNG},
		{ "Araord the Handsome",          20000,  250, 150, 10,  5, RACE_AMBERITE},
		{ "Theradfrid the Loser",          30000,  250, 150, 10,  5, RACE_HUMAN},
		{ "One-Legged Eroolo",           30000,  250, 150, 10,  5, RACE_HALF_OGRE},
#endif
	},
	{
		/* Home */
#ifdef JP
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
		{ "我が家",                          0,      100, 100,  0, 99, 99},
#else
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
		{ "Your home",                          0,      100, 100,  0, 99, 99},
#endif

	},

	{
		/* Bookstore - 21 unique names */
#ifdef JP
		{ "強欲ドラフ", 10000, 175, 108, 4, 12, RACE_HUMAN},
		{ "賢者オドナー", 15000, 120, 105, 6, 16, RACE_HIGH_ELF},
		{ "中立のガンダー", 25000, 120, 110, 7, 19, RACE_DARK_ELF},
		{ "忍耐の人ロ=シャ", 30000, 140, 105, 6, 12, RACE_ELF},
		{ "ランドルフ・カーター", 15000, 175, 108, 4, 12, RACE_HUMAN},
		{ "隼のサライ", 15000, 175, 108, 4, 12, RACE_HUMAN},
		{ "千里眼ボドリル", 20000, 120, 105, 6, 16, RACE_HIGH_ELF},
		{ "沈黙のヴェオロイン", 25000, 120, 110, 7, 19, RACE_ZOMBIE},
		{ "学者のヴァンシラス", 30000, 140, 105, 6, 12, RACE_MIND_FLAYER},
		{ "物書きオセイン", 15000, 175, 108, 4, 12, RACE_SKELETON},
		{ "本の虫オルヴァー", 20000, 120, 105, 6, 16, RACE_VAMPIRE},
		{ "浅井墓男", 25000, 120, 110, 7, 19, RACE_ZOMBIE},
		{ "デスマスク", 30000, 140, 105, 6, 12, RACE_ZOMBIE},
		{ "学者のアスーヌ", 15000, 175, 108, 4, 12, RACE_MIND_FLAYER},
		{ "死人のプリランド", 20000, 120, 105, 6, 16, RACE_ZOMBIE},
		{ "鉄のロナール", 25000, 120, 110, 7, 19, RACE_GOLEM},
#else
		{ "Dolaf the Greedy", 10000, 175, 108, 4, 12, RACE_HUMAN},
		{ "Odnar the Sage", 15000, 120, 105, 6, 16, RACE_HIGH_ELF},
		{ "Gandar the Neutral", 25000, 120, 110, 7, 19, RACE_DARK_ELF},
		{ "Ro-sha the Patient", 30000, 140, 105, 6, 12, RACE_ELF},
		{ "Randolph Carter", 15000, 175, 108, 4, 12, RACE_HUMAN},
		{ "Sarai the Swift", 15000, 175, 108, 4, 12, RACE_HUMAN},
		{ "Bodril the Seer", 20000, 120, 105, 6, 16, RACE_HIGH_ELF},
		{ "Veloin the Quiet", 25000, 120, 110, 7, 19, RACE_ZOMBIE},
		{ "Vanthylas the Learned", 30000, 140, 105, 6, 12, RACE_MIND_FLAYER},
		{ "Ossein the Literate", 15000, 175, 108, 4, 12, RACE_SKELETON},
		{ "Olvar Bookworm", 20000, 120, 105, 6, 16, RACE_VAMPIRE},
		{ "Shallowgrave", 25000, 120, 110, 7, 19, RACE_ZOMBIE},
		{ "Death Mask", 30000, 140, 105, 6, 12, RACE_ZOMBIE},
		{ "Asuunu the Learned", 15000, 175, 108, 4, 12, RACE_MIND_FLAYER},
		{ "Prirand the Dead", 20000, 120, 105, 6, 16, RACE_ZOMBIE},
		{ "Ronar the Iron", 25000, 120, 110, 7, 19, RACE_GOLEM},
#endif
#ifdef JP
		{ "ガリル=ガミル", 30000, 140, 105, 6, 12, RACE_ELF},
		{ "本食いローバグ", 15000, 175, 108, 4, 12, RACE_KOBOLD},
		{ "キリアリキーク", 20000, 120, 105, 6, 16, RACE_KLACKON},
		{ "静かなるリリン", 25000, 120, 110, 7, 19, RACE_DWARF},
		{ "王者イサング", 30000, 140, 105, 6, 12, RACE_HIGH_ELF},
		{ "強欲ドラフ", 10000, 175, 108, 4, 12, RACE_HUMAN},
		{ "賢者オドナー", 15000, 120, 105, 6, 16, RACE_HIGH_ELF},
		{ "中立のガンダー", 25000, 120, 110, 7, 19, RACE_DARK_ELF},
		{ "忍耐の人ロ=シャ", 30000, 140, 105, 6, 12, RACE_ELF},
		{ "ランドルフ・カーター", 15000, 175, 108, 4, 12, RACE_HUMAN},
		{ "隼サライ", 15000, 175, 108, 4, 12, RACE_HUMAN},
		{ "千里眼ボドリル", 20000, 120, 105, 6, 16, RACE_HIGH_ELF},
		{ "沈黙のヴェオロイン", 25000, 120, 110, 7, 19, RACE_ZOMBIE},
		{ "学者のヴァンシラス", 30000, 140, 105, 6, 12, RACE_MIND_FLAYER},
		{ "物書きオセイン", 15000, 175, 108, 4, 12, RACE_SKELETON},
		{ "本の虫オルヴァー", 20000, 120, 105, 6, 16, RACE_VAMPIRE},
#else
		{ "Galil-Gamir", 30000, 140, 105, 6, 12, RACE_ELF},
		{ "Rorbag Book-Eater", 15000, 175, 108, 4, 12, RACE_KOBOLD},
		{ "Kiriarikirk", 20000, 120, 105, 6, 16, RACE_KLACKON},
		{ "Rilin the Quiet", 25000, 120, 110, 7, 19, RACE_DWARF},
		{ "Isung the Lord", 30000, 140, 105, 6, 12, RACE_HIGH_ELF},
		{ "Dolaf the Greedy", 10000, 175, 108, 4, 12, RACE_HUMAN},
		{ "Odnar the Sage", 15000, 120, 105, 6, 16, RACE_HIGH_ELF},
		{ "Gandar the Neutral", 25000, 120, 110, 7, 19, RACE_DARK_ELF},
		{ "Ro-sha the Patient", 30000, 140, 105, 6, 12, RACE_ELF},
		{ "Randolph Carter", 15000, 175, 108, 4, 12, RACE_HUMAN},
		{ "Sarai the Swift", 15000, 175, 108, 4, 12, RACE_HUMAN},
		{ "Bodril the Seer", 20000, 120, 105, 6, 16, RACE_HIGH_ELF},
		{ "Veloin the Quiet", 25000, 120, 110, 7, 19, RACE_ZOMBIE},
		{ "Vanthylas the Learned", 30000, 140, 105, 6, 12, RACE_MIND_FLAYER},
		{ "Ossein the Literate", 15000, 175, 108, 4, 12, RACE_SKELETON},
		{ "Olvar Bookworm", 20000, 120, 105, 6, 16, RACE_VAMPIRE},
#endif
	},

	{
		/* Museum */
#ifdef JP
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
		{ "博物館",                          0,      100, 100,  0, 99, 99},
#else
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
		{ "Museum",                          0,      100, 100,  0, 99, 99},
#endif

	},

};

static int cur_store_num = 0;
static int store_top = 0;
static int store_bottom = 0;
static int xtra_stock = 0;
static store_type *st_ptr = NULL;
static const owner_type *ot_ptr = NULL;
static s16b old_town_num = 0;
static s16b inner_town_num = 0;
#define RUMOR_CHANCE 8

#define MAX_COMMENT_1	6

static concptr comment_1[MAX_COMMENT_1] =
{
#ifdef JP
	"オーケーだ。",
	"結構だ。",
	"そうしよう！",
	"賛成だ！",
	"よし！",
	"わかった！"
#else
	"Okay.",
	"Fine.",
	"Accepted!",
	"Agreed!",
	"Done!",
	"Taken!"
#endif

};

#ifdef JP
/*! ブラックマーケット追加メッセージ（承諾） */
static concptr comment_1_B[MAX_COMMENT_1] = {
	"まあ、それでいいや。",
	"今日はそれで勘弁してやる。",
	"分かったよ。",
	"しょうがない。",
	"それで我慢するよ。",
	"こんなもんだろう。"
};
#endif
#define MAX_COMMENT_2A	2

static concptr comment_2a[MAX_COMMENT_2A] =
{
#ifdef JP
	"私の忍耐力を試しているのかい？ $%s が最後だ。",
	"我慢にも限度があるぞ。 $%s が最後だ。"
#else
	"You try my patience.  %s is final.",
	"My patience grows thin.  %s is final."
#endif

};

#define MAX_COMMENT_2B	12

static concptr comment_2b[MAX_COMMENT_2B] =
{
#ifdef JP
	" $%s ぐらいは出さなきゃダメだよ。",
	" $%s なら受け取ってもいいが。",
	"ハ！ $%s 以下はないね。",
	"何て奴だ！ $%s 以下はあり得ないぞ。",
	"それじゃ少なすぎる！ $%s は欲しいところだ。",
	"バカにしている！ $%s はもらわないと。",
	"嘘だろう！ $%s でどうだい？",
	"おいおい！ $%s を考えてくれないか？",
	"1000匹のオークのノミに苦しめられるがいい！ $%s だ。",
	"お前の大切なものに災いあれ！ $%s でどうだ。",
	"モルゴスに賞味されるがいい！本当は $%s なんだろう？",
	"お前の母親はオーガか！ $%s は出すつもりなんだろ？"
#else
	"I can take no less than %s gold pieces.",
	"I will accept no less than %s gold pieces.",
	"Ha!  No less than %s gold pieces.",
	"You knave!  No less than %s gold pieces.",
	"That's a pittance!  I want %s gold pieces.",
	"That's an insult!  I want %s gold pieces.",
	"As if!  How about %s gold pieces?",
	"My arse!  How about %s gold pieces?",
	"May the fleas of 1000 orcs molest you!  Try %s gold pieces.",
	"May your most favourite parts go moldy!  Try %s gold pieces.",
	"May Morgoth find you tasty!  Perhaps %s gold pieces?",
	"Your mother was an Ogre!  Perhaps %s gold pieces?"
#endif

};

#ifdef JP
/*! ブラックマーケット用追加メッセージ（売るとき） */
static concptr comment_2b_B[MAX_COMMENT_2B] = {
	"いくら俺様がお人好しとはいえ $%s が限界だね。嫌なら帰りな。",
	"金がないのかい、あんた？まずは家に帰って $%s 揃えてきな。",
	"物の価値が分からん奴だな。これは $%s が普通なんだよ。",
	"俺の付けた値段に文句があるのか？ $%s が限界だ。",
	"ひょっとして新手の冗談かい？ $%s 持ってないなら帰りな。",
	"うちは他の店とは違うんだよ。$%s ぐらいは出しな。",
	"買う気がないなら帰りな。 $%s だと言っているんだ。",
	"話にならないね。 $%s くらい持っているんだろ？",
	"は？なんだそりゃ？ $%s の間違いか、ひょっとして？",
	"出口はあっちだよ。それとも $%s 出せるのかい、あんたに。",
	"命知らずな奴だな。 $%s 出せば今日の所は勘弁してやるよ。",
	"うちの店は貧乏人お断りだ。 $%s ぐらい出せないのかい？"
};
#endif
#define MAX_COMMENT_3A	2

static concptr comment_3a[MAX_COMMENT_3A] =
{
#ifdef JP
	"私の忍耐力を試しているのかい？ $%s が最後だ。",
	"我慢にも限度があるぞ。 $%s が最後だ。"
#else
	"You try my patience.  %s is final.",
	"My patience grows thin.  %s is final."
#endif

};


#define MAX_COMMENT_3B	12

static concptr comment_3b[MAX_COMMENT_3B] =
{
#ifdef JP
	"本音を言うと $%s でいいんだろ？",
	" $%s でどうだい？",
	" $%s ぐらいなら出してもいいが。",
	" $%s 以上払うなんて考えられないね。",
	"まあ落ちついて。 $%s でどうだい？",
	"そのガラクタなら $%s で引き取るよ。",
	"それじゃ高すぎる！ $%s がいいとこだろ。",
	"どうせいらないんだろ！ $%s でいいだろ？",
	"だめだめ！ $%s がずっとお似合いだよ。",
	"バカにしている！ $%s がせいぜいだ。",
	" $%s なら嬉しいところだがなあ。",
	" $%s 、それ以上はビタ一文出さないよ！"
#else
	"Perhaps %s gold pieces?",
	"How about %s gold pieces?",
	"I will pay no more than %s gold pieces.",
	"I can afford no more than %s gold pieces.",
	"Be reasonable.  How about %s gold pieces?",
	"I'll buy it as scrap for %s gold pieces.",
	"That is too much!  How about %s gold pieces?",
	"That looks war surplus!  Say %s gold pieces?",
	"Never!  %s is more like it.",
	"That's an insult!  %s is more like it.",
	"%s gold pieces and be thankful for it!",
	"%s gold pieces and not a copper more!"
#endif

};

#ifdef JP
/*! ブラックマーケット用追加メッセージ（買い取り） */
static concptr comment_3b_B[MAX_COMMENT_3B] = {
	" $%s ってところだね。そのどうしようもないガラクタは。",
	"この俺が $%s って言っているんだから、その通りにした方が身のためだぞ。",
	"俺の優しさに甘えるのもいい加減にしておけ。 $%s だ。",
	"その品なら $%s で売ってくれているがね、常識ある紳士はみんな。",
	"こりゃまた、がめつい奴だな。いくら俺が温厚とはいえ $%s が限界だ。",
	" $%s だ。別に俺はそんなガラクタ欲しくはないんだから。",
	"俺の鑑定額が気に入らないのか？ $%s 、嫌なら帰りな。",
	" $%s で引き取ってやるよ。喜んで受け取りな、貧乏人。",
	"物の価値が分からん奴は始末におえんな。それは $%s なんだよ。",
	"そんなに金が欲しいのか、あんた？ $%s で満足できんのか？",
	"入る店間違えてんじゃないのか？ $%s で嫌なら他をあたってくれ。",
	"俺の言い値にケチをつける奴がいるとは！ その度胸に免じて $%s だ。"
};
#endif
#define MAX_COMMENT_4A	4

static concptr comment_4a[MAX_COMMENT_4A] =
{
#ifdef JP
	"もうたくさんだ！何度も私をわずらわせないでくれ！",
	"うがー！一日の我慢の限度を超えている！",
	"もういい！時間の無駄以外のなにものでもない！",
	"もうやってられないよ！顔も見たくない！"
#else
	"Enough!  You have abused me once too often!",
	"Arghhh!  I have had enough abuse for one day!",
	"That does it!  You shall waste my time no more!",
	"This is getting nowhere!  I'm going to Londis!"
#endif

};

#ifdef JP
/*! ブラックマーケット用追加メッセージ（怒りの頂点） */
static concptr comment_4a_B[MAX_COMMENT_4A] = {
	"なめやがって！温厚な俺様でも限界があるってことを知れ！",
	"俺をここまで怒らせて...命があるだけでもありがたいと思え！",
	"ふざけてるのか！冷やかしなら相手を見てからにしろ！",
	"いいかげんにしろ！今度こんなまねしたらただじゃおかねえぞ！"
};
#endif
#define MAX_COMMENT_4B	4

static concptr comment_4b[MAX_COMMENT_4B] =
{
#ifdef JP
	"店から出て行け！",
	"俺の前から消え失せろ！",
	"どっかに行っちまえ！",
	"出ろ、出ろ、出て行け！"
#else
	"Leave my store!",
	"Get out of my sight!",
	"Begone, you scoundrel!",
	"Out, out, out!"
#endif

};

#ifdef JP
/*! ブラックマーケット用追加メッセージ（追い出し） */
static concptr comment_4b_B[MAX_COMMENT_4B] = {
	"二度とうちに来るんじゃねえ！！",
	"とっとと、どっかへ失せろ！！",
	"今すぐ消え失せろ！！",
	"出ていけ！出ていけ！！"
};
#endif
#define MAX_COMMENT_5	8

static concptr comment_5[MAX_COMMENT_5] =
{
#ifdef JP
	"考え直してくれ。",
	"そりゃおかしい！",
	"もっと真面目に言ってくれ！",
	"交渉する気があるのかい？",
	"冷やかしに来たのか！",
	"悪い冗談だ！",
	"我慢くらべかい。",
	"ふーむ、良い天気だ。"
#else
	"Try again.",
	"Ridiculous!",
	"You will have to do better than that!",
	"Do you wish to do business or not?",
	"You've got to be kidding!",
	"You'd better be kidding!",
	"You try my patience.",
	"Hmmm, nice weather we're having."
#endif

};

#ifdef JP
/*! ブラックマーケット用追加メッセージ（怒り） */
static concptr comment_5_B[MAX_COMMENT_5] = {
	"時間の無駄だな、これは。",
	"厄介なお客様だな！",
	"話して分かる相手じゃなさそうだ。",
	"痛い目にあいたいらしいな！",
	"なんて強欲な奴だ！",
	"話にならん輩だ！",
	"どうしようもない貧乏人だ！",
	"喧嘩を売っているのか？"
};
#endif
#define MAX_COMMENT_6	4

static concptr comment_6[MAX_COMMENT_6] =
{
#ifdef JP
	"どうやら聞き間違えたらしい。",
	"失礼、よく聞こえなかったよ。",
	"すまない、何だって？",
	"悪い、もう一度言ってくれる？"
#else
	"I must have heard you wrong.",
	"I'm sorry, I missed that.",
	"I'm sorry, what was that?",
	"Sorry, what was that again?"
#endif

};

/*** Initialize others ***/

/*!
 * 店舗で販売するオブジェクトを定義する / Hack -- Objects sold in the stores -- by tval/sval pair.
 */
byte store_table[MAX_STORES][STORE_CHOICES][2] =
{
	{
		/* General Store */

		{ TV_FOOD, SV_FOOD_RATION },
		{ TV_FOOD, SV_FOOD_RATION },
		{ TV_FOOD, SV_FOOD_RATION },
		{ TV_FOOD, SV_FOOD_RATION },

		{ TV_FOOD, SV_FOOD_RATION },
		{ TV_FOOD, SV_FOOD_BISCUIT },
		{ TV_FOOD, SV_FOOD_JERKY },
		{ TV_FOOD, SV_FOOD_JERKY },

		{ TV_FOOD, SV_FOOD_PINT_OF_WINE },
		{ TV_FOOD, SV_FOOD_PINT_OF_ALE },
		{ TV_LITE, SV_LITE_TORCH },
		{ TV_LITE, SV_LITE_TORCH },

		{ TV_LITE, SV_LITE_TORCH },
		{ TV_LITE, SV_LITE_TORCH },
		{ TV_LITE, SV_LITE_LANTERN },
		{ TV_LITE, SV_LITE_LANTERN },

		{ TV_FLASK, 0 },
		{ TV_FLASK, 0 },
		{ TV_FLASK, 0 },
		{ TV_FLASK, 0 },

		{ TV_FLASK, 0 },
		{ TV_FLASK, 0 },
		{ TV_SPIKE, 0 },
		{ TV_SPIKE, 0 },

		{ TV_SHOT, SV_AMMO_NORMAL },
		{ TV_ARROW, SV_AMMO_NORMAL },
		{ TV_BOLT, SV_AMMO_NORMAL },
		{ TV_DIGGING, SV_SHOVEL },

		{ TV_DIGGING, SV_PICK },
		{ TV_CLOAK, SV_CLOAK },
		{ TV_CLOAK, SV_CLOAK },
		{ TV_CLOAK, SV_FUR_CLOAK },

		{ TV_FOOD, SV_FOOD_RATION },
		{ TV_FOOD, SV_FOOD_RATION },
		{ TV_FOOD, SV_FOOD_RATION },
		{ TV_FOOD, SV_FOOD_RATION },

		{ TV_POTION, SV_POTION_WATER },
		{ TV_POTION, SV_POTION_WATER },
		{ TV_LITE, SV_LITE_LANTERN },
		{ TV_LITE, SV_LITE_LANTERN },

		{ TV_FOOD, SV_FOOD_WAYBREAD },
		{ TV_FOOD, SV_FOOD_WAYBREAD },
		{ TV_CAPTURE, 0 },
		{ TV_FIGURINE, 0 },

		{ TV_SHOT, SV_AMMO_NORMAL },
		{ TV_ARROW, SV_AMMO_NORMAL },
		{ TV_BOLT, SV_AMMO_NORMAL },
		{ TV_DIGGING, SV_SHOVEL }
	},

	{
		/* Armoury */

		{ TV_BOOTS, SV_PAIR_OF_SOFT_LEATHER_BOOTS },
		{ TV_BOOTS, SV_PAIR_OF_SOFT_LEATHER_BOOTS },
		{ TV_BOOTS, SV_PAIR_OF_HARD_LEATHER_BOOTS },
		{ TV_BOOTS, SV_PAIR_OF_HARD_LEATHER_BOOTS },

		{ TV_HELM, SV_HARD_LEATHER_CAP },
		{ TV_HELM, SV_HARD_LEATHER_CAP },
		{ TV_HELM, SV_METAL_CAP },
		{ TV_HELM, SV_IRON_HELM },

		{ TV_SOFT_ARMOR, SV_ROBE },
		{ TV_SOFT_ARMOR, SV_ROBE },
		{ TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
		{ TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },

		{ TV_SOFT_ARMOR, SV_HARD_LEATHER_ARMOR },
		{ TV_SOFT_ARMOR, SV_HARD_LEATHER_ARMOR },
		{ TV_SOFT_ARMOR, SV_HARD_STUDDED_LEATHER },
		{ TV_SOFT_ARMOR, SV_HARD_STUDDED_LEATHER },

		{ TV_SOFT_ARMOR, SV_RHINO_HIDE_ARMOR },
		{ TV_SOFT_ARMOR, SV_LEATHER_SCALE_MAIL },
		{ TV_HARD_ARMOR, SV_METAL_SCALE_MAIL },
		{ TV_HARD_ARMOR, SV_CHAIN_MAIL },

		{ TV_HARD_ARMOR, SV_DOUBLE_RING_MAIL },
		{ TV_HARD_ARMOR, SV_AUGMENTED_CHAIN_MAIL },
		{ TV_HARD_ARMOR, SV_BAR_CHAIN_MAIL },
		{ TV_HARD_ARMOR, SV_DOUBLE_CHAIN_MAIL },

		{ TV_HARD_ARMOR, SV_METAL_BRIGANDINE_ARMOUR },
		{ TV_HARD_ARMOR, SV_SPLINT_MAIL },
		{ TV_GLOVES, SV_SET_OF_LEATHER_GLOVES },
		{ TV_GLOVES, SV_SET_OF_LEATHER_GLOVES },

		{ TV_GLOVES, SV_SET_OF_GAUNTLETS },
		{ TV_SHIELD, SV_SMALL_LEATHER_SHIELD },
		{ TV_SHIELD, SV_LARGE_LEATHER_SHIELD },
		{ TV_SHIELD, SV_SMALL_METAL_SHIELD },

		{ TV_BOOTS, SV_PAIR_OF_HARD_LEATHER_BOOTS },
		{ TV_BOOTS, SV_PAIR_OF_HARD_LEATHER_BOOTS },
		{ TV_HELM, SV_HARD_LEATHER_CAP },
		{ TV_HELM, SV_HARD_LEATHER_CAP },

		{ TV_SOFT_ARMOR, SV_ROBE },
		{ TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
		{ TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
		{ TV_SOFT_ARMOR, SV_HARD_LEATHER_ARMOR },

		{ TV_SOFT_ARMOR, SV_LEATHER_JACK },
		{ TV_HARD_ARMOR, SV_METAL_SCALE_MAIL },
		{ TV_HARD_ARMOR, SV_CHAIN_MAIL },
		{ TV_HARD_ARMOR, SV_CHAIN_MAIL },

		{ TV_GLOVES, SV_SET_OF_LEATHER_GLOVES },
		{ TV_GLOVES, SV_SET_OF_GAUNTLETS },
		{ TV_SHIELD, SV_SMALL_LEATHER_SHIELD },
		{ TV_SHIELD, SV_SMALL_LEATHER_SHIELD }
	},

	{
		/* Weaponsmith */

		{ TV_SWORD, SV_DAGGER },
		{ TV_SWORD, SV_MAIN_GAUCHE },
		{ TV_SWORD, SV_RAPIER },
		{ TV_SWORD, SV_SMALL_SWORD },

		{ TV_SWORD, SV_SHORT_SWORD },
		{ TV_SWORD, SV_SABRE },
		{ TV_SWORD, SV_CUTLASS },
		{ TV_SWORD, SV_TULWAR },

		{ TV_SWORD, SV_BROAD_SWORD },
		{ TV_SWORD, SV_LONG_SWORD },
		{ TV_SWORD, SV_SCIMITAR },
		{ TV_SWORD, SV_KATANA },

		{ TV_SWORD, SV_BASTARD_SWORD },
		{ TV_POLEARM, SV_SPEAR },
		{ TV_POLEARM, SV_AWL_PIKE },
		{ TV_POLEARM, SV_TRIDENT },

		{ TV_POLEARM, SV_PIKE },
		{ TV_POLEARM, SV_BEAKED_AXE },
		{ TV_POLEARM, SV_BROAD_AXE },
		{ TV_POLEARM, SV_LANCE },

		{ TV_POLEARM, SV_BATTLE_AXE },
		{ TV_POLEARM, SV_HATCHET },
		{ TV_BOW, SV_SLING },
		{ TV_BOW, SV_SHORT_BOW },

		{ TV_BOW, SV_LIGHT_XBOW },
		{ TV_SHOT, SV_AMMO_NORMAL },
		{ TV_SHOT, SV_AMMO_NORMAL },
		{ TV_ARROW, SV_AMMO_NORMAL },

		{ TV_ARROW, SV_AMMO_NORMAL },
		{ TV_BOLT, SV_AMMO_NORMAL },
		{ TV_BOLT, SV_AMMO_NORMAL },
		{ TV_BOW, SV_LIGHT_XBOW },

		{ TV_ARROW, SV_AMMO_NORMAL },
		{ TV_BOLT, SV_AMMO_NORMAL },
		{ TV_BOW, SV_SHORT_BOW },
		{ TV_BOW, SV_LIGHT_XBOW },

		{ TV_SWORD, SV_DAGGER },
		{ TV_SWORD, SV_TANTO },
		{ TV_SWORD, SV_RAPIER },
		{ TV_SWORD, SV_SMALL_SWORD },

		{ TV_SWORD, SV_SHORT_SWORD },
		{ TV_SWORD, SV_LONG_SWORD },
		{ TV_SWORD, SV_SCIMITAR },
		{ TV_SWORD, SV_BROAD_SWORD },

		{ TV_HISSATSU_BOOK, 0 },
		{ TV_HISSATSU_BOOK, 0 },
		{ TV_HISSATSU_BOOK, 1 },
		{ TV_HISSATSU_BOOK, 1 },
	},

	{
		/* Temple */

		{ TV_HAFTED, SV_NUNCHAKU },
		{ TV_HAFTED, SV_QUARTERSTAFF },
		{ TV_HAFTED, SV_MACE },
		{ TV_HAFTED, SV_BO_STAFF },

		{ TV_HAFTED, SV_WAR_HAMMER },
		{ TV_HAFTED, SV_WAR_HAMMER },
		{ TV_HAFTED, SV_MORNING_STAR },
		{ TV_HAFTED, SV_FLAIL },

		{ TV_HAFTED, SV_LEAD_FILLED_MACE },
		{ TV_SCROLL, SV_SCROLL_REMOVE_CURSE },
		{ TV_SCROLL, SV_SCROLL_BLESSING },
		{ TV_SCROLL, SV_SCROLL_HOLY_CHANT },

		{ TV_POTION, SV_POTION_HEROISM },
		{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
		{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
		{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },

		{ TV_POTION, SV_POTION_CURE_LIGHT },
		{ TV_POTION, SV_POTION_CURE_SERIOUS },
		{ TV_POTION, SV_POTION_CURE_SERIOUS },
		{ TV_POTION, SV_POTION_CURE_CRITICAL },

		{ TV_POTION, SV_POTION_CURE_CRITICAL },
		{ TV_POTION, SV_POTION_RESTORE_EXP },
		{ TV_POTION, SV_POTION_RESTORE_EXP },
		{ TV_POTION, SV_POTION_RESTORE_EXP },

		{ TV_LIFE_BOOK, 0 },
		{ TV_LIFE_BOOK, 0 },
		{ TV_LIFE_BOOK, 1 },
		{ TV_LIFE_BOOK, 1 },

		{ TV_CRUSADE_BOOK, 0 },
		{ TV_CRUSADE_BOOK, 0 },
		{ TV_CRUSADE_BOOK, 1 },
		{ TV_CRUSADE_BOOK, 1 },

		{ TV_HAFTED, SV_WHIP },
		{ TV_HAFTED, SV_MACE },
		{ TV_HAFTED, SV_BALL_AND_CHAIN },
		{ TV_HAFTED, SV_WAR_HAMMER },

		{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
		{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
		{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
		{ TV_POTION, SV_POTION_CURE_CRITICAL },

		{ TV_POTION, SV_POTION_CURE_CRITICAL },
		{ TV_POTION, SV_POTION_RESTORE_EXP },

		{ TV_FIGURINE, 0 },
		{ TV_STATUE, SV_ANY },

		{ TV_SCROLL, SV_SCROLL_REMOVE_CURSE },
		{ TV_SCROLL, SV_SCROLL_REMOVE_CURSE },
		{ TV_SCROLL, SV_SCROLL_STAR_REMOVE_CURSE },
		{ TV_SCROLL, SV_SCROLL_STAR_REMOVE_CURSE }
	},

	{
		/* Alchemy shop */

		{ TV_SCROLL, SV_SCROLL_ENCHANT_WEAPON_TO_HIT },
		{ TV_SCROLL, SV_SCROLL_ENCHANT_WEAPON_TO_DAM },
		{ TV_SCROLL, SV_SCROLL_ENCHANT_ARMOR },
		{ TV_SCROLL, SV_SCROLL_IDENTIFY },

		{ TV_SCROLL, SV_SCROLL_IDENTIFY },
		{ TV_SCROLL, SV_SCROLL_IDENTIFY },
		{ TV_SCROLL, SV_SCROLL_IDENTIFY },
		{ TV_SCROLL, SV_SCROLL_LIGHT },

		{ TV_SCROLL, SV_SCROLL_PHASE_DOOR },
		{ TV_SCROLL, SV_SCROLL_PHASE_DOOR },
		{ TV_SCROLL, SV_SCROLL_TELEPORT },
		{ TV_SCROLL, SV_SCROLL_MONSTER_CONFUSION },

		{ TV_SCROLL, SV_SCROLL_MAPPING },
		{ TV_SCROLL, SV_SCROLL_DETECT_GOLD },
		{ TV_SCROLL, SV_SCROLL_DETECT_ITEM },
		{ TV_SCROLL, SV_SCROLL_DETECT_TRAP },

		{ TV_SCROLL, SV_SCROLL_DETECT_INVIS },
		{ TV_SCROLL, SV_SCROLL_RECHARGING },
		{ TV_SCROLL, SV_SCROLL_TELEPORT },
		{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },

		{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
		{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
		{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
		{ TV_SCROLL, SV_SCROLL_TELEPORT },

		{ TV_SCROLL, SV_SCROLL_TELEPORT },
		{ TV_POTION, SV_POTION_RES_STR },
		{ TV_POTION, SV_POTION_RES_INT },
		{ TV_POTION, SV_POTION_RES_WIS },

		{ TV_POTION, SV_POTION_RES_DEX },
		{ TV_POTION, SV_POTION_RES_CON },
		{ TV_POTION, SV_POTION_RES_CHR },
		{ TV_SCROLL, SV_SCROLL_IDENTIFY },

		{ TV_SCROLL, SV_SCROLL_IDENTIFY },
		{ TV_SCROLL, SV_SCROLL_STAR_IDENTIFY },  /* Yep, occasionally! */
		{ TV_SCROLL, SV_SCROLL_STAR_IDENTIFY },
		{ TV_SCROLL, SV_SCROLL_LIGHT },

		{ TV_POTION, SV_POTION_RES_STR },
		{ TV_POTION, SV_POTION_RES_INT },
		{ TV_POTION, SV_POTION_RES_WIS },
		{ TV_POTION, SV_POTION_RES_DEX },

		{ TV_POTION, SV_POTION_RES_CON },
		{ TV_POTION, SV_POTION_RES_CHR },
		{ TV_SCROLL, SV_SCROLL_ENCHANT_ARMOR },
		{ TV_SCROLL, SV_SCROLL_ENCHANT_ARMOR },

		{ TV_SCROLL, SV_SCROLL_RECHARGING },
		{ TV_SCROLL, SV_SCROLL_PHASE_DOOR },
		{ TV_SCROLL, SV_SCROLL_ENCHANT_WEAPON_TO_HIT },
		{ TV_SCROLL, SV_SCROLL_ENCHANT_WEAPON_TO_DAM },

	},

	{
		/* Magic-User store */

		{ TV_RING, SV_RING_PROTECTION },
		{ TV_RING, SV_RING_LEVITATION_FALL },
		{ TV_RING, SV_RING_PROTECTION },
		{ TV_RING, SV_RING_RESIST_FIRE },

		{ TV_RING, SV_RING_RESIST_COLD },
		{ TV_AMULET, SV_AMULET_CHARISMA },
		{ TV_RING, SV_RING_WARNING },
		{ TV_AMULET, SV_AMULET_RESIST_ACID },

		{ TV_AMULET, SV_AMULET_SEARCHING },
		{ TV_WAND, SV_WAND_SLOW_MONSTER },
		{ TV_WAND, SV_WAND_CONFUSE_MONSTER },
		{ TV_WAND, SV_WAND_SLEEP_MONSTER },

		{ TV_WAND, SV_WAND_MAGIC_MISSILE },
		{ TV_WAND, SV_WAND_STINKING_CLOUD },
		{ TV_WAND, SV_WAND_WONDER },
		{ TV_WAND, SV_WAND_DISARMING },

		{ TV_STAFF, SV_STAFF_LITE },
		{ TV_STAFF, SV_STAFF_MAPPING },
		{ TV_STAFF, SV_STAFF_DETECT_TRAP },
		{ TV_STAFF, SV_STAFF_DETECT_DOOR },

		{ TV_STAFF, SV_STAFF_DETECT_GOLD },
		{ TV_STAFF, SV_STAFF_DETECT_ITEM },
		{ TV_STAFF, SV_STAFF_DETECT_INVIS },
		{ TV_STAFF, SV_STAFF_DETECT_EVIL },

		{ TV_STAFF, SV_STAFF_TELEPORTATION },
		{ TV_STAFF, SV_STAFF_TELEPORTATION },
		{ TV_STAFF, SV_STAFF_TELEPORTATION },
		{ TV_STAFF, SV_STAFF_TELEPORTATION },

		{ TV_STAFF, SV_STAFF_IDENTIFY },
		{ TV_STAFF, SV_STAFF_IDENTIFY },
		{ TV_STAFF, SV_STAFF_IDENTIFY },

		{ TV_STAFF, SV_STAFF_IDENTIFY },
		{ TV_STAFF, SV_STAFF_REMOVE_CURSE },
		{ TV_STAFF, SV_STAFF_CURE_LIGHT },
		{ TV_STAFF, SV_STAFF_PROBING },

		{ TV_FIGURINE, 0 },

		{ TV_SORCERY_BOOK, 0 },
		{ TV_SORCERY_BOOK, 0 },
		{ TV_SORCERY_BOOK, 1 },
		{ TV_SORCERY_BOOK, 1 },

		{ TV_ARCANE_BOOK, 0 },
		{ TV_ARCANE_BOOK, 0 },
		{ TV_ARCANE_BOOK, 1 },
		{ TV_ARCANE_BOOK, 1 },

		{ TV_ARCANE_BOOK, 2 },
		{ TV_ARCANE_BOOK, 2 },
		{ TV_ARCANE_BOOK, 3 },
		{ TV_ARCANE_BOOK, 3 },

	},

	{
		/* Black Market (unused) */
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 }
	},

	{
		/* Home (unused) */
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 }
	},

	{
		/* Bookstore */
		{ TV_SORCERY_BOOK, 0 },
		{ TV_SORCERY_BOOK, 0 },
		{ TV_SORCERY_BOOK, 1 },
		{ TV_SORCERY_BOOK, 1 },

		{ TV_NATURE_BOOK, 0 },
		{ TV_NATURE_BOOK, 0 },
		{ TV_NATURE_BOOK, 1 },
		{ TV_NATURE_BOOK, 1 },

		{ TV_CHAOS_BOOK, 0 },
		{ TV_CHAOS_BOOK, 0 },
		{ TV_CHAOS_BOOK, 1 },
		{ TV_CHAOS_BOOK, 1 },

		{ TV_DEATH_BOOK, 0 },
		{ TV_DEATH_BOOK, 0 },
		{ TV_DEATH_BOOK, 1 },
		{ TV_DEATH_BOOK, 1 },

		{ TV_TRUMP_BOOK, 0 },		/* +16 */
		{ TV_TRUMP_BOOK, 0 },
		{ TV_TRUMP_BOOK, 1 },
		{ TV_TRUMP_BOOK, 1 },

		{ TV_ARCANE_BOOK, 0 },
		{ TV_ARCANE_BOOK, 1 },
		{ TV_ARCANE_BOOK, 2 },
		{ TV_ARCANE_BOOK, 3 },

		{ TV_CRAFT_BOOK, 0 },
		{ TV_CRAFT_BOOK, 0 },
		{ TV_CRAFT_BOOK, 1 },
		{ TV_CRAFT_BOOK, 1 },

		{ TV_DAEMON_BOOK, 0 },
		{ TV_DAEMON_BOOK, 0 },
		{ TV_DAEMON_BOOK, 1 },
		{ TV_DAEMON_BOOK, 1 },

		{ TV_MUSIC_BOOK, 0 },
		{ TV_MUSIC_BOOK, 0 },
		{ TV_MUSIC_BOOK, 1 },
		{ TV_MUSIC_BOOK, 1 },

		{ TV_HEX_BOOK, 0 },
		{ TV_HEX_BOOK, 0 },
		{ TV_HEX_BOOK, 1 },
		{ TV_HEX_BOOK, 1 },
	},

	{
		/* Museum (unused) */
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 }
	}
};



/*!
 * @brief 取引成功時の店主のメッセージ処理 /
 * Successful haggle.
 * @return なし
 */
static void say_comment_1(void)
{
#ifdef JP
	/* ブラックマーケットのときは別のメッセージを出す */
	if ( cur_store_num == STORE_BLACK ) {
		msg_print(comment_1_B[randint0(MAX_COMMENT_1)]);
	}
	else{
		msg_print(comment_1[randint0(MAX_COMMENT_1)]);
	}
#else
	msg_print(comment_1[randint0(MAX_COMMENT_1)]);
#endif


	if (one_in_(RUMOR_CHANCE))
	{
#ifdef JP
		msg_print("店主は耳うちした:");
#else
		msg_print("The shopkeeper whispers something into your ear:");
#endif
		display_rumor(TRUE);
	}
}


/*!
 * @brief プレイヤーがアイテムを買う時の価格代案メッセージ処理 /
 * Continue haggling (player is buying)
 * @param value 店主の提示価格
 * @param annoyed 店主のいらつき度
 * @return なし
 */
static void say_comment_2(PRICE value, int annoyed)
{
	char	tmp_val[80];

	/* Prepare a string to insert */
	sprintf(tmp_val, "%ld", (long)value);

	/* Final offer */
	if (annoyed > 0)
	{
		/* Formatted message */
		msg_format(comment_2a[randint0(MAX_COMMENT_2A)], tmp_val);
	}

	/* Normal offer */
	else
	{
		/* Formatted message */
#ifdef JP
		/* ブラックマーケットの時は別のメッセージを出す */
		if ( cur_store_num == STORE_BLACK ){
			msg_format(comment_2b_B[randint0(MAX_COMMENT_2B)], tmp_val);
		}
		else{
		msg_format(comment_2b[randint0(MAX_COMMENT_2B)], tmp_val);
	}
#else
		msg_format(comment_2b[randint0(MAX_COMMENT_2B)], tmp_val);
#endif

	}
}


/*!
 * @brief プレイヤーがアイテムを売る時の価格代案メッセージ処理 /
 * Continue haggling (player is selling)
 * @param value 店主の提示価格
 * @param annoyed 店主のいらつき度
 * @return なし
 */
static void say_comment_3(PRICE value, int annoyed)
{
	char	tmp_val[80];

	/* Prepare a string to insert */
	sprintf(tmp_val, "%ld", (long)value);

	/* Final offer */
	if (annoyed > 0)
	{
		/* Formatted message */
		msg_format(comment_3a[randint0(MAX_COMMENT_3A)], tmp_val);
	}

	/* Normal offer */
	else
	{
		/* Formatted message */
#ifdef JP
		/* ブラックマーケットの時は別のメッセージを出す */
		if ( cur_store_num == STORE_BLACK ){
			msg_format(comment_3b_B[randint0(MAX_COMMENT_3B)], tmp_val);
		}
		else{
		msg_format(comment_3b[randint0(MAX_COMMENT_3B)], tmp_val);
	}
#else
		msg_format(comment_3b[randint0(MAX_COMMENT_3B)], tmp_val);
#endif

	}
}


/*!
 * @brief 店主がプレイヤーを追い出す時のメッセージ処理 /
 * Kick 'da bum out.					-RAK-
 * @return なし
 */
static void say_comment_4(void)
{
#ifdef JP
	/* ブラックマーケットの時は別のメッセージを出す */
	if ( cur_store_num == STORE_BLACK ){
		msg_print(comment_4a_B[randint0(MAX_COMMENT_4A)]);
		msg_print(comment_4b_B[randint0(MAX_COMMENT_4B)]);
	}
	else{
		msg_print(comment_4a[randint0(MAX_COMMENT_4A)]);
		msg_print(comment_4b[randint0(MAX_COMMENT_4B)]);
	}
#else
	msg_print(comment_4a[randint0(MAX_COMMENT_4A)]);
	msg_print(comment_4b[randint0(MAX_COMMENT_4B)]);
#endif

}


/*!
 * @brief 店主がプレイヤーに取り合わない時のメッセージ処理 /
 * You are insulting me
 * @return なし
 */
static void say_comment_5(void)
{
#ifdef JP
	/* ブラックマーケットの時は別のメッセージを出す */
	if ( cur_store_num == STORE_BLACK ){
		msg_print(comment_5_B[randint0(MAX_COMMENT_5)]);
	}
	else{
		msg_print(comment_5[randint0(MAX_COMMENT_5)]);
	}
#else
	msg_print(comment_5[randint0(MAX_COMMENT_5)]);
#endif

}


/*!
 * @brief 店主がプレイヤーの提示を理解できなかった時のメッセージ処理 /
 * That makes no sense.
 * @return なし
 */
static void say_comment_6(void)
{
	msg_print(comment_6[randint0(MAX_COMMENT_6)]);
}


#define MAX_COMMENT_7A	4

static concptr comment_7a[MAX_COMMENT_7A] =
{
#ifdef JP
	"うわああぁぁ！",
	"なんてこった！",
	"誰かがむせび泣く声が聞こえる...。",
	"店主が悔しげにわめいている！"
#else
	"Arrgghh!",
	"You bastard!",
	"You hear someone sobbing...",
	"The shopkeeper howls in agony!"
#endif

};

#define MAX_COMMENT_7B	4

static concptr comment_7b[MAX_COMMENT_7B] =
{
#ifdef JP
	"くそう！",
	"この悪魔め！",
	"店主が恨めしそうに見ている。",
	"店主が睨んでいる。"
#else
	"Damn!",
	"You fiend!",
	"The shopkeeper curses at you.",
	"The shopkeeper glares at you."
#endif

};

#define MAX_COMMENT_7C	4

static concptr comment_7c[MAX_COMMENT_7C] =
{
#ifdef JP
	"すばらしい！",
	"君が天使に見えるよ！",
	"店主がクスクス笑っている。",
	"店主が大声で笑っている。"
#else
	"Cool!",
	"You've made my day!",
	"The shopkeeper giggles.",
	"The shopkeeper laughs loudly."
#endif

};

#define MAX_COMMENT_7D	4

static concptr comment_7d[MAX_COMMENT_7D] =
{
#ifdef JP
	"やっほぅ！",
	"こんなおいしい思いをしたら、真面目に働けなくなるなぁ。",
	"店主は嬉しくて跳ね回っている。",
	"店主は満面に笑みをたたえている。"
#else
	"Yipee!",
	"I think I'll retire!",
	"The shopkeeper jumps for joy.",
	"The shopkeeper smiles gleefully."
#endif

};


/*!
 * @brief 店主が交渉を終えた際の反応を返す処理 /
 * Let a shop-keeper React to a purchase
 * @param price アイテムの取引額
 * @param value アイテムの実際価値
 * @param guess 店主が当初予想していた価値
 * @return なし
 * @details 
 * We paid "price", it was worth "value", and we thought it was worth "guess"
 */
static void purchase_analyze(PRICE price, PRICE value, PRICE guess)
{
	/* Item was worthless, but we bought it */
	if ((value <= 0) && (price > value))
	{
		msg_print(comment_7a[randint0(MAX_COMMENT_7A)]);
		chg_virtue(p_ptr, V_HONOUR, -1);
		chg_virtue(p_ptr, V_JUSTICE, -1);
		sound(SOUND_STORE1);
	}

	/* Item was cheaper than we thought, and we paid more than necessary */
	else if ((value < guess) && (price > value))
	{
		msg_print(comment_7b[randint0(MAX_COMMENT_7B)]);
		chg_virtue(p_ptr, V_JUSTICE, -1);
		if (one_in_(4)) chg_virtue(p_ptr, V_HONOUR, -1);
		sound(SOUND_STORE2);
	}

	/* Item was a good bargain, and we got away with it */
	else if ((value > guess) && (value < (4 * guess)) && (price < value))
	{
		msg_print(comment_7c[randint0(MAX_COMMENT_7C)]);
		if (one_in_(4)) chg_virtue(p_ptr, V_HONOUR, -1);
		else if (one_in_(4)) chg_virtue(p_ptr, V_HONOUR, 1);
		sound(SOUND_STORE3);
	}

	/* Item was a great bargain, and we got away with it */
	else if ((value > guess) && (price < value))
	{
		msg_print(comment_7d[randint0(MAX_COMMENT_7D)]);
		if (one_in_(2)) chg_virtue(p_ptr, V_HONOUR, -1);
		if (one_in_(4)) chg_virtue(p_ptr, V_HONOUR, 1);
		if (10 * price < value) chg_virtue(p_ptr, V_SACRIFICE, 1);
		sound(SOUND_STORE4);
	}
}



/*
 * We store the current "store feat" here so everyone can access it
 */
static int cur_store_feat;


/*
 * Buying and selling adjustments for race combinations.
 * Entry[owner][player] gives the basic "cost inflation".
 */
static byte rgold_adj[MAX_RACES][MAX_RACES] =
{
	/*Hum, HfE, Elf,  Hal, Gno, Dwa, HfO, HfT, Dun, HiE, Barbarian,
	 HfOg, HGn, HTn, Cyc, Yek, Klc, Kbd, Nbl, DkE, Drc, Mind Flayer,
	 Imp,  Glm, Skl, Zombie, Vampire, Spectre, Fairy, Beastman, Ent,
	 Angel, Demon, Kutar, Android, Merfolk */

	/* Human */
	{ 100, 105, 105, 110, 113, 115, 120, 125, 100, 105, 100,
	  124, 120, 110, 125, 115, 120, 120, 120, 120, 115, 120,
	  115, 105, 125, 125, 125, 125, 105, 120, 105,  95, 140,
	  100, 120, 110, 105, 110 },

	/* Half-Elf */
	{ 110, 100, 100, 105, 110, 120, 125, 130, 110, 100, 110,
	  120, 115, 108, 115, 110, 110, 120, 120, 115, 115, 110,
	  120, 110, 110, 110, 120, 110, 100, 125, 100,  95, 140,
	  110, 115, 110, 110, 110 },

	/* Elf */
	{ 110, 105, 100, 105, 110, 120, 125, 130, 110, 100, 110,
	  120, 120, 105, 120, 110, 105, 125, 125, 110, 115, 108,
	  120, 115, 110, 110, 120, 110, 100, 125, 100,  95, 140,
	  110, 110, 105, 110, 110 },

	/* Halfling */
	{ 115, 110, 105,  95, 105, 110, 115, 130, 115, 105, 115,
	  125, 120, 120, 125, 115, 110, 120, 120, 120, 115, 115,
	  120, 110, 120, 120, 130, 110, 110, 130, 110,  95, 140,
	  115, 120, 105, 115, 105 },

	/* Gnome */
	{ 115, 115, 110, 105,  95, 110, 115, 130, 115, 110, 115,
	  120, 125, 110, 120, 110, 105, 120, 110, 110, 105, 110,
	  120, 101, 110, 110, 120, 120, 115, 130, 115,  95, 140,
	  115, 110, 110, 115, 110 },

	/* Dwarf */
	{ 115, 120, 120, 110, 110,  95, 125, 135, 115, 120, 115,
	  125, 140, 130, 130, 120, 115, 115, 115, 135, 125, 120,
	  120, 105, 115, 115, 115, 115, 120, 130, 120,  95, 140,
	  115, 110, 115, 115, 120 },

	/* Half-Orc */
	{ 115, 120, 125, 115, 115, 130, 110, 115, 115, 125, 115,
	  110, 110, 120, 110, 120, 125, 115, 115, 110, 120, 110,
	  115, 125, 120, 120, 115, 120, 125, 115, 125,  95, 140,
	  115, 110, 115, 115, 125 },

	/* Half-Troll */
	{ 110, 115, 115, 110, 110, 130, 110, 110, 110, 115, 110,
	  110, 115, 120, 110, 120, 120, 110, 110, 110, 115, 110,
	  110, 115, 112, 112, 115, 112, 120, 110, 120,  95, 140,
	  110, 110, 115, 110, 130 },

	/* Amberite */
	{ 100, 105, 105, 110, 113, 115, 120, 125, 100, 105, 100,
	  120, 120, 105, 120, 115, 105, 115, 120, 110, 105, 105,
	  120, 105, 120, 120, 125, 120, 105, 135, 105,  95, 140,
	  100, 110, 110, 100, 110 },

	/* High_Elf */
	{ 110, 105, 100, 105, 110, 120, 125, 130, 110, 100, 110,
	  125, 125, 101, 120, 115, 110, 115, 125, 110, 110, 110,
	  125, 115, 120, 120, 125, 120, 100, 125, 100,  95, 140,
	  110, 110, 105, 110, 110 },

	/* Human / Barbarian (copied from human) */
	{ 100, 105, 105, 110, 113, 115, 120, 125, 100, 105, 100,
	  124, 120, 110, 125, 115, 120, 120, 120, 120, 115, 120,
	  115, 105, 125, 125, 130, 125, 115, 120, 115,  95, 140,
	  100, 120, 110, 100, 110 },

	/* Half-Ogre: theoretical, copied from half-troll */
	{ 110, 115, 115, 110, 110, 130, 110, 110, 110, 115, 110,
	  110, 115, 120, 110, 120, 120, 110, 110, 110, 115, 110,
	  110, 115, 112, 112, 115, 112, 120, 110, 120,  95, 140,
	  110, 110, 115, 110, 120 },

	/* Half-Giant: theoretical, copied from half-troll */
	{ 110, 115, 115, 110, 110, 130, 110, 110, 110, 115, 110,
	  110, 115, 120, 110, 120, 120, 110, 110, 110, 115, 110,
	  110, 115, 112, 112, 115, 112, 130, 120, 130,  95, 140,
	  110, 110, 115, 110, 115 },

	/* Half-Titan: theoretical, copied from High_Elf */
	{ 110, 105, 100, 105, 110, 120, 125, 130, 110, 100, 110,
	  125, 125, 101, 120, 115, 110, 115, 125, 110, 110, 110,
	  125, 115, 120, 120, 120, 120, 130, 130, 130,  95, 140,
	  110, 110, 115, 110, 108 },

	/* Cyclops: theoretical, copied from half-troll */
	{ 110, 115, 115, 110, 110, 130, 110, 110, 110, 115, 110,
	  110, 115, 120, 110, 120, 120, 110, 110, 110, 115, 110,
	  110, 115, 112, 112, 115, 112, 130, 130, 130,  95, 140,
	  110, 110, 115, 110, 115 },

	/* Yeek: theoretical, copied from Half-Orc */
	{ 115, 120, 125, 115, 115, 130, 110, 115, 115, 125, 115,
	  110, 110, 120, 110, 120, 125, 115, 115, 110, 120, 110,
	  115, 125, 120, 120, 120, 120, 130, 130, 130,  95, 140,
	  115, 110, 115, 115, 110 },

	/* Klackon: theoretical, copied from Gnome */
	{ 115, 115, 110, 105,  95, 110, 115, 130, 115, 110, 115,
	  120, 125, 110, 120, 110, 105, 120, 110, 110, 105, 110,
	  120, 101, 110, 110, 120, 120, 130, 130, 130,  95, 140,
	  115, 110, 115, 115, 110 },

	/* Kobold: theoretical, copied from Half-Orc */
	{ 115, 120, 125, 115, 115, 130, 110, 115, 115, 125, 115,
	  110, 110, 120, 110, 120, 125, 115, 115, 110, 120, 110,
	  115, 125, 120, 120, 120, 120, 130, 130, 130,  95, 140,
	  115, 110, 115, 115, 120 },

	/* Nibelung: theoretical, copied from Dwarf */
	{ 115, 120, 120, 110, 110,  95, 125, 135, 115, 120, 115,
	  125, 140, 130, 130, 120, 115, 115, 115, 135, 125, 120,
	  120, 105, 115, 115, 120, 120, 130, 130, 130,  95, 140,
	  115, 135, 115, 115, 120 },

	/* Dark Elf */
	{ 110, 110, 110, 115, 120, 130, 115, 115, 120, 110, 115,
	  115, 115, 116, 115, 120, 120, 115, 115, 101, 110, 110,
	  110, 110, 112, 122, 110, 110, 110, 115, 110, 120, 120,
	  110, 101, 115, 110, 115 },

	/* Draconian: theoretical, copied from High_Elf */
	{ 110, 105, 100, 105, 110, 120, 125, 130, 110, 100, 110,
	  125, 125, 101, 120, 115, 110, 115, 125, 110, 110, 110,
	  125, 115, 120, 120, 120, 120, 130, 130, 130,  95, 140,
	  110, 110, 115, 110, 115 },

	/* Mind Flayer: theoretical, copied from High_Elf */
	{ 110, 105, 100, 105, 110, 120, 125, 130, 110, 100, 110,
	  125, 125, 101, 120, 115, 110, 115, 125, 110, 110, 110,
	  125, 115, 120, 120, 120, 120, 130, 130, 130,  95, 140,
	  110, 110, 115, 110, 110 },

	/* Imp: theoretical, copied from High_Elf */
	{ 110, 105, 100, 105, 110, 120, 125, 130, 110, 100, 110,
	  125, 125, 101, 120, 115, 110, 115, 125, 110, 110, 110,
	  125, 115, 120, 120, 120, 120, 130, 130, 130, 120, 120,
	  110, 110, 115, 110, 120 },

	/* Golem: theoretical, copied from High_Elf */
	{ 110, 105, 100, 105, 110, 120, 125, 130, 110, 100, 110,
	  125, 125, 101, 120, 115, 110, 115, 125, 110, 110, 110,
	  125, 115, 120, 120, 120, 120, 130, 130, 130,  95, 140,
	  110, 110, 115, 110, 110 },

	/* Skeleton: theoretical, copied from half-orc */
	{ 115, 120, 125, 115, 115, 130, 110, 115, 115, 125, 115,
	  110, 110, 120, 110, 120, 125, 115, 115, 110, 120, 110,
	  115, 125, 120, 120, 120, 120, 130, 130, 130, 120, 120,
	  115, 110, 125, 115, 110 },

	/* Zombie: Theoretical, copied from half-orc */
	{ 115, 120, 125, 115, 115, 130, 110, 115, 115, 125, 115,
	  110, 110, 120, 110, 120, 125, 115, 115, 110, 120, 110,
	  115, 125, 120, 120, 120, 120, 130, 130, 130, 120, 120,
	  115, 110, 125, 115, 110 },

	/* Vampire: Theoretical, copied from half-orc */
	{ 115, 120, 125, 115, 115, 130, 110, 115, 115, 125, 115,
	  110, 110, 120, 110, 120, 125, 115, 115, 110, 120, 110,
	  115, 125, 120, 120, 120, 120, 130, 130, 130, 120, 120,
	  115, 110, 125, 115, 120 },

	/* Spectre: Theoretical, copied from half-orc */
	{ 115, 120, 125, 115, 115, 130, 110, 115, 115, 125, 115,
	  110, 110, 120, 110, 120, 125, 115, 115, 110, 120, 110,
	  115, 125, 120, 120, 120, 120, 130, 130, 130, 120, 120,
	  115, 110, 125, 115, 110 },

	/* Sprite: Theoretical, copied from half-orc */
	{ 115, 120, 125, 115, 115, 130, 110, 115, 115, 125, 115,
	  110, 110, 120, 110, 120, 125, 115, 115, 110, 120, 110,
	  115, 125, 120, 120, 120, 120, 130, 130, 130,  95, 140,
	  115, 110, 105, 115, 110 },

	/* Beastman: Theoretical, copied from half-orc */
	{ 115, 120, 125, 115, 115, 130, 110, 115, 115, 125, 115,
	  110, 110, 120, 110, 120, 125, 115, 115, 110, 120, 110,
	  115, 125, 120, 120, 120, 120, 130, 130, 130,  95, 140,
	  115, 110, 115, 115, 125 },

	/* Ent */
	{ 110, 105, 100, 105, 110, 120, 125, 130, 110, 100, 110,
	  120, 120, 105, 120, 110, 105, 125, 125, 110, 115, 108,
	  120, 115, 110, 110, 120, 110, 100, 125, 100,  95, 140,
	  110, 110, 105, 110, 110 },

	/* Angel */
	{  95,  95,  95,  95,  95,  95,  95,  95,  95,  95,  95,
	   95,  95,  95,  95,  95,  95,  95,  95,  95,  95,  95,
	   95,  95,  95,  95,  95,  95,  95,  95,  95,  95, 160,
	   95,  95,  95,  95,  95 },

	/* Demon */
	{ 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140,
	  140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140,
	  140, 140, 140, 140, 140, 140, 140, 140, 140, 160, 120,
	  140, 140, 140, 140, 140 },

	/* Dunadan */
	{ 100, 105, 105, 110, 113, 115, 120, 125, 100, 105, 100,
	  124, 120, 110, 125, 115, 120, 120, 120, 120, 115, 120,
	  115, 105, 125, 125, 125, 125, 105, 120, 105,  95, 140,
	  100, 120, 110, 100, 110 },

	/* Shadow Fairy */
	{ 110, 110, 110, 115, 120, 130, 115, 115, 120, 110, 115,
	  115, 115, 116, 115, 120, 120, 115, 115, 101, 110, 110,
	  110, 110, 112, 122, 110, 110, 110, 115, 110, 120, 120,
	  110, 101, 115, 110, 115 },

	/* Kutar */
	{ 110, 110, 105, 105, 110, 115, 115, 115, 110, 105, 110,
	  115, 115, 115, 115, 115, 115, 115, 115, 115, 115, 115,
	  115, 115, 125, 125, 125, 125, 105, 115, 105,  95, 140,
	  110, 115, 100, 110, 110 },

	/* Android */
	{ 105, 105, 105, 110, 113, 115, 120, 125, 100, 105, 100,
	  124, 120, 110, 125, 115, 120, 120, 120, 120, 115, 120,
	  115, 105, 125, 125, 125, 125, 105, 120, 105,  95, 140,
	  100, 120, 110, 100, 110 },

	/* Merfolk */
	{ 110, 110, 110, 105, 110, 120, 125, 130, 110, 110, 110,
	  120, 115, 108, 115, 110, 110, 120, 120, 115, 115, 110,
	  120, 110, 110, 110, 120, 110, 110, 125, 110,  95, 140,
	  110, 115, 110, 110, 100 },
};



/*!
 * @brief 店舗価格を決定する /
 * Determine the price of an item (qty one) in a store.
 * @param o_ptr 店舗に並べるオブジェクト構造体の参照ポインタ
 * @param greed 店主の強欲度
 * @param flip TRUEならば店主にとっての買取価格、FALSEなら売出価格を計算
 * @return アイテムの店舗価格
 * @details 
 * <pre>
 * This function takes into account the player's charisma, and the
 * shop-keepers friendliness, and the shop-keeper's base greed, but
 * never lets a shop-keeper lose money in a transaction.
 * The "greed" value should exceed 100 when the player is "buying" the
 * item, and should be less than 100 when the player is "selling" it.
 * Hack -- the black market always charges twice as much as it should.
 * Charisma adjustment runs from 80 to 130
 * Racial adjustment runs from 95 to 130
 * Since greed/charisma/racial adjustments are centered at 100, we need
 * to adjust (by 200) to extract a usable multiplier.  Note that the
 * "greed" value is always something (?).
 * </pre>
 */
static PRICE price_item(object_type *o_ptr, int greed, bool flip)
{
	int factor;
	int adjust;
	PRICE price;

	/* Get the value of one of the items */
	price = object_value(o_ptr);

	/* Worthless items */
	if (price <= 0) return (0L);

	/* Compute the racial factor */
	factor = rgold_adj[ot_ptr->owner_race][p_ptr->prace];

	/* Add in the charisma factor */
	factor += adj_chr_gold[p_ptr->stat_ind[A_CHR]];


	/* Shop is buying */
	if (flip)
	{
		/* Adjust for greed */
		adjust = 100 + (300 - (greed + factor));

		/* Never get "silly" */
		if (adjust > 100) adjust = 100;

		/* Mega-Hack -- Black market sucks */
		if (cur_store_num == STORE_BLACK)
			price = price / 2;

		/* Compute the final price (with rounding) */
		/* Hack -- prevent underflow */
		price = (price * adjust + 50L) / 100L;
	}

	/* Shop is selling */
	else
	{
		/* Adjust for greed */
		adjust = 100 + ((greed + factor) - 300);

		/* Never get "silly" */
		if (adjust < 100) adjust = 100;

		/* Mega-Hack -- Black market sucks */
		if (cur_store_num == STORE_BLACK)
			price = price * 2;

		/* Compute the final price (with rounding) */
		/* Hack -- prevent overflow */
		price = (s32b)(((u32b)price * (u32b)adjust + 50UL) / 100UL);
	}

	/* Note -- Never become "free" */
	if (price <= 0L) return (1L);

	/* Return the price */
	return (price);
}


/*!
 * @brief 安価な消耗品の販売数を増やし、低確率で割引にする /
 * Certain "cheap" objects should be created in "piles"
 * @param o_ptr 店舗に並べるオブジェクト構造体の参照ポインタ
 * @return なし
 * @details 
 * <pre>
 * Some objects can be sold at a "discount" (in small piles)
 * </pre>
 */
static void mass_produce(object_type *o_ptr)
{
	int size = 1;
	DISCOUNT_RATE discount = 0;

	PRICE cost = object_value(o_ptr);


	/* Analyze the type */
	switch (o_ptr->tval)
	{
		/* Food, Flasks, and Lites */
		case TV_FOOD:
		case TV_FLASK:
		case TV_LITE:
		{
			if (cost <= 5L) size += damroll(3, 5);
			if (cost <= 20L) size += damroll(3, 5);
			if (cost <= 50L) size += damroll(2, 2);
			break;
		}

		case TV_POTION:
		case TV_SCROLL:
		{
			if (cost <= 60L) size += damroll(3, 5);
			if (cost <= 240L) size += damroll(1, 5);
			if (o_ptr->sval == SV_SCROLL_STAR_IDENTIFY) size += damroll(3, 5);
			if (o_ptr->sval == SV_SCROLL_STAR_REMOVE_CURSE) size += damroll(1, 4);
			break;
		}

		case TV_LIFE_BOOK:
		case TV_SORCERY_BOOK:
		case TV_NATURE_BOOK:
		case TV_CHAOS_BOOK:
		case TV_DEATH_BOOK:
		case TV_TRUMP_BOOK:
		case TV_ARCANE_BOOK:
		case TV_CRAFT_BOOK:
		case TV_DAEMON_BOOK:
		case TV_CRUSADE_BOOK:
		case TV_MUSIC_BOOK:
		case TV_HISSATSU_BOOK:
		case TV_HEX_BOOK:
		{
			if (cost <= 50L) size += damroll(2, 3);
			if (cost <= 500L) size += damroll(1, 3);
			break;
		}

		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_SHIELD:
		case TV_GLOVES:
		case TV_BOOTS:
		case TV_CLOAK:
		case TV_HELM:
		case TV_CROWN:
		case TV_SWORD:
		case TV_POLEARM:
		case TV_HAFTED:
		case TV_DIGGING:
		case TV_BOW:
		{
			if (object_is_artifact(o_ptr)) break;
			if (object_is_ego(o_ptr)) break;
			if (cost <= 10L) size += damroll(3, 5);
			if (cost <= 100L) size += damroll(3, 5);
			break;
		}

		case TV_SPIKE:
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		{
			if (cost <= 5L) size += damroll(5, 5);
			if (cost <= 50L) size += damroll(5, 5);
			if (cost <= 500L) size += damroll(5, 5);
			break;
		}

		case TV_FIGURINE:
		{
			if (cost <= 100L) size += damroll(2, 2);
			if (cost <= 1000L) size += damroll(2, 2);
			break;
		}

		case TV_CAPTURE:
		case TV_STATUE:
		case TV_CARD:
		{
			size = 1;
			break;
		}

		/*
		 * Because many rods (and a few wands and staffs) are useful mainly
		 * in quantity, the Black Market will occasionally have a bunch of
		 * one kind. -LM-
		 */
		case TV_ROD:
		case TV_WAND:
		case TV_STAFF:
		{
			if ((cur_store_num == STORE_BLACK) && one_in_(3))
			{
				if (cost < 1601L) size += damroll(1, 5);
				else if (cost < 3201L) size += damroll(1, 3);
			}
			break;
		}
	}


	/* Pick a discount */
	if (cost < 5)
	{
		discount = 0;
	}
	else if (one_in_(25))
	{
		discount = 25;
	}
	else if (one_in_(150))
	{
		discount = 50;
	}
	else if (one_in_(300))
	{
		discount = 75;
	}
	else if (one_in_(500))
	{
		discount = 90;
	}

	if (o_ptr->art_name)
	{
		discount = 0;
	}

	/* Save the discount */
	o_ptr->discount = discount;

	/* Save the total pile size */
	o_ptr->number = size - (size * discount / 100);

	/* Ensure that mass-produced rods and wands get the correct pvals. */
	if ((o_ptr->tval == TV_ROD) || (o_ptr->tval == TV_WAND))
	{
		o_ptr->pval *= (PARAMETER_VALUE)o_ptr->number;
	}
}



/*!
 * @brief 店舗に並べた品を同一品であるかどうか判定する /
 * Determine if a store item can "absorb" another item
 * @param o_ptr 判定するオブジェクト構造体の参照ポインタ1
 * @param j_ptr 判定するオブジェクト構造体の参照ポインタ2
 * @return 同一扱いできるならTRUEを返す
 * @details 
 * <pre>
 * See "object_similar()" for the same function for the "player"
 * </pre>
 */
static bool store_object_similar(object_type *o_ptr, object_type *j_ptr)
{
	int i;

	/* Hack -- Identical items cannot be stacked */
	if (o_ptr == j_ptr) return (0);

	/* Different objects cannot be stacked */
	if (o_ptr->k_idx != j_ptr->k_idx) return (0);

	/* Different charges (etc) cannot be stacked, unless wands or rods. */
	if ((o_ptr->pval != j_ptr->pval) && (o_ptr->tval != TV_WAND) && (o_ptr->tval != TV_ROD)) return (0);

	/* Require many identical values */
	if (o_ptr->to_h != j_ptr->to_h) return (0);
	if (o_ptr->to_d != j_ptr->to_d) return (0);
	if (o_ptr->to_a != j_ptr->to_a) return (0);

	/* Require identical "ego-item" names */
	if (o_ptr->name2 != j_ptr->name2) return (0);

	/* Artifacts don't stack! */
	if (object_is_artifact(o_ptr) || object_is_artifact(j_ptr)) return (0);

	/* Hack -- Identical art_flags! */
	for (i = 0; i < TR_FLAG_SIZE; i++)
		if (o_ptr->art_flags[i] != j_ptr->art_flags[i]) return (0);

	/* Hack -- Never stack "powerful" items */
	if (o_ptr->xtra1 || j_ptr->xtra1) return (0);

	/* Hack -- Never stack recharging items */
	if (o_ptr->timeout || j_ptr->timeout) return (0);

	/* Require many identical values */
	if (o_ptr->ac != j_ptr->ac)   return (0);
	if (o_ptr->dd != j_ptr->dd)   return (0);
	if (o_ptr->ds != j_ptr->ds)   return (0);

	/* Hack -- Never stack chests */
	if (o_ptr->tval == TV_CHEST) return (0);
	if (o_ptr->tval == TV_STATUE) return (0);
	if (o_ptr->tval == TV_CAPTURE) return (0);

	/* Require matching discounts */
	if (o_ptr->discount != j_ptr->discount) return (0);

	/* They match, so they must be similar */
	return (TRUE);
}


/*!
 * @brief 店舗に並べた品を重ね合わせできるかどうか判定する /
 * Allow a store item to absorb another item
 * @param o_ptr 判定するオブジェクト構造体の参照ポインタ1
 * @param j_ptr 判定するオブジェクト構造体の参照ポインタ2
 * @return 重ね合わせできるならTRUEを返す
 * @details 
 * <pre>
 * See "object_similar()" for the same function for the "player"
 * </pre>
 */
static void store_object_absorb(object_type *o_ptr, object_type *j_ptr)
{
	int max_num = (o_ptr->tval == TV_ROD) ?
		MIN(99, MAX_SHORT / k_info[o_ptr->k_idx].pval) : 99;
	int total = o_ptr->number + j_ptr->number;
	int diff = (total > max_num) ? total - max_num : 0;

	/* Combine quantity, lose excess items */
	o_ptr->number = (total > max_num) ? max_num : total;

	/* Hack -- if rods are stacking, add the pvals (maximum timeouts) together. -LM- */
	if (o_ptr->tval == TV_ROD)
	{
		o_ptr->pval += j_ptr->pval * (j_ptr->number - diff) / j_ptr->number;
	}

	/* Hack -- if wands are stacking, combine the charges. -LM- */
	if (o_ptr->tval == TV_WAND)
	{
		o_ptr->pval += j_ptr->pval * (j_ptr->number - diff) / j_ptr->number;
	}
}


/*!
 * @brief 店舗に品を置くスペースがあるかどうかの判定を返す /
 * Check to see if the shop will be carrying too many objects	-RAK-
 * @param o_ptr 店舗に置きたいオブジェクト構造体の参照ポインタ
 * @return 置き場がないなら0、重ね合わせできるアイテムがあるなら-1、スペースがあるなら1を返す。
 * @details 
 * <pre>
 * Note that the shop, just like a player, will not accept things
 * it cannot hold.	Before, one could "nuke" potions this way.
 * Return value is now int:
 *  0 : No space
 * -1 : Can be combined to existing slot.
 *  1 : Cannot be combined but there are empty spaces.
 * </pre>
 */
static int store_check_num(object_type *o_ptr)
{
	int i;
	object_type *j_ptr;

	/* The "home" acts like the player */
	if ((cur_store_num == STORE_HOME) || (cur_store_num == STORE_MUSEUM))
	{
		bool old_stack_force_notes = stack_force_notes;
		bool old_stack_force_costs = stack_force_costs;

		if (cur_store_num != STORE_HOME)
		{
			stack_force_notes = FALSE;
			stack_force_costs = FALSE;
		}

		/* Check all the items */
		for (i = 0; i < st_ptr->stock_num; i++)
		{
			/* Get the existing item */
			j_ptr = &st_ptr->stock[i];

			/* Can the new object be combined with the old one? */
			if (object_similar(j_ptr, o_ptr))
			{
				if (cur_store_num != STORE_HOME)
				{
					stack_force_notes = old_stack_force_notes;
					stack_force_costs = old_stack_force_costs;
				}

				return -1;
			}
		}

		if (cur_store_num != STORE_HOME)
		{
			stack_force_notes = old_stack_force_notes;
			stack_force_costs = old_stack_force_costs;
		}
	}

	/* Normal stores do special stuff */
	else
	{
		/* Check all the items */
		for (i = 0; i < st_ptr->stock_num; i++)
		{
			/* Get the existing item */
			j_ptr = &st_ptr->stock[i];

			/* Can the new object be combined with the old one? */
			if (store_object_similar(j_ptr, o_ptr)) return -1;
		}
	}

	/* Free space is always usable */
	/*
	 * オプション powerup_home が設定されていると
	 * 我が家が 20 ページまで使える
	 */
	if ((cur_store_num == STORE_HOME) && ( powerup_home == FALSE )) {
		if (st_ptr->stock_num < ((st_ptr->stock_size) / 10)) {
			return 1;
		}
	}
	else{
		if (st_ptr->stock_num < st_ptr->stock_size) {
			return 1;
		}
	}

	/* But there was no room at the inn... */
	return 0;
}

/*!
 * @brief オブジェクトが祝福されているかの判定を返す /
 * @param o_ptr 判定したいオブジェクト構造体の参照ポインタ
 * @return アイテムが祝福されたアイテムならばTRUEを返す
 */
static bool is_blessed(object_type *o_ptr)
{
	BIT_FLAGS flgs[TR_FLAG_SIZE];
	object_flags(o_ptr, flgs);
	if (have_flag(flgs, TR_BLESSED)) return (TRUE);
	else return (FALSE);
}



/*!
 * @brief オブジェクトが所定の店舗で引き取れるかどうかを返す /
 * Determine if the current store will purchase the given item
 * @param o_ptr 判定したいオブジェクト構造体の参照ポインタ
 * @return アイテムが買い取れるならばTRUEを返す
 * @note
 * Note that a shop-keeper must refuse to buy "worthless" items
 */
static bool store_will_buy(object_type *o_ptr)
{
	/* Hack -- The Home is simple */
	if ((cur_store_num == STORE_HOME) || (cur_store_num == STORE_MUSEUM)) return (TRUE);

	/* Switch on the store */
	switch (cur_store_num)
	{
		/* General Store */
		case STORE_GENERAL:
		{
			/* Analyze the type */
			switch (o_ptr->tval)
			{
				case TV_POTION:
					if (o_ptr->sval != SV_POTION_WATER) return FALSE;

				case TV_WHISTLE:
				case TV_FOOD:
				case TV_LITE:
				case TV_FLASK:
				case TV_SPIKE:
				case TV_SHOT:
				case TV_ARROW:
				case TV_BOLT:
				case TV_DIGGING:
				case TV_CLOAK:
				case TV_BOTTLE: /* 'Green', recycling Angband */
				case TV_FIGURINE:
				case TV_STATUE:
				case TV_CAPTURE:
				case TV_CARD:
				break;
				default:
				return (FALSE);
			}
			break;
		}

		/* Armoury */
		case STORE_ARMOURY:
		{
			/* Analyze the type */
			switch (o_ptr->tval)
			{
				case TV_BOOTS:
				case TV_GLOVES:
				case TV_CROWN:
				case TV_HELM:
				case TV_SHIELD:
				case TV_CLOAK:
				case TV_SOFT_ARMOR:
				case TV_HARD_ARMOR:
				case TV_DRAG_ARMOR:
				break;
				default:
				return (FALSE);
			}
			break;
		}

		/* Weapon Shop */
		case STORE_WEAPON:
		{
			/* Analyze the type */
			switch (o_ptr->tval)
			{
				case TV_SHOT:
				case TV_BOLT:
				case TV_ARROW:
				case TV_BOW:
				case TV_DIGGING:
				case TV_POLEARM:
				case TV_SWORD:
				case TV_HISSATSU_BOOK:
				break;
				case TV_HAFTED:
				{
					if(o_ptr->sval == SV_WIZSTAFF) return (FALSE);
				}
				break;
				default:
				return (FALSE);
			}
			break;
		}

		/* Temple */
		case STORE_TEMPLE:
		{
			/* Analyze the type */
			switch (o_ptr->tval)
			{
				case TV_LIFE_BOOK:
				case TV_CRUSADE_BOOK:
				case TV_SCROLL:
				case TV_POTION:
				case TV_HAFTED:
				{
					break;
				}
				case TV_FIGURINE:
				case TV_STATUE:
				{
					monster_race *r_ptr = &r_info[o_ptr->pval];

					/* Decline evil */
					if (!(r_ptr->flags3 & RF3_EVIL))
					{
						/* Accept good */
						if (r_ptr->flags3 & RF3_GOOD) break;

						/* Accept animals */
						if (r_ptr->flags3 & RF3_ANIMAL) break;

						/* Accept mimics */
						if (my_strchr("?!", r_ptr->d_char)) break;
					}
				}
				case TV_POLEARM:
				case TV_SWORD:
				{
					if (is_blessed(o_ptr)) break;
				}
				default:
				return (FALSE);
			}
			break;
		}

		/* Alchemist */
		case STORE_ALCHEMIST:
		{
			/* Analyze the type */
			switch (o_ptr->tval)
			{
				case TV_SCROLL:
				case TV_POTION:
				break;
				default:
				return (FALSE);
			}
			break;
		}

		/* Magic Shop */
		case STORE_MAGIC:
		{
			/* Analyze the type */
			switch (o_ptr->tval)
			{
				case TV_SORCERY_BOOK:
				case TV_NATURE_BOOK:
				case TV_CHAOS_BOOK:
				case TV_DEATH_BOOK:
				case TV_TRUMP_BOOK:
				case TV_ARCANE_BOOK:
				case TV_CRAFT_BOOK:
				case TV_DAEMON_BOOK:
				case TV_MUSIC_BOOK:
				case TV_HEX_BOOK:
				case TV_AMULET:
				case TV_RING:
				case TV_STAFF:
				case TV_WAND:
				case TV_ROD:
				case TV_SCROLL:
				case TV_POTION:
				case TV_FIGURINE:
				break;
				case TV_HAFTED:
				{
					if(o_ptr->sval == SV_WIZSTAFF) break;
					else return (FALSE);
				}
				default:
				return (FALSE);
			}
			break;
		}
		/* Bookstore Shop */
		case STORE_BOOK:
		{
			/* Analyze the type */
			switch (o_ptr->tval)
			{
				case TV_SORCERY_BOOK:
				case TV_NATURE_BOOK:
				case TV_CHAOS_BOOK:
				case TV_DEATH_BOOK:
				case TV_LIFE_BOOK:
				case TV_TRUMP_BOOK:
				case TV_ARCANE_BOOK:
				case TV_CRAFT_BOOK:
				case TV_DAEMON_BOOK:
				case TV_CRUSADE_BOOK:
				case TV_MUSIC_BOOK:
				case TV_HEX_BOOK:
					break;
				default:
					return (FALSE);
			}
			break;
		}
	}

	/* Ignore "worthless" items */
	if (object_value(o_ptr) <= 0) return (FALSE);

	/* Assume okay */
	return (TRUE);
}


/*!
 * @brief 現在の町の指定された店舗のアイテムを整理する /
 * Combine and reorder items in store.
 * @param store_num 店舗ID
 * @return 実際に整理が行われたならばTRUEを返す。
 */
bool combine_and_reorder_home(int store_num)
{
	int         i, j, k;
	s32b        o_value;
	object_type forge, *o_ptr, *j_ptr;
	bool        flag = FALSE, combined;
	store_type  *old_st_ptr = st_ptr;
	bool        old_stack_force_notes = stack_force_notes;
	bool        old_stack_force_costs = stack_force_costs;

	st_ptr = &town_info[1].store[store_num];
	if (store_num != STORE_HOME)
	{
		stack_force_notes = FALSE;
		stack_force_costs = FALSE;
	}

	do
	{
		combined = FALSE;

		/* Combine the items in the home (backwards) */
		for (i = st_ptr->stock_num - 1; i > 0; i--)
		{
			o_ptr = &st_ptr->stock[i];

			/* Skip empty items */
			if (!o_ptr->k_idx) continue;

			/* Scan the items above that item */
			for (j = 0; j < i; j++)
			{
				int max_num;

				j_ptr = &st_ptr->stock[j];

				/* Skip empty items */
				if (!j_ptr->k_idx) continue;

				/*
				 * Get maximum number of the stack if these
				 * are similar, get zero otherwise.
				 */
				max_num = object_similar_part(j_ptr, o_ptr);

				/* Can we (partialy) drop "o_ptr" onto "j_ptr"? */
				if (max_num && j_ptr->number < max_num)
				{
					if (o_ptr->number + j_ptr->number <= max_num)
					{
						/* Add together the item counts */
						object_absorb(j_ptr, o_ptr);

						/* One object is gone */
						st_ptr->stock_num--;

						/* Slide everything down */
						for (k = i; k < st_ptr->stock_num; k++)
						{
							/* Structure copy */
							st_ptr->stock[k] = st_ptr->stock[k + 1];
						}

						/* Erase the "final" slot */
						object_wipe(&st_ptr->stock[k]);
					}
					else
					{
						ITEM_NUMBER old_num = o_ptr->number;
						ITEM_NUMBER remain = j_ptr->number + o_ptr->number - max_num;

						/* Add together the item counts */
						object_absorb(j_ptr, o_ptr);

						o_ptr->number = remain;

						/* Hack -- if rods are stacking, add the pvals (maximum timeouts) and current timeouts together. -LM- */
						if (o_ptr->tval == TV_ROD)
						{
							o_ptr->pval =  o_ptr->pval * remain / old_num;
							o_ptr->timeout = o_ptr->timeout * remain / old_num;
						}

						/* Hack -- if wands are stacking, combine the charges. -LM- */
						else if (o_ptr->tval == TV_WAND)
						{
							o_ptr->pval = o_ptr->pval * remain / old_num;
						}
					}

					/* Take note */
					combined = TRUE;
					break;
				}
			}
		}

		flag |= combined;
	}
	while (combined);

	/* Re-order the items in the home (forwards) */
	for (i = 0; i < st_ptr->stock_num; i++)
	{
		o_ptr = &st_ptr->stock[i];

		/* Skip empty slots */
		if (!o_ptr->k_idx) continue;

		/* Get the "value" of the item */
		o_value = object_value(o_ptr);

		/* Scan every occupied slot */
		for (j = 0; j < st_ptr->stock_num; j++)
		{
			if (object_sort_comp(o_ptr, o_value, &st_ptr->stock[j])) break;
		}

		/* Never move down */
		if (j >= i) continue;

		/* Take note */
		flag = TRUE;
		j_ptr = &forge;

		/* Save a copy of the moving item */
		object_copy(j_ptr, &st_ptr->stock[i]);

		/* Slide the objects */
		for (k = i; k > j; k--)
		{
			/* Slide the item */
			object_copy(&st_ptr->stock[k], &st_ptr->stock[k - 1]);
		}

		/* Insert the moving item */
		object_copy(&st_ptr->stock[j], j_ptr);
	}

	st_ptr = old_st_ptr;
	if (store_num != STORE_HOME)
	{
		stack_force_notes = old_stack_force_notes;
		stack_force_costs = old_stack_force_costs;
	}

	return flag;
}


/*!
 * @brief 我が家にオブジェクトを加える /
 * Add the item "o_ptr" to the inventory of the "Home"
 * @param o_ptr 加えたいオブジェクトの構造体参照ポインタ
 * @return 収めた先のID
 * @details
 * <pre>
 * In all cases, return the slot (or -1) where the object was placed
 * Note that this is a hacked up version of "inven_carry()".
 * Also note that it may not correctly "adapt" to "knowledge" bacoming
 * known, the player may have to pick stuff up and drop it again.
 * </pre>
 */
static int home_carry(object_type *o_ptr)
{
	int slot;
	PRICE value;
	int i;
	object_type *j_ptr;
	bool old_stack_force_notes = stack_force_notes;
	bool old_stack_force_costs = stack_force_costs;

	if (cur_store_num != STORE_HOME)
	{
		stack_force_notes = FALSE;
		stack_force_costs = FALSE;
	}

	/* Check each existing item (try to combine) */
	for (slot = 0; slot < st_ptr->stock_num; slot++)
	{
		/* Get the existing item */
		j_ptr = &st_ptr->stock[slot];

		/* The home acts just like the player */
		if (object_similar(j_ptr, o_ptr))
		{
			/* Save the new number of items */
			object_absorb(j_ptr, o_ptr);

			if (cur_store_num != STORE_HOME)
			{
				stack_force_notes = old_stack_force_notes;
				stack_force_costs = old_stack_force_costs;
			}

			/* All done */
			return (slot);
		}
	}

	if (cur_store_num != STORE_HOME)
	{
		stack_force_notes = old_stack_force_notes;
		stack_force_costs = old_stack_force_costs;
	}

	/* No space? */
	/*
	 * 隠し機能: オプション powerup_home が設定されていると
	 *           我が家が 20 ページまで使える
	 */
	/* No space? */
	if ((cur_store_num != STORE_HOME) || (powerup_home == TRUE)) {
		if (st_ptr->stock_num >= st_ptr->stock_size) {
			return (-1);
		}
	}
	else{
		if (st_ptr->stock_num >= ((st_ptr->stock_size) / 10)) {
			return (-1);
		}
	}


	/* Determine the "value" of the item */
	value = object_value(o_ptr);

	/* Check existing slots to see if we must "slide" */
	for (slot = 0; slot < st_ptr->stock_num; slot++)
	{
		if (object_sort_comp(o_ptr, value, &st_ptr->stock[slot])) break;
	}

	/* Slide the others up */
	for (i = st_ptr->stock_num; i > slot; i--)
	{
		st_ptr->stock[i] = st_ptr->stock[i-1];
	}

	/* More stuff now */
	st_ptr->stock_num++;

	/* Insert the new item */
	st_ptr->stock[slot] = *o_ptr;

	chg_virtue(p_ptr, V_SACRIFICE, -1);

	(void)combine_and_reorder_home(cur_store_num);

	/* Return the location */
	return (slot);
}


/*!
 * @brief 店舗にオブジェクトを加える /
 * Add the item "o_ptr" to a real stores inventory.
 * @param o_ptr 加えたいオブジェクトの構造体参照ポインタ
 * @return 収めた先のID
 * @details
 * <pre>
 * In all cases, return the slot (or -1) where the object was placed
 * Note that this is a hacked up version of "inven_carry()".
 * Also note that it may not correctly "adapt" to "knowledge" bacoming
 * known, the player may have to pick stuff up and drop it again.
 * </pre>
 */
static int store_carry(object_type *o_ptr)
{
	int i, slot;
	PRICE value, j_value;
	object_type *j_ptr;


	/* Evaluate the object */
	value = object_value(o_ptr);

	/* Cursed/Worthless items "disappear" when sold */
	if (value <= 0) return (-1);

	/* All store items are fully *identified* */
	o_ptr->ident |= IDENT_MENTAL;

	/* Erase the inscription */
	o_ptr->inscription = 0;

	/* Erase the "feeling" */
	o_ptr->feeling = FEEL_NONE;

	/* Check each existing item (try to combine) */
	for (slot = 0; slot < st_ptr->stock_num; slot++)
	{
		/* Get the existing item */
		j_ptr = &st_ptr->stock[slot];

		/* Can the existing items be incremented? */
		if (store_object_similar(j_ptr, o_ptr))
		{
			/* Hack -- extra items disappear */
			store_object_absorb(j_ptr, o_ptr);

			/* All done */
			return (slot);
		}
	}

	/* No space? */
	if (st_ptr->stock_num >= st_ptr->stock_size) return (-1);


	/* Check existing slots to see if we must "slide" */
	for (slot = 0; slot < st_ptr->stock_num; slot++)
	{
		/* Get that item */
		j_ptr = &st_ptr->stock[slot];

		/* Objects sort by decreasing type */
		if (o_ptr->tval > j_ptr->tval) break;
		if (o_ptr->tval < j_ptr->tval) continue;

		/* Objects sort by increasing sval */
		if (o_ptr->sval < j_ptr->sval) break;
		if (o_ptr->sval > j_ptr->sval) continue;

		/*
		 * Hack:  otherwise identical rods sort by
		 * increasing recharge time --dsb
		 */
		if (o_ptr->tval == TV_ROD)
		{
			if (o_ptr->pval < j_ptr->pval) break;
			if (o_ptr->pval > j_ptr->pval) continue;
		}

		/* Evaluate that slot */
		j_value = object_value(j_ptr);

		/* Objects sort by decreasing value */
		if (value > j_value) break;
		if (value < j_value) continue;
	}

	/* Slide the others up */
	for (i = st_ptr->stock_num; i > slot; i--)
	{
		st_ptr->stock[i] = st_ptr->stock[i-1];
	}

	/* More stuff now */
	st_ptr->stock_num++;

	/* Insert the new item */
	st_ptr->stock[slot] = *o_ptr;

	/* Return the location */
	return (slot);
}


/*!
 * @brief 店舗のオブジェクト数を増やす /
 * Add the item "o_ptr" to a real stores inventory.
 * @param item 増やしたいアイテムのID
 * @param num 増やしたい数
 * @return なし
 * @details
 * <pre>
 * Increase, by a given amount, the number of a certain item
 * in a certain store.	This can result in zero items.
 * </pre>
 * @todo numは本来ITEM_NUMBER型にしたい。
 */
static void store_item_increase(INVENTORY_IDX item, int num)
{
	int 		cnt;
	object_type *o_ptr;

	o_ptr = &st_ptr->stock[item];

	/* Verify the number */
	cnt = o_ptr->number + num;
	if (cnt > 255) cnt = 255;
	else if (cnt < 0) cnt = 0;
	num = cnt - o_ptr->number;

	/* Save the new number */
	o_ptr->number += (ITEM_NUMBER)num;
}


/*!
 * @brief 店舗のオブジェクト数を削除する /
 * Remove a slot if it is empty
 * @param item 削除したいアイテムのID
 * @return なし
 */
static void store_item_optimize(INVENTORY_IDX item)
{
	int 		j;
	object_type *o_ptr;

	o_ptr = &st_ptr->stock[item];

	/* Must exist */
	if (!o_ptr->k_idx) return;

	/* Must have no items */
	if (o_ptr->number) return;

	/* One less item */
	st_ptr->stock_num--;

	/* Slide everyone */
	for (j = item; j < st_ptr->stock_num; j++)
	{
		st_ptr->stock[j] = st_ptr->stock[j + 1];
	}

	/* Nuke the final slot */
	object_wipe(&st_ptr->stock[j]);
}

/*!
 * @brief ブラックマーケット用の無価値品の排除判定 /
 * This function will keep 'crap' out of the black market.
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return ブラックマーケットにとって無価値な品ならばTRUEを返す
 * @details
 * <pre>
 * Crap is defined as any item that is "available" elsewhere
 * Based on a suggestion by "Lee Vogt" <lvogt@cig.mcel.mot.com>
 * </pre>
 */
static bool black_market_crap(object_type *o_ptr)
{
	int i, j;

	/* Ego items are never crap */
	if (object_is_ego(o_ptr)) return (FALSE);

	/* Good items are never crap */
	if (o_ptr->to_a > 0) return (FALSE);
	if (o_ptr->to_h > 0) return (FALSE);
	if (o_ptr->to_d > 0) return (FALSE);

	/* Check all stores */
	for (i = 0; i < MAX_STORES; i++)
	{
		if (i == STORE_HOME) continue;
		if (i == STORE_MUSEUM) continue;

		/* Check every item in the store */
		for (j = 0; j < town_info[p_ptr->town_num].store[i].stock_num; j++)
		{
			object_type *j_ptr = &town_info[p_ptr->town_num].store[i].stock[j];

			/* Duplicate item "type", assume crappy */
			if (o_ptr->k_idx == j_ptr->k_idx) return (TRUE);
		}
	}

	/* Assume okay */
	return (FALSE);
}


/*!
 * @brief 店舗の品揃え変化のためにアイテムを削除する /
 * Attempt to delete (some of) a random item from the store
 * @return なし
 * @details
 * <pre>
 * Hack -- we attempt to "maintain" piles of items when possible.
 * </pre>
 */
static void store_delete(void)
{
	INVENTORY_IDX what;
	int num;

	/* Pick a random slot */
	what = (INVENTORY_IDX)randint0(st_ptr->stock_num);

	/* Determine how many items are here */
	num = st_ptr->stock[what].number;

	/* Hack -- sometimes, only destroy half the items */
	if (randint0(100) < 50) num = (num + 1) / 2;

	/* Hack -- sometimes, only destroy a single item */
	if (randint0(100) < 50) num = 1;

	/* Hack -- decrement the maximum timeouts and total charges of rods and wands. -LM- */
	if ((st_ptr->stock[what].tval == TV_ROD) || (st_ptr->stock[what].tval == TV_WAND))
	{
		st_ptr->stock[what].pval -= num * st_ptr->stock[what].pval / st_ptr->stock[what].number;
	}

	/* Actually destroy (part of) the item */
	store_item_increase(what, -num);
	store_item_optimize(what);
}


/*!
 * @brief 店舗の品揃え変化のためにアイテムを追加する /
 * Creates a random item and gives it to a store
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * <pre>
 * This algorithm needs to be rethought.  A lot.
 * Currently, "normal" stores use a pre-built array.
 * Note -- the "level" given to "obj_get_num()" is a "favored"
 * level, that is, there is a much higher chance of getting
 * items with a level approaching that of the given level...
 * Should we check for "permission" to have the given item?
 * </pre>
 */
static void store_create(player_type *player_ptr)
{
	OBJECT_IDX i;
	int tries;
	DEPTH level;

	object_type forge;
	object_type *q_ptr;

	/* Paranoia -- no room left */
	if (st_ptr->stock_num >= st_ptr->stock_size) return;

	/* Hack -- consider up to four items */
	for (tries = 0; tries < 4; tries++)
	{
		/* Black Market */
		if (cur_store_num == STORE_BLACK)
		{
			/* Pick a level for object/magic */
			level = 25 + randint0(25);

			/* Random item (usually of given level) */
			i = get_obj_num(player_ptr, level, 0x00000000);

			/* Handle failure */
			if (!i) continue;
		}

		/* Normal Store */
		else
		{
			/* Hack -- Pick an item to sell */
			i = st_ptr->table[randint0(st_ptr->table_num)];

			/* Hack -- fake level for apply_magic() */
			level = rand_range(1, STORE_OBJ_LEVEL);
		}

		q_ptr = &forge;

		/* Create a new object of the chosen kind */
		object_prep(q_ptr, i);

		/* Apply some "low-level" magic (no artifacts) */
		apply_magic(player_ptr, q_ptr, level, AM_NO_FIXED_ART);

		/* Require valid object */
		if (!store_will_buy(q_ptr)) continue;

		/* Hack -- Charge lite's */
		if (q_ptr->tval == TV_LITE)
		{
			if (q_ptr->sval == SV_LITE_TORCH) q_ptr->xtra4 = FUEL_TORCH / 2;
			if (q_ptr->sval == SV_LITE_LANTERN) q_ptr->xtra4 = FUEL_LAMP / 2;
		}


		/* The item is "known" */
		object_known(q_ptr);

		/* Mark it storebought */
		q_ptr->ident |= IDENT_STORE;

		/* Mega-Hack -- no chests in stores */
		if (q_ptr->tval == TV_CHEST) continue;

		/* Prune the black market */
		if (cur_store_num == STORE_BLACK)
		{
			/* Hack -- No "crappy" items */
			if (black_market_crap(q_ptr)) continue;

			/* Hack -- No "cheap" items */
			if (object_value(q_ptr) < 10) continue;

			/* No "worthless" items */
			/* if (object_value(q_ptr) <= 0) continue; */
		}

		/* Prune normal stores */
		else
		{
			/* No "worthless" items */
			if (object_value(q_ptr) <= 0) continue;
		}


		/* Mass produce and/or Apply discount */
		mass_produce(q_ptr);

		/* Attempt to carry the (known) item */
		(void)store_carry(q_ptr);

		/* Definitely done */
		break;
	}
}


/*!
 * @brief 店舗の割引対象外にするかどうかを判定 /
 * Eliminate need to bargain if player has haggled well in the past
 * @param minprice アイテムの最低販売価格
 * @return 割引を禁止するならTRUEを返す。
 */
static bool noneedtobargain(PRICE minprice)
{
	PRICE good = st_ptr->good_buy;
	PRICE bad = st_ptr->bad_buy;

	/* Cheap items are "boring" */
	if (minprice < 10L) return (TRUE);

	/* Perfect haggling */
	if (good == MAX_SHORT) return (TRUE);

	/* Reward good haggles, punish bad haggles, notice price */
	if (good > ((3 * bad) + (5 + (minprice/50)))) return (TRUE);

	/* Return the flag */
	return (FALSE);
}


/*!
 * @brief 店主の持つプレイヤーに対する売買の良し悪し経験を記憶する /
 * Update the bargain info
 * @param price 実際の取引価格
 * @param minprice 店主の提示した価格
 * @param num 売買数 
 * @return なし
 */
static void updatebargain(PRICE price, PRICE minprice, int num)
{
	/* Hack -- auto-haggle */
	if (!manual_haggle) return;

	/* Cheap items are "boring" */
	if ((minprice/num) < 10L) return;

	/* Count the successful haggles */
	if (price == minprice)
	{
		/* Just count the good haggles */
		if (st_ptr->good_buy < MAX_SHORT)
		{
			st_ptr->good_buy++;
		}
	}

	/* Count the failed haggles */
	else
	{
		/* Just count the bad haggles */
		if (st_ptr->bad_buy < MAX_SHORT)
		{
			st_ptr->bad_buy++;
		}
	}
}


/*!
 * @brief 店の商品リストを再表示する /
 * Re-displays a single store entry
 * @param pos 表示行
 * @return なし
 */
static void display_entry(int pos)
{
	int i, cur_col;
	object_type *o_ptr;
	s32b x;

	GAME_TEXT o_name[MAX_NLEN];
	char out_val[160];

	int maxwid = 75;

	o_ptr = &st_ptr->stock[pos];

	/* Get the "offset" */
	i = (pos % store_bottom);

	/* Label it, clear the line --(-- */
	(void)sprintf(out_val, "%c) ", ((i > 25) ? toupper(I2A(i - 26)) : I2A(i)));
	prt(out_val, i+6, 0);

	cur_col = 3;
	if (show_item_graph)
	{
		TERM_COLOR a = object_attr(o_ptr);
		SYMBOL_CODE c = object_char(o_ptr);

		Term_queue_bigchar(cur_col, i + 6, a, c, 0, 0);
		if (use_bigtile) cur_col++;

		cur_col += 2;
	}

	/* Describe an item in the home */
	if ((cur_store_num == STORE_HOME) || (cur_store_num == STORE_MUSEUM))
	{
		maxwid = 75;

		/* Leave room for weights, if necessary -DRS- */
		if (show_weights) maxwid -= 10;

		object_desc(o_name, o_ptr, 0);
		o_name[maxwid] = '\0';
		c_put_str(tval_to_attr[o_ptr->tval], o_name, i+6, cur_col);

		/* Show weights */
		if(show_weights)
		{
			/* Only show the weight of an individual item */
			WEIGHT wgt = o_ptr->weight;
#ifdef JP
			sprintf(out_val, "%3d.%1d kg", lbtokg1(wgt) , lbtokg2(wgt));
			put_str(out_val, i+6, 67);
#else
			(void)sprintf(out_val, "%3d.%d lb", wgt / 10, wgt % 10);
			put_str(out_val, i+6, 68);
#endif

		}
	}

	/* Describe an item (fully) in a store */
	else
	{
		/* Must leave room for the "price" */
		maxwid = 65;

		/* Leave room for weights, if necessary -DRS- */
		if (show_weights) maxwid -= 7;

		/* Describe the object (fully) */
		object_desc(o_name, o_ptr, 0);
		o_name[maxwid] = '\0';
		c_put_str(tval_to_attr[o_ptr->tval], o_name, i+6, cur_col);

		/* Show weights */
		if (show_weights)
		{
			/* Only show the weight of an individual item */
			int wgt = o_ptr->weight;
#ifdef JP
			sprintf(out_val, "%3d.%1d", lbtokg1(wgt) , lbtokg2(wgt) );
			put_str(out_val, i+6, 60);
#else
			(void)sprintf(out_val, "%3d.%d", wgt / 10, wgt % 10);
			put_str(out_val, i+6, 61);
#endif

		}

		/* Display a "fixed" cost */
		if (o_ptr->ident & (IDENT_FIXED))
		{
			/* Extract the "minimum" price */
			x = price_item(o_ptr, ot_ptr->min_inflate, FALSE);

			/* Actually draw the price (not fixed) */
#ifdef JP
			(void)sprintf(out_val, "%9ld固", (long)x);
#else
			(void)sprintf(out_val, "%9ld F", (long)x);
#endif
			put_str(out_val, i+6, 68);
		}

		/* Display a "taxed" cost */
		else if (!manual_haggle)
		{
			/* Extract the "minimum" price */
			x = price_item(o_ptr, ot_ptr->min_inflate, FALSE);

			/* Hack -- Apply Sales Tax if needed */
			if (!noneedtobargain(x)) x += x / 10;

			/* Actually draw the price (with tax) */
			(void)sprintf(out_val, "%9ld  ", (long)x);
			put_str(out_val, i+6, 68);
		}

		/* Display a "haggle" cost */
		else
		{
			/* Extrect the "maximum" price */
			x = price_item(o_ptr, ot_ptr->max_inflate, FALSE);

			/* Actually draw the price (not fixed) */
			(void)sprintf(out_val, "%9ld  ", (long)x);
			put_str(out_val, i+6, 68);
		}
	}
}


/*!
 * @brief 店の商品リストを表示する /
 * Displays a store's inventory -RAK-
 * @return なし
 * @details
 * All prices are listed as "per individual object".  -BEN-
 */
static void display_inventory(void)
{
	int i, k;

	/* Display the next 12 items */
	for (k = 0; k < store_bottom; k++)
	{
		/* Do not display "dead" items */
		if (store_top + k >= st_ptr->stock_num) break;

		/* Display that line */
		display_entry(store_top + k);
	}

	/* Erase the extra lines and the "more" prompt */
	for (i = k; i < store_bottom + 1; i++) prt("", i + 6, 0);

	/* Assume "no current page" */
#ifdef JP
	put_str("          ", 5, 20);
#else
	put_str("        ", 5, 20);
#endif


	/* Visual reminder of "more items" */
	if (st_ptr->stock_num > store_bottom)
	{
		/* Show "more" reminder (after the last item) */
		prt(_("-続く-", "-more-"), k + 6, 3);

		/* Indicate the "current page" */
		/* Trailing spaces are to display (Page xx) and (Page x) */
		put_str(format(_("(%dページ)  ", "(Page %d)  "), store_top/store_bottom + 1), 5, 20);
	}

	if (cur_store_num == STORE_HOME || cur_store_num == STORE_MUSEUM)
	{
		k = st_ptr->stock_size;

		if (cur_store_num == STORE_HOME && !powerup_home) k /= 10;
#ifdef JP
		put_str(format("アイテム数:  %4d/%4d", st_ptr->stock_num, k), 19 + xtra_stock, 27);
#else
		put_str(format("Objects:  %4d/%4d", st_ptr->stock_num, k), 19 + xtra_stock, 30);
#endif
	}
}


/*!
 * @brief プレイヤーの所持金を表示する /
 * Displays players gold					-RAK-
 * @return なし
 * @details
 */
static void store_prt_gold(void)
{
	char out_val[64];

	prt(_("手持ちのお金: ", "Gold Remaining: "), 19 + xtra_stock, 53);
	sprintf(out_val, "%9ld", (long)p_ptr->au);
	prt(out_val, 19 + xtra_stock, 68);
}

/*!
 * @brief 店舗情報全体を表示するメインルーチン /
 * Displays store (after clearing screen)		-RAK-
 * @return なし
 * @details
 */
static void display_store(void)
{
	char buf[80];

	Term_clear();

	/* The "Home" is special */
	if (cur_store_num == STORE_HOME)
	{
		/* Put the owner name */
		put_str(_("我が家", "Your Home"), 3, 31);

		/* Label the item descriptions */
		put_str(_("アイテムの一覧", "Item Description"), 5, 4);

		/* If showing weights, show label */
		if (show_weights)
		{
			put_str(_("  重さ", "Weight"), 5, 70);
		}
	}

	/* The "Home" is special */
	else if (cur_store_num == STORE_MUSEUM)
	{
		/* Put the owner name */
		put_str(_("博物館", "Museum"), 3, 31);

		/* Label the item descriptions */
		put_str(_("アイテムの一覧", "Item Description"), 5, 4);

		/* If showing weights, show label */
		if (show_weights)
		{
			put_str(_("  重さ", "Weight"), 5, 70);
		}
	}

	/* Normal stores */
	else
	{
		concptr store_name = (f_name + f_info[cur_store_feat].name);
		concptr owner_name = (ot_ptr->owner_name);
		concptr race_name = race_info[ot_ptr->owner_race].title;

		/* Put the owner name and race */
		sprintf(buf, "%s (%s)", owner_name, race_name);
		put_str(buf, 3, 10);

		/* Show the max price in the store (above prices) */
		sprintf(buf, "%s (%ld)", store_name, (long)(ot_ptr->max_cost));
		prt(buf, 3, 50);

		/* Label the item descriptions */
		put_str(_("商品の一覧", "Item Description"), 5, 5);


		/* If showing weights, show label */
		if (show_weights)
		{
			put_str(_("  重さ", "Weight"), 5, 60);
		}

		/* Label the asking price (in stores) */
		put_str(_(" 価格", "Price"), 5, 72);
	}

	/* Display the current gold */
	store_prt_gold();
	display_inventory();
}



/*!
 * @brief 店舗からアイテムを選択する /
 * Get the ID of a store item and return its value	-RAK-
 * @param com_val 選択IDを返す参照ポインタ
 * @param pmt メッセージキャプション
 * @param i 選択範囲の最小値
 * @param j 選択範囲の最大値
 * @return 実際に選択したらTRUE、キャンセルしたらFALSE
 */
static int get_stock(COMMAND_CODE *com_val, concptr pmt, int i, int j)
{
	char command;
	char out_val[160];
	char lo, hi;

	/* Get the item index */
	if (repeat_pull(com_val))
	{
		/* Verify the item */
		if ((*com_val >= i) && (*com_val <= j))
		{
			/* Success */
			return (TRUE);
		}
	}
	msg_print(NULL);

	/* Assume failure */
	*com_val = (-1);

	/* Build the prompt */
	lo = I2A(i);
	hi = (j > 25) ? toupper(I2A(j - 26)) : I2A(j);
#ifdef JP
	(void)sprintf(out_val, "(%s:%c-%c, ESCで中断) %s",
		(((cur_store_num == STORE_HOME) || (cur_store_num == STORE_MUSEUM)) ? "アイテム" : "商品"), 
				  lo, hi, pmt);
#else
	(void)sprintf(out_val, "(Items %c-%c, ESC to exit) %s",
				  lo, hi, pmt);
#endif


	/* Ask until done */
	while (TRUE)
	{
		COMMAND_CODE k;

		/* Escape */
		if (!get_com(out_val, &command, FALSE)) break;

		/* Convert */
		if (islower(command))
			k = A2I(command);
		else if (isupper(command))
			k = A2I(tolower(command)) + 26;
		else
			k = -1;

		/* Legal responses */
		if ((k >= i) && (k <= j))
		{
			*com_val = k;
			break;
		}

		bell();
	}

	/* Clear the prompt */
	prt("", 0, 0);

	if (command == ESCAPE) return (FALSE);

	repeat_push(*com_val);

	/* Success */
	return (TRUE);
}


/*!
 * @brief 店主の不満度を増やし、プレイヤーを締め出す判定と処理を行う /
 * Increase the insult counter and get angry if too many -RAK-
 * @return プレイヤーを締め出す場合TRUEを返す
 */
static int increase_insults(void)
{
	/* Increase insults */
	st_ptr->insult_cur++;

	/* Become insulted */
	if (st_ptr->insult_cur > ot_ptr->insult_max)
	{
		/* Complain */
		say_comment_4();

		/* Reset insults */
		st_ptr->insult_cur = 0;
		st_ptr->good_buy = 0;
		st_ptr->bad_buy = 0;

		/* Open tomorrow */
		st_ptr->store_open = current_world_ptr->game_turn + TURNS_PER_TICK*TOWN_DAWN/8 + randint1(TURNS_PER_TICK*TOWN_DAWN/8);

		/* Closed */
		return (TRUE);
	}

	/* Not closed */
	return (FALSE);
}


/*!
 * @brief 店主の不満度を減らす /
 * Decrease insults 				-RAK-
 * @return プレイヤーを締め出す場合TRUEを返す
 */
static void decrease_insults(void)
{
	/* Decrease insults */
	if (st_ptr->insult_cur) st_ptr->insult_cur--;
}


/*!
 * @brief 店主の不満度が増えた場合のみのメッセージを表示する /
 * Have insulted while haggling 			-RAK-
 * @return プレイヤーを締め出す場合TRUEを返す
 */
static int haggle_insults(void)
{
	/* Increase insults */
	if (increase_insults()) return (TRUE);

	/* Display and flush insult */
	say_comment_5();

	/* Still okay */
	return (FALSE);
}


/*
 * Mega-Hack -- Enable "increments"
 */
static bool allow_inc = FALSE;

/*
 * Mega-Hack -- Last "increment" during haggling
 */
static s32b last_inc = 0L;


/*!
 * @brief 交渉価格を確認と認証の是非を行う /
 * Get a haggle
 * @param pmt メッセージ
 * @param poffer 別途価格提示をした場合の値を返す参照ポインタ
 * @param price 現在の交渉価格
 * @param final 最終確定価格ならばTRUE
 * @return プレイヤーを締め出す場合TRUEを返す
 */
static int get_haggle(concptr pmt, s32b *poffer, PRICE price, int final)
{
	s32b i;
	concptr p;
	GAME_TEXT buf[128];
	GAME_TEXT out_val[160];

	/* Clear old increment if necessary */
	if (!allow_inc) last_inc = 0L;

	/* Final offer */
	if (final)
	{
		sprintf(buf, _("%s [承諾] ", "%s [accept] "), pmt);
	}

	/* Old (negative) increment, and not final */
	else if (last_inc < 0)
	{
		sprintf(buf, _("%s [-$%ld] ", "%s [-%ld] "), pmt, (long)(ABS(last_inc)));
	}

	/* Old (positive) increment, and not final */
	else if (last_inc > 0)
	{
		sprintf(buf, _("%s [+$%ld] ", "%s [+%ld] "), pmt, (long)(ABS(last_inc)));
	}

	/* Normal haggle */
	else
	{
		sprintf(buf, "%s ", pmt);
	}
	msg_print(NULL);

	/* Ask until done */
	while (TRUE)
	{
		bool res;

		/* Display prompt */
		prt(buf, 0, 0);

		/* Default */
		strcpy(out_val, "");

		/*
		 * Ask the user for a response.
		 * Don't allow to use numpad as cursor key.
		 */
		res = askfor_aux(out_val, 32, FALSE);

		/* Clear prompt */
		prt("", 0, 0);

		/* Cancelled */
		if (!res) return FALSE;

		/* Skip leading spaces */
		for (p = out_val; *p == ' '; p++) /* loop */;

		/* Empty response */
		if (*p == '\0')
		{
			/* Accept current price */
			if (final)
			{
				*poffer = price;
				last_inc = 0L;
				break;
			}

			/* Use previous increment */
			if (allow_inc && last_inc)
			{
				*poffer += last_inc;
				break;
			}
		}

		/* Normal response */
		else
		{
			/* Extract a number */
			i = atol(p);

			/* Handle "incremental" number */
			if ((*p == '+' || *p == '-'))
			{
				/* Allow increments */
				if (allow_inc)
				{
					/* Use the given "increment" */
					*poffer += i;
					last_inc = i;
					break;
				}
			}

			/* Handle normal number */
			else
			{
				/* Use the given "number" */
				*poffer = i;
				last_inc = 0L;
				break;
			}
		}

		/* Warning */
		msg_print(_("値がおかしいです。", "Invalid response."));
		msg_print(NULL);
	}

	/* Success */
	return (TRUE);
}


/*!
 * @brief 店主がプレイヤーからの交渉価格を判断する /
 * Receive an offer (from the player)
 * @param pmt メッセージ
 * @param poffer 店主からの交渉価格を返す参照ポインタ
 * @param last_offer 現在の交渉価格
 * @param factor 店主の価格基準倍率
 * @param price アイテムの実価値
 * @param final 最終価格確定ならばTRUE
 * @return プレイヤーの価格に対して不服ならばTRUEを返す /
 * Return TRUE if offer is NOT okay
 */
static bool receive_offer(concptr pmt, s32b *poffer,
			  s32b last_offer, int factor,
			  PRICE price, int final)
{
	/* Haggle till done */
	while (TRUE)
	{
		/* Get a haggle (or cancel) */
		if (!get_haggle(pmt, poffer, price, final)) return (TRUE);

		/* Acceptable offer */
		if (((*poffer) * factor) >= (last_offer * factor)) break;

		/* Insult, and check for kicked out */
		if (haggle_insults()) return (TRUE);

		/* Reject offer (correctly) */
		(*poffer) = last_offer;
	}

	/* Success */
	return (FALSE);
}


/*!
 * @brief プレイヤーが購入する時の値切り処理メインルーチン /
 * Haggling routine 				-RAK-
 * @param o_ptr オブジェクトの構造体参照ポインタ
 * @param price 最終価格を返す参照ポインタ
 * @return プレイヤーの価格に対して店主が不服ならばTRUEを返す /
 * Return TRUE if purchase is NOT successful
 */
static bool purchase_haggle(object_type *o_ptr, s32b *price)
{
	s32b			   cur_ask, final_ask;
	s32b			   last_offer, offer;
	s32b			   x1, x2, x3;
	s32b			   min_per, max_per;
	int 			   flag, loop_flag, noneed;
	int 			   annoyed = 0, final = FALSE;
	bool		cancel = FALSE;
	char		out_val[160];
	*price = 0;

	concptr pmt = _("提示価格", "Asking");

	/* Extract the starting offer and the final offer */
	cur_ask = price_item(o_ptr, ot_ptr->max_inflate, FALSE);
	final_ask = price_item(o_ptr, ot_ptr->min_inflate, FALSE);

	/* Determine if haggling is necessary */
	noneed = noneedtobargain(final_ask);

	/* No need to haggle */
	if (noneed || !manual_haggle)
	{
		/* No need to haggle */
		if (noneed)
		{
			/* Message summary */
			msg_print(_("結局この金額にまとまった。", "You eventually agree upon the price."));
			msg_print(NULL);
		}

		/* No haggle option */
		else
		{
			/* Message summary */
			msg_print(_("すんなりとこの金額にまとまった。", "You quickly agree upon the price."));
			msg_print(NULL);

			/* Apply Sales Tax */
			final_ask += final_ask / 10;
		}

		/* Final price */
		cur_ask = final_ask;

		/* Go to final offer */
		pmt = _("最終提示価格", "Final Offer");
		final = TRUE;
	}


	/* Haggle for the whole pile */
	cur_ask *= o_ptr->number;
	final_ask *= o_ptr->number;


	/* Haggle parameters */
	min_per = ot_ptr->haggle_per;
	max_per = min_per * 3;

	/* Mega-Hack -- artificial "last offer" value */
	last_offer = object_value(o_ptr) * o_ptr->number;
	last_offer = last_offer * (200 - (int)(ot_ptr->max_inflate)) / 100L;
	if (last_offer <= 0) last_offer = 1;

	/* No offer yet */
	offer = 0;

	/* No incremental haggling yet */
	allow_inc = FALSE;

	/* Haggle until done */
	for (flag = FALSE; !flag; )
	{
		loop_flag = TRUE;

		while (!flag && loop_flag)
		{
			(void)sprintf(out_val, "%s :  %ld", pmt, (long)cur_ask);
			put_str(out_val, 1, 0);
			cancel = receive_offer(_("提示する金額? ", "What do you offer? "), &offer, last_offer, 1, cur_ask, final);
			if (cancel)
			{
				flag = TRUE;
			}
			else if (offer > cur_ask)
			{
				say_comment_6();
				offer = last_offer;
			}
			else if (offer == cur_ask)
			{
				flag = TRUE;
				*price = offer;
			}
			else
			{
				loop_flag = FALSE;
			}
		}

		if (!flag)
		{
			x1 = 100 * (offer - last_offer) / (cur_ask - last_offer);
			if (x1 < min_per)
			{
				if (haggle_insults())
				{
					flag = TRUE;
					cancel = TRUE;
				}
			}
			else if (x1 > max_per)
			{
				x1 = x1 * 3 / 4;
				if (x1 < max_per) x1 = max_per;
			}
			x2 = rand_range(x1-2, x1+2);
			x3 = ((cur_ask - offer) * x2 / 100L) + 1;
			/* don't let the price go up */
			if (x3 < 0) x3 = 0;
			cur_ask -= x3;

			/* Too little */
			if (cur_ask < final_ask)
			{
				final = TRUE;
				cur_ask = final_ask;
				pmt = _("最終提示価格", "What do you offer? ");
				annoyed++;
				if (annoyed > 3)
				{
					(void)(increase_insults());
					cancel = TRUE;
					flag = TRUE;
				}
			}
			else if (offer >= cur_ask)
			{
				flag = TRUE;
				*price = offer;
			}

			if (!flag)
			{
				last_offer = offer;
				allow_inc = TRUE;
				prt("", 1, 0);
				(void)sprintf(out_val, _("前回の提示金額: $%ld", "Your last offer: %ld"), (long)last_offer);
				put_str(out_val, 1, 39);
				say_comment_2(cur_ask, annoyed);
			}
		}
	}

	if (cancel) return (TRUE);

	updatebargain(*price, final_ask, o_ptr->number);

	/* Do not cancel */
	return (FALSE);
}


/*!
 * @brief プレイヤーが売却する時の値切り処理メインルーチン /
 * Haggling routine 				-RAK-
 * @param o_ptr オブジェクトの構造体参照ポインタ
 * @param price 最終価格を返す参照ポインタ
 * @return プレイヤーの価格に対して店主が不服ならばTRUEを返す /
 * Return TRUE if purchase is NOT successful
 */
static bool sell_haggle(object_type *o_ptr, s32b *price)
{
	s32b    purse, cur_ask, final_ask;
	s32b    last_offer = 0, offer = 0;
	s32b    x1, x2, x3;
	s32b    min_per, max_per;
	int     flag, loop_flag, noneed;
	int     annoyed = 0, final = FALSE;
	bool    cancel = FALSE;
	concptr pmt = _("提示金額", "Offer");

	char    out_val[160];

	*price = 0;

	/* Obtain the starting offer and the final offer */
	cur_ask = price_item(o_ptr, ot_ptr->max_inflate, TRUE);
	final_ask = price_item(o_ptr, ot_ptr->min_inflate, TRUE);

	/* Determine if haggling is necessary */
	noneed = noneedtobargain(final_ask);

	/* Get the owner's payout limit */
	purse = (s32b)(ot_ptr->max_cost);

	/* No need to haggle */
	if (noneed || !manual_haggle || (final_ask >= purse))
	{
		/* Apply Sales Tax (if needed) */
		if (!manual_haggle && !noneed)
		{
			final_ask -= final_ask / 10;
		}

		/* No reason to haggle */
		if (final_ask >= purse)
		{
			msg_print(_("即座にこの金額にまとまった。", "You instantly agree upon the price."));
			msg_print(NULL);

			/* Offer full purse */
			final_ask = purse;
		}

		/* No need to haggle */
		else if (noneed)
		{
			msg_print(_("結局この金額にまとまった。", "You eventually agree upon the price."));
			msg_print(NULL);
		}

		/* No haggle option */
		else
		{
			/* Message summary */
			msg_print(_("すんなりとこの金額にまとまった。", "You quickly agree upon the price."));
			msg_print(NULL);
		}

		/* Final price */
		cur_ask = final_ask;

		/* Final offer */
		final = TRUE;
		pmt = _("最終提示金額", "Final Offer");
	}

	/* Haggle for the whole pile */
	cur_ask *= o_ptr->number;
	final_ask *= o_ptr->number;


	/* Display commands */

	/* Haggling parameters */
	min_per = ot_ptr->haggle_per;
	max_per = min_per * 3;

	/* Mega-Hack -- artificial "last offer" value */
	last_offer = object_value(o_ptr) * o_ptr->number;
	last_offer = last_offer * ot_ptr->max_inflate / 100L;

	/* No offer yet */
	offer = 0;

	/* No incremental haggling yet */
	allow_inc = FALSE;

	/* Haggle */
	for (flag = FALSE; !flag; )
	{
		while (1)
		{
			loop_flag = TRUE;

			(void)sprintf(out_val, "%s :  %ld", pmt, (long)cur_ask);
			put_str(out_val, 1, 0);
			cancel = receive_offer(_("提示する価格? ", "What price do you ask? "),
								   &offer, last_offer, -1, cur_ask, final);

			if (cancel)
			{
				flag = TRUE;
			}
			else if (offer < cur_ask)
			{
				say_comment_6();
				/* rejected, reset offer for incremental haggling */
				offer = last_offer;
			}
			else if (offer == cur_ask)
			{
				flag = TRUE;
				*price = offer;
			}
			else
			{
				loop_flag = FALSE;
			}

			/* Stop */
			if (flag || !loop_flag) break;
		}

		if (!flag)
		{
			x1 = 100 * (last_offer - offer) / (last_offer - cur_ask);
			if (x1 < min_per)
			{
				if (haggle_insults())
				{
					flag = TRUE;
					cancel = TRUE;
				}
			}
			else if (x1 > max_per)
			{
				x1 = x1 * 3 / 4;
				if (x1 < max_per) x1 = max_per;
			}
			x2 = rand_range(x1-2, x1+2);
			x3 = ((offer - cur_ask) * x2 / 100L) + 1;
			/* don't let the price go down */
			if (x3 < 0) x3 = 0;
			cur_ask += x3;

			if (cur_ask > final_ask)
			{
				cur_ask = final_ask;
				final = TRUE;
				pmt = _("最終提示金額", "Final Offer");

				annoyed++;
				if (annoyed > 3)
				{
					flag = TRUE;
#ifdef JP
				/* 追加 $0 で買い取られてしまうのを防止 By FIRST*/
					cancel = TRUE;
#endif
					(void)(increase_insults());
				}
			}
			else if (offer <= cur_ask)
			{
				flag = TRUE;
				*price = offer;
			}

			if (!flag)
			{
				last_offer = offer;
				allow_inc = TRUE;
				prt("", 1, 0);
				(void)sprintf(out_val,
					_("前回の提示価格 $%ld", "Your last bid %ld"), (long)last_offer);
				put_str(out_val, 1, 39);
				say_comment_3(cur_ask, annoyed);
			}
		}
	}

	if (cancel) return (TRUE);

	updatebargain(*price, final_ask, o_ptr->number);

	/* Do not cancel */
	return (FALSE);
}


/*!
 * @brief 店からの購入処理のメインルーチン /
 * Buy an item from a store 			-RAK-
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void store_purchase(player_type *player_ptr)
{
	int i, choice;
	COMMAND_CODE item, item_new;

	ITEM_NUMBER amt;

	PRICE price, best;

	object_type forge;
	object_type *j_ptr;

	object_type *o_ptr;

	GAME_TEXT o_name[MAX_NLEN];

	char out_val[160];

	if (cur_store_num == STORE_MUSEUM)
	{
		msg_print(_("博物館から取り出すことはできません。", "Museum."));
		return;
	}

	/* Empty? */
	if (st_ptr->stock_num <= 0)
	{
		if (cur_store_num == STORE_HOME)
			msg_print(_("我が家には何も置いてありません。", "Your home is empty."));
		else
			msg_print(_("現在商品の在庫を切らしています。", "I am currently out of stock."));
		return;
	}

	/* Find the number of objects on this and following pages */
	i = (st_ptr->stock_num - store_top);

	/* And then restrict it to the current page */
	if (i > store_bottom) i = store_bottom;

	/* Prompt */
#ifdef JP
	/* ブラックマーケットの時は別のメッセージ */
	switch( cur_store_num ) {
		case 7:
			sprintf(out_val, "どのアイテムを取りますか? ");
			break;
		case 6:
			sprintf(out_val, "どれ? ");
			break;
		default:
			sprintf(out_val, "どの品物が欲しいんだい? ");
			break;
	}
#else
	if (cur_store_num == STORE_HOME)
	{
		sprintf(out_val, "Which item do you want to take? ");
	}
	else
	{
		sprintf(out_val, "Which item are you interested in? ");
	}
#endif


	/* Get the item number to be bought */
	if (!get_stock(&item, out_val, 0, i - 1)) return;

	/* Get the actual index */
	item = item + store_top;

	/* Get the actual item */
	o_ptr = &st_ptr->stock[item];

	/* Assume the player wants just one of them */
	amt = 1;
	j_ptr = &forge;

	/* Get a copy of the object */
	object_copy(j_ptr, o_ptr);

	/*
	 * If a rod or wand, allocate total maximum timeouts or charges
	 * between those purchased and left on the shelf.
	 */
	reduce_charges(j_ptr, o_ptr->number - amt);

	/* Modify quantity */
	j_ptr->number = amt;

	/* Hack -- require room in pack */
	if (!inven_carry_okay(j_ptr))
	{
		msg_print(_("そんなにアイテムを持てない。", "You cannot carry that many different items."));
		return;
	}

	/* Determine the "best" price (per item) */
	best = price_item(j_ptr, ot_ptr->min_inflate, FALSE);

	/* Find out how many the player wants */
	if (o_ptr->number > 1)
	{
		/* Hack -- note cost of "fixed" items */
		if ((cur_store_num != STORE_HOME) &&
		    (o_ptr->ident & IDENT_FIXED))
		{
			msg_format(_("一つにつき $%ldです。", "That costs %ld gold per item."), (long)(best));
		}
		amt = get_quantity(NULL, o_ptr->number);
		if (amt <= 0) return;
	}
	j_ptr = &forge;

	/* Get desired object */
	object_copy(j_ptr, o_ptr);

	/*
	 * If a rod or wand, allocate total maximum timeouts or charges
	 * between those purchased and left on the shelf.
	 */
	reduce_charges(j_ptr, o_ptr->number - amt);

	/* Modify quantity */
	j_ptr->number = amt;

	/* Hack -- require room in pack */
	if (!inven_carry_okay(j_ptr))
	{
		msg_print(_("ザックにそのアイテムを入れる隙間がない。", "You cannot carry that many items."));
		return;
	}

	/* Attempt to buy it */
	if (cur_store_num != STORE_HOME)
	{
		/* Fixed price, quick buy */
		if (o_ptr->ident & (IDENT_FIXED))
		{
			/* Assume accept */
			choice = 0;

			/* Go directly to the "best" deal */
			price = (best * j_ptr->number);
		}

		/* Haggle for it */
		else
		{
			/* Describe the object (fully) */
			object_desc(o_name, j_ptr, 0);
			msg_format(_("%s(%c)を購入する。", "Buying %s (%c)."), o_name, I2A(item));
			msg_print(NULL);

			/* Haggle for a final price */
			choice = purchase_haggle(j_ptr, &price);

			/* Hack -- Got kicked out */
			if (st_ptr->store_open >= current_world_ptr->game_turn) return;
		}

		/* Player wants it */
		if (choice == 0)
		{
			/* Fix the item price (if "correctly" haggled) */
			if (price == (best * j_ptr->number)) o_ptr->ident |= (IDENT_FIXED);

			/* Player can afford it */
			if (player_ptr->au >= price)
			{
				/* Say "okay" */
				say_comment_1();

				if (cur_store_num == STORE_BLACK) /* The black market is illegal! */
					chg_virtue(player_ptr, V_JUSTICE, -1);
				if((o_ptr->tval == TV_BOTTLE) && (cur_store_num != STORE_HOME))
					chg_virtue(player_ptr, V_NATURE, -1);

				/* Make a sound */
				sound(SOUND_BUY);

				/* Be happy */
				decrease_insults();

				/* Spend the money */
				player_ptr->au -= price;

				/* Update the display */
				store_prt_gold();

				/* Hack -- buying an item makes you aware of it */
				object_aware(player_ptr, j_ptr);

				/* Hack -- clear the "fixed" flag from the item */
				j_ptr->ident &= ~(IDENT_FIXED);

				/* Describe the transaction */
				object_desc(o_name, j_ptr, 0);

				msg_format(_("%sを $%ldで購入しました。", "You bought %s for %ld gold."), o_name, (long)price);

				strcpy(record_o_name, o_name);
				record_turn = current_world_ptr->game_turn;

				if (record_buy) exe_write_diary(player_ptr, NIKKI_BUY, 0, o_name);
				object_desc(o_name, o_ptr, OD_NAME_ONLY);
				if(record_rand_art && o_ptr->art_name)
					exe_write_diary(player_ptr, NIKKI_ART, 0, o_name);

				/* Erase the inscription */
				j_ptr->inscription = 0;

				/* Erase the "feeling" */
				j_ptr->feeling = FEEL_NONE;
				j_ptr->ident &= ~(IDENT_STORE);
				/* Give it to the player */
				item_new = inven_carry(player_ptr, j_ptr);

				/* Describe the final result */
				object_desc(o_name, &player_ptr->inventory_list[item_new], 0);
				msg_format(_("%s(%c)を手に入れた。", "You have %s (%c)."), o_name, index_to_label(item_new));

				/* Auto-inscription */
				autopick_alter_item(item_new, FALSE);

				/* Now, reduce the original stack's pval. */
				if ((o_ptr->tval == TV_ROD) || (o_ptr->tval == TV_WAND))
				{
					o_ptr->pval -= j_ptr->pval;
				}
				handle_stuff();

				/* Note how many slots the store used to have */
				i = st_ptr->stock_num;

				/* Remove the bought items from the store */
				store_item_increase(item, -amt);
				store_item_optimize(item);

				/* Store is empty */
				if (st_ptr->stock_num == 0)
				{
					/* Shuffle */
					if (one_in_(STORE_SHUFFLE))
					{
						char buf[80];
						msg_print(_("店主は引退した。", "The shopkeeper retires."));

						/* Shuffle the store */
						store_shuffle(cur_store_num);

						prt("",3,0);
						sprintf(buf, "%s (%s)",
							ot_ptr->owner_name, race_info[ot_ptr->owner_race].title);
						put_str(buf, 3, 10);
						sprintf(buf, "%s (%ld)",
							(f_name + f_info[cur_store_feat].name), (long)(ot_ptr->max_cost));
						prt(buf, 3, 50);
					}

					/* Maintain */
					else
					{
						msg_print(_("店主は新たな在庫を取り出した。", "The shopkeeper brings out some new stock."));
					}

					for (i = 0; i < 10; i++)
					{
						/* Maintain the store */
						store_maint(player_ptr, player_ptr->town_num, cur_store_num);
					}

					/* Start over */
					store_top = 0;
					display_inventory();
				}

				/* The item is gone */
				else if (st_ptr->stock_num != i)
				{
					/* Pick the correct screen */
					if (store_top >= st_ptr->stock_num) store_top -= store_bottom;
					display_inventory();
				}

				/* Item is still here */
				else
				{
					/* Redraw the item */
					display_entry(item);
				}
			}

			/* Player cannot afford it */
			else
			{
				/* Simple message (no insult) */
				msg_print(_("お金が足りません。", "You do not have enough gold."));
			}
		}
	}

	/* Home is much easier */
	else
	{
		bool combined_or_reordered;

		/* Distribute charges of wands/rods */
		distribute_charges(o_ptr, j_ptr, amt);

		/* Give it to the player */
		item_new = inven_carry(player_ptr, j_ptr);

		/* Describe just the result */
		object_desc(o_name, &player_ptr->inventory_list[item_new], 0);

		msg_format(_("%s(%c)を取った。", "You have %s (%c)."), o_name, index_to_label(item_new));
		handle_stuff();

		/* Take note if we take the last one */
		i = st_ptr->stock_num;

		/* Remove the items from the home */
		store_item_increase(item, -amt);
		store_item_optimize(item);

		combined_or_reordered = combine_and_reorder_home(STORE_HOME);

		/* Hack -- Item is still here */
		if (i == st_ptr->stock_num)
		{
			if (combined_or_reordered) display_inventory();

			/* Redraw the item */
			else display_entry(item);
		}

		/* The item is gone */
		else
		{
			/* Nothing left */
			if (st_ptr->stock_num == 0) store_top = 0;

			/* Nothing left on that screen */
			else if (store_top >= st_ptr->stock_num) store_top -= store_bottom;
			display_inventory();

			chg_virtue(player_ptr, V_SACRIFICE, 1);
		}
	}
}


/*!
 * @brief 店からの売却処理のメインルーチン /
 * Sell an item to the store (or home)
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void store_sell(player_type *owner_ptr)
{
	int choice;
	OBJECT_IDX item;
	int item_pos;
	int amt;

	PRICE price, value, dummy;

	object_type forge;
	object_type *q_ptr;
	object_type *o_ptr;

	concptr q, s;

	GAME_TEXT o_name[MAX_NLEN];


	/* Prepare a prompt */
	if (cur_store_num == STORE_HOME)
		q = _("どのアイテムを置きますか? ", "Drop which item? ");

	else if (cur_store_num == STORE_MUSEUM)
		q = _("どのアイテムを寄贈しますか? ", "Give which item? ");

	else
		q = _("どのアイテムを売りますか? ", "Sell which item? ");

	/* Only allow items the store will buy */
	item_tester_hook = store_will_buy;

	/* 我が家でおかしなメッセージが出るオリジナルのバグを修正 */
	if (cur_store_num == STORE_HOME)
	{
		s = _("置けるアイテムを持っていません。", "You don't have any item to drop.");
	}
	else if (cur_store_num == STORE_MUSEUM)
	{
		s = _("寄贈できるアイテムを持っていません。", "You don't have any item to give.");
	}
	else
	{
		s = _("欲しい物がないですねえ。", "You have nothing that I want.");
	}

	o_ptr = choose_object(owner_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);
	if (!o_ptr) return;

	/* Hack -- Cannot remove cursed items */
	if ((item >= INVEN_RARM) && object_is_cursed(o_ptr))
	{
		msg_print(_("ふーむ、どうやらそれは呪われているようだね。", "Hmmm, it seems to be cursed."));
		return;
	}

	/* Assume one item */
	amt = 1;

	if (o_ptr->number > 1)
	{
		amt = get_quantity(NULL, o_ptr->number);
		if (amt <= 0) return;
	}
	q_ptr = &forge;

	/* Get a copy of the object */
	object_copy(q_ptr, o_ptr);

	/* Modify quantity */
	q_ptr->number = amt;

	/*
	 * Hack -- If a rod or wand, allocate total maximum
	 * timeouts or charges to those being sold. -LM-
	 */
	if ((o_ptr->tval == TV_ROD) || (o_ptr->tval == TV_WAND))
	{
		q_ptr->pval = o_ptr->pval * amt / o_ptr->number;
	}

	/* Get a full description */
	object_desc(o_name, q_ptr, 0);

	/* Remove any inscription, feeling for stores */
	if ((cur_store_num != STORE_HOME) && (cur_store_num != STORE_MUSEUM))
	{
		q_ptr->inscription = 0;
		q_ptr->feeling = FEEL_NONE;
	}

	/* Is there room in the store (or the home?) */
	if (!store_check_num(q_ptr))
	{
		if (cur_store_num == STORE_HOME)
			msg_print(_("我が家にはもう置く場所がない。", "Your home is full."));

		else if (cur_store_num == STORE_MUSEUM)
			msg_print(_("博物館はもう満杯だ。", "Museum is full."));

		else
			msg_print(_("すいませんが、店にはもう置く場所がありません。", "I have not the room in my store to keep it."));

		return;
	}

	/* Real store */
	if ((cur_store_num != STORE_HOME) && (cur_store_num != STORE_MUSEUM))
	{
		/* Describe the transaction */
		msg_format(_("%s(%c)を売却する。", "Selling %s (%c)."), o_name, index_to_label(item));

		msg_print(NULL);

		/* Haggle for it */
		choice = sell_haggle(q_ptr, &price);

		/* Kicked out */
		if (st_ptr->store_open >= current_world_ptr->game_turn) return;

		/* Sold... */
		if (choice == 0)
		{
			/* Say "okay" */
			say_comment_1();

			/* Make a sound */
			sound(SOUND_SELL);

			/* Be happy */
			if (cur_store_num == STORE_BLACK) /* The black market is illegal! */
				chg_virtue(owner_ptr, V_JUSTICE, -1);

			if((o_ptr->tval == TV_BOTTLE) && (cur_store_num != STORE_HOME))
				chg_virtue(owner_ptr, V_NATURE, 1);
			decrease_insults();

			/* Get some money */
			owner_ptr->au += price;

			/* Update the display */
			store_prt_gold();

			/* Get the "apparent" value */
			dummy = object_value(q_ptr) * q_ptr->number;

			identify_item(owner_ptr, o_ptr);
			q_ptr = &forge;

			/* Get a copy of the object */
			object_copy(q_ptr, o_ptr);

			/* Modify quantity */
			q_ptr->number = amt;

			/* Make it look like to be known */
			q_ptr->ident |= IDENT_STORE;

			/*
			 * Hack -- If a rod or wand, let the shopkeeper know just
			 * how many charges he really paid for. -LM-
			 */
			if ((o_ptr->tval == TV_ROD) || (o_ptr->tval == TV_WAND))
			{
				q_ptr->pval = o_ptr->pval * amt / o_ptr->number;
			}

			/* Get the "actual" value */
			value = object_value(q_ptr) * q_ptr->number;

			/* Get the description all over again */
			object_desc(o_name, q_ptr, 0);

			/* Describe the result (in message buffer) */
			msg_format(_("%sを $%ldで売却しました。", "You sold %s for %ld gold."), o_name, (long)price);

			if (record_sell) exe_write_diary(owner_ptr, NIKKI_SELL, 0, o_name);

			if (!((o_ptr->tval == TV_FIGURINE) && (value > 0)))
			{
			 /* Analyze the prices (and comment verbally) unless a figurine*/
			purchase_analyze(price, value, dummy);
			}

			/*
			 * Hack -- Allocate charges between those wands or rods sold
			 * and retained, unless all are being sold. -LM-
			 */
			distribute_charges(o_ptr, q_ptr, amt);

			/* Reset timeouts of the sold items */
			q_ptr->timeout = 0;

			/* Take the item from the player, describe the result */
			inven_item_increase(item, -amt);
			inven_item_describe(owner_ptr, item);

			/* If items remain, auto-inscribe before optimizing */
			if (o_ptr->number > 0)
				autopick_alter_item(item, FALSE);

			inven_item_optimize(item);
			handle_stuff();

			/* The store gets that (known) item */
			item_pos = store_carry(q_ptr);

			/* Re-display if item is now in store */
			if (item_pos >= 0)
			{
				store_top = (item_pos / store_bottom) * store_bottom;
				display_inventory();
			}
		}
	}

	/* Player is at museum */
	else if (cur_store_num == STORE_MUSEUM)
	{
		char o2_name[MAX_NLEN];
		object_desc(o2_name, q_ptr, OD_NAME_ONLY);

		if (-1 == store_check_num(q_ptr))
		{
			msg_print(_("それと同じ品物は既に博物館にあるようです。", "The same object as it is already in the Museum."));
		}
		else
		{
			msg_print(_("博物館に寄贈したものは取り出すことができません！！", "You cannot take items which is given to the Museum back!!"));
		}

		if (!get_check(format(_("本当に%sを寄贈しますか？", "Really give %s to the Museum? "), o2_name))) return;

		identify_item(owner_ptr, q_ptr);
		q_ptr->ident |= IDENT_MENTAL;

		/* Distribute charges of wands/rods */
		distribute_charges(o_ptr, q_ptr, amt);
		msg_format(_("%sを置いた。(%c)", "You drop %s (%c)."), o_name, index_to_label(item));
		choice = 0;

		vary_item(owner_ptr, item, -amt);
		handle_stuff();

		/* Let the home carry it */
		item_pos = home_carry(q_ptr);

		/* Update store display */
		if (item_pos >= 0)
		{
			store_top = (item_pos / store_bottom) * store_bottom;
			display_inventory();
		}
	}
	/* Player is at home */
	else
	{
		/* Distribute charges of wands/rods */
		distribute_charges(o_ptr, q_ptr, amt);
		msg_format(_("%sを置いた。(%c)", "You drop %s (%c)."), o_name, index_to_label(item));

		choice = 0;

		vary_item(owner_ptr, item, -amt);
		handle_stuff();

		/* Let the home carry it */
		item_pos = home_carry(q_ptr);

		/* Update store display */
		if (item_pos >= 0)
		{
			store_top = (item_pos / store_bottom) * store_bottom;
			display_inventory();
		}
	}

	if ((choice == 0) && (item >= INVEN_RARM))
	{
		calc_android_exp(owner_ptr);
		verify_equip_slot(owner_ptr, item);
	}
}


/*!
 * @brief 店のアイテムを調べるコマンドのメインルーチン /
 * Examine an item in a store			   -JDL-
 * @return なし
 */
static void store_examine(void)
{
	int         i;
	COMMAND_CODE item;
	object_type *o_ptr;
	GAME_TEXT o_name[MAX_NLEN];
	char        out_val[160];

	/* Empty? */
	if (st_ptr->stock_num <= 0)
	{
		if (cur_store_num == STORE_HOME)
			msg_print(_("我が家には何も置いてありません。", "Your home is empty."));
		else if (cur_store_num == STORE_MUSEUM)
			msg_print(_("博物館には何も置いてありません。", "Museum is empty."));
		else
			msg_print(_("現在商品の在庫を切らしています。", "I am currently out of stock."));
		return;
	}

	/* Find the number of objects on this and following pages */
	i = (st_ptr->stock_num - store_top);

	/* And then restrict it to the current page */
	if (i > store_bottom) i = store_bottom;

	/* Prompt */
	sprintf(out_val, _("どれを調べますか？", "Which item do you want to examine? "));

	/* Get the item number to be examined */
	if (!get_stock(&item, out_val, 0, i - 1)) return;

	/* Get the actual index */
	item = item + store_top;

	/* Get the actual item */
	o_ptr = &st_ptr->stock[item];

	/* Require full knowledge */
	if (!(o_ptr->ident & IDENT_MENTAL))
	{
		/* This can only happen in the home */
		msg_print(_("このアイテムについて特に知っていることはない。", "You have no special knowledge about that item."));
		return;
	}

	object_desc(o_name, o_ptr, 0);
	msg_format(_("%sを調べている...", "Examining %s..."), o_name);

	if (!screen_object(o_ptr, SCROBJ_FORCE_DETAIL))
		msg_print(_("特に変わったところはないようだ。", "You see nothing special."));

	return;
}


/*!
 * @brief 博物館のアイテムを除去するコマンドのメインルーチン /
 * Remove an item from museum (Originally from TOband)
 * @return なし
 */
static void museum_remove_object(void)
{
	int i;
	COMMAND_CODE item;
	object_type *o_ptr;
	GAME_TEXT o_name[MAX_NLEN];
	char out_val[160];

	/* Empty? */
	if (st_ptr->stock_num <= 0)
	{
		msg_print(_("博物館には何も置いてありません。", "Museum is empty."));
		return;
	}

	/* Find the number of objects on this and following pages */
	i = st_ptr->stock_num - store_top;

	/* And then restrict it to the current page */
	if (i > store_bottom) i = store_bottom;

	/* Prompt */
	sprintf(out_val, _("どのアイテムの展示をやめさせますか？", "Which item do you want to order to remove? "));

	/* Get the item number to be removed */
	if (!get_stock(&item, out_val, 0, i - 1)) return;

	/* Get the actual index */
	item = item + store_top;

	/* Get the actual item */
	o_ptr = &st_ptr->stock[item];

	object_desc(o_name, o_ptr, 0);

	msg_print(_("展示をやめさせたアイテムは二度と見ることはできません！", "You cannot see items which is removed from the Museum!"));
	if (!get_check(format(_("本当に%sの展示をやめさせますか？", "Really order to remove %s from the Museum? "), o_name))) return;

	msg_format(_("%sの展示をやめさせた。", "You ordered to remove %s."), o_name);

	/* Remove the items from the home */
	store_item_increase(item, -o_ptr->number);
	store_item_optimize(item);

	(void)combine_and_reorder_home(STORE_MUSEUM);

	/* The item is gone */

	/* Nothing left */
	if (st_ptr->stock_num == 0) store_top = 0;

	/* Nothing left on that screen */
	else if (store_top >= st_ptr->stock_num) store_top -= store_bottom;
	display_inventory();

	return;
}


/*
 * Hack -- set this to leave the store
 */
static bool leave_store = FALSE;


/*!
 * @brief 店舗処理コマンド選択のメインルーチン /
 * Process a command in a store
 * @param client_ptr 顧客となるクリーチャーの参照ポインタ
 * @return なし
 * @note
 * <pre>
 * Note that we must allow the use of a few "special" commands
 * in the stores which are not allowed in the dungeon, and we
 * must disable some commands which are allowed in the dungeon
 * but not in the stores, to prevent chaos.
 * </pre>
 */
static void store_process_command(player_type *client_ptr)
{
	/* Handle repeating the last command */
	repeat_check();

	if (rogue_like_commands && command_cmd == 'l')
	{
		command_cmd = 'x';	/* hack! */
	}

	/* Parse the command */
	switch (command_cmd)
	{
		/* Leave */
		case ESCAPE:
		{
			leave_store = TRUE;
			break;
		}

		/* 日本語版追加 */
		/* 1 ページ戻るコマンド: 我が家のページ数が多いので重宝するはず By BUG */
		case '-':
		{
			if (st_ptr->stock_num <= store_bottom) {
				msg_print(_("これで全部です。", "Entire client_ptr->inventory_list is shown."));
			}
			else{
				store_top -= store_bottom;
				if ( store_top < 0 )
					store_top = ((st_ptr->stock_num - 1 )/store_bottom) * store_bottom;
				if ( (cur_store_num == STORE_HOME) && (powerup_home == FALSE) )
					if ( store_top >= store_bottom ) store_top = store_bottom;
				display_inventory();
			}
			break;
		}

		/* Browse */
		case ' ':
		{
			if (st_ptr->stock_num <= store_bottom)
			{
				msg_print(_("これで全部です。", "Entire client_ptr->inventory_list is shown."));
			}
			else
			{
				store_top += store_bottom;
				/*
				 * 隠しオプション(powerup_home)がセットされていないときは
				 * 我が家では 2 ページまでしか表示しない
				 */
				if ((cur_store_num == STORE_HOME) && 
				    (powerup_home == FALSE) && 
					(st_ptr->stock_num >= STORE_INVEN_MAX))
				{
					if (store_top >= (STORE_INVEN_MAX - 1))
					{
						store_top = 0;
					}
				}
				else
				{
					if (store_top >= st_ptr->stock_num) store_top = 0;
				}

				display_inventory();
			}
			break;
		}

		case KTRL('R'):
		{
			do_cmd_redraw(client_ptr);
			display_store();
			break;
		}

		/* Get (purchase) */
		case 'g':
		{
			store_purchase(client_ptr);
			break;
		}

		/* Drop (Sell) */
		case 'd':
		{
			store_sell(client_ptr);
			break;
		}

		/* Examine */
		case 'x':
		{
			store_examine();
			break;
		}

		/* Ignore return */
		case '\r':
		{
			break;
		}

		/*** Inventory Commands ***/

		/* Wear/wield equipment */
		case 'w':
		{
			do_cmd_wield(client_ptr);
			break;
		}

		/* Take off equipment */
		case 't':
		{
			do_cmd_takeoff(client_ptr);
			break;
		}

		/* Destroy an item */
		case 'k':
		{
			do_cmd_destroy(client_ptr);
			break;
		}

		/* Equipment list */
		case 'e':
		{
			do_cmd_equip(client_ptr);
			break;
		}

		/* Inventory list */
		case 'i':
		{
			do_cmd_inven(client_ptr);
			break;
		}


		/*** Various commands ***/

		/* Identify an object */
		case 'I':
		{
			do_cmd_observe(client_ptr);
			break;
		}

		case KTRL('I'):
		{
			toggle_inven_equip(client_ptr);
			break;
		}

		/*** Use various objects ***/

		/* Browse a book */
		case 'b':
		{
			if ( (client_ptr->pclass == CLASS_MINDCRAFTER) ||
			     (client_ptr->pclass == CLASS_BERSERKER) ||
			     (client_ptr->pclass == CLASS_NINJA) ||
			     (client_ptr->pclass == CLASS_MIRROR_MASTER) 
			     ) do_cmd_mind_browse(client_ptr);
			else if (client_ptr->pclass == CLASS_SMITH)
				do_cmd_kaji(client_ptr, TRUE);
			else if (client_ptr->pclass == CLASS_MAGIC_EATER)
				do_cmd_magic_eater(client_ptr, TRUE, FALSE);
			else if (client_ptr->pclass == CLASS_SNIPER)
				do_cmd_snipe_browse(client_ptr);
			else do_cmd_browse(client_ptr);
			break;
		}

		/* Inscribe an object */
		case '{':
		{
			do_cmd_inscribe(client_ptr);
			break;
		}

		/* Uninscribe an object */
		case '}':
		{
			do_cmd_uninscribe(client_ptr);
			break;
		}



		/*** Help and Such ***/

		/* Help */
		case '?':
		{
			do_cmd_help();
			break;
		}

		/* Identify symbol */
		case '/':
		{
			do_cmd_query_symbol();
			break;
		}

		/* Character description */
		case 'C':
		{
			client_ptr->town_num = old_town_num;
			do_cmd_player_status(client_ptr);
			client_ptr->town_num = inner_town_num;
			display_store();
			break;
		}


		/*** System Commands ***/

		/* Hack -- User interface */
		case '!':
		{
			(void)Term_user(0);
			break;
		}

		/* Single line from a pref file */
		case '"':
		{
			client_ptr->town_num = old_town_num;
			do_cmd_pref();
			client_ptr->town_num = inner_town_num;
			break;
		}

		/* Interact with macros */
		case '@':
		{
			client_ptr->town_num = old_town_num;
			do_cmd_macros(client_ptr);
			client_ptr->town_num = inner_town_num;
			break;
		}

		/* Interact with visuals */
		case '%':
		{
			client_ptr->town_num = old_town_num;
			do_cmd_visuals(client_ptr);
			client_ptr->town_num = inner_town_num;
			break;
		}

		/* Interact with colors */
		case '&':
		{
			client_ptr->town_num = old_town_num;
			do_cmd_colors(client_ptr);
			client_ptr->town_num = inner_town_num;
			break;
		}

		/* Interact with options */
		case '=':
		{
			do_cmd_options();
			(void)combine_and_reorder_home(STORE_HOME);
			do_cmd_redraw(client_ptr);
			display_store();
			break;
		}

		/*** Misc Commands ***/

		/* Take notes */
		case ':':
		{
			do_cmd_note();
			break;
		}

		/* Version info */
		case 'V':
		{
			do_cmd_version();
			break;
		}

		/* Repeat level feeling */
		case KTRL('F'):
		{
			do_cmd_feeling(client_ptr);
			break;
		}

		/* Show previous message */
		case KTRL('O'):
		{
			do_cmd_message_one();
			break;
		}

		/* Show previous messages */
		case KTRL('P'):
		{
			do_cmd_messages(0);
			break;
		}

		case '|':
		{
			do_cmd_diary(client_ptr);
			break;
		}

		/* Check artifacts, uniques etc. */
		case '~':
		{
			do_cmd_knowledge(client_ptr);
			break;
		}

		/* Load "screen dump" */
		case '(':
		{
			do_cmd_load_screen();
			break;
		}

		/* Save "screen dump" */
		case ')':
		{
			do_cmd_save_screen(client_ptr);
			break;
		}

		/* Hack -- Unknown command */
		default:
		{
			if ((cur_store_num == STORE_MUSEUM) && (command_cmd == 'r'))
			{
				museum_remove_object();
			}
			else
			{
				msg_print(_("そのコマンドは店の中では使えません。", "That command does not work in stores."));
			}
			break;
		}
	}
}


/*!
 * @brief 店舗処理全体のメインルーチン /
 * Enter a store, and interact with it. *
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @note
 * <pre>
 * Note that we use the standard "request_command()" function
 * to get a command, allowing us to use "command_arg" and all
 * command macros and other nifty stuff, but we use the special
 * "shopping" argument, to force certain commands to be converted
 * into other commands, normally, we convert "p" (pray) and "m"
 * (cast magic) into "g" (get), and "s" (search) into "d" (drop).
 * </pre>
 */
void do_cmd_store(player_type *player_ptr)
{
	int which;
	int maintain_num;
	int i;
	grid_type *g_ptr;
	bool need_redraw_store_inv; /* To redraw missiles damage and prices in store */
	TERM_LEN w, h;

	if(player_ptr->wild_mode) return;
	Term_get_size(&w, &h);

	/* Calculate stocks per 1 page */
	xtra_stock = MIN(14+26, ((h > 24) ? (h - 24) : 0));
	store_bottom = MIN_STOCK + xtra_stock;

	/* Access the player grid */
	g_ptr = &player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x];

	/* Verify a store */
	if (!cave_have_flag_grid(g_ptr, FF_STORE))
	{
		msg_print(_("ここには店がありません。", "You see no store here."));
		return;
	}

	/* Extract the store code */
	which = f_info[g_ptr->feat].subtype;

	old_town_num = player_ptr->town_num;
	if ((which == STORE_HOME) || (which == STORE_MUSEUM)) player_ptr->town_num = 1;
	if (player_ptr->current_floor_ptr->dun_level) player_ptr->town_num = NO_TOWN;
	inner_town_num = player_ptr->town_num;

	/* Hack -- Check the "locked doors" */
	if ((town_info[player_ptr->town_num].store[which].store_open >= current_world_ptr->game_turn) ||
	    (ironman_shops))
	{
		msg_print(_("ドアに鍵がかかっている。", "The doors are locked."));
		player_ptr->town_num = old_town_num;
		return;
	}

	/* Calculate the number of store maintainances since the last visit */
	maintain_num = (current_world_ptr->game_turn - town_info[player_ptr->town_num].store[which].last_visit) / (TURNS_PER_TICK * STORE_TICKS);

	/* Maintain the store max. 10 times */
	if (maintain_num > 10) maintain_num = 10;

	if (maintain_num)
	{
		/* Maintain the store */
		for (i = 0; i < maintain_num; i++)
			store_maint(player_ptr, player_ptr->town_num, which);

		/* Save the visit */
		town_info[player_ptr->town_num].store[which].last_visit = current_world_ptr->game_turn;
	}

	forget_lite(player_ptr->current_floor_ptr);
	forget_view(player_ptr->current_floor_ptr);

	/* Hack -- Character is in "icky" mode */
	current_world_ptr->character_icky = TRUE;

	/* command reset */
	command_arg = 0;
	command_rep = 0;
	command_new = 0;

	/* Do not expand macros */
	get_com_no_macros = TRUE;

	/* Save the store number */
	cur_store_num = which;

	/* Hack -- save the store feature */
	cur_store_feat = g_ptr->feat;

	/* Save the store and owner pointers */
	st_ptr = &town_info[player_ptr->town_num].store[cur_store_num];
	ot_ptr = &owners[cur_store_num][st_ptr->owner];

	/* Start at the beginning */
	store_top = 0;

	play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_BUILD);

	display_store();

	/* Do not leave */
	leave_store = FALSE;

	/* Interact with player */
	while (!leave_store)
	{
		/* Hack -- Clear line 1 */
		prt("", 1, 0);
		clear_from(20 + xtra_stock);

		/* Basic commands */
		prt(_(" ESC) 建物から出る", " ESC) Exit from Building."), 21 + xtra_stock, 0);

		/* Browse if necessary */
		if (st_ptr->stock_num > store_bottom)
		{
			prt(_(" -)前ページ", " -) Previous page"), 22 + xtra_stock, 0);
			prt(_(" スペース) 次ページ", " SPACE) Next page"), 23 + xtra_stock, 0);
		}

		/* Home commands */
		if (cur_store_num == STORE_HOME)
		{
			prt(_("g) アイテムを取る", "g) Get an item."), 21 + xtra_stock, 27);
			prt(_("d) アイテムを置く", "d) Drop an item."), 22 + xtra_stock, 27);
			prt(_("x) 家のアイテムを調べる", "x) eXamine an item in the home."), 23 + xtra_stock, 27);
		}

		/* Museum commands */
		else if (cur_store_num == STORE_MUSEUM)
		{
			prt(_("d) アイテムを置く", "d) Drop an item."), 21 + xtra_stock, 27);
			prt(_("r) アイテムの展示をやめる", "r) order to Remove an item."), 22 + xtra_stock, 27);
			prt(_("x) 博物館のアイテムを調べる", "x) eXamine an item in the museum."), 23 + xtra_stock, 27);
		}

		/* Shop commands */
		else
		{
			prt(_("p) 商品を買う", "p) Purchase an item."), 21 + xtra_stock, 30);
			prt(_("s) アイテムを売る", "s) Sell an item."), 22 + xtra_stock, 30);
			prt(_("x) 商品を調べる", "x) eXamine an item in the shop"), 23 + xtra_stock,30);
		}

		/* 基本的なコマンドの追加表示 */
		prt(_("i/e) 持ち物/装備の一覧", "i/e) Inventry/Equipment list"), 21 + xtra_stock, 56);

		if (rogue_like_commands)
		{
			prt(_("w/T) 装備する/はずす", "w/T) Wear/Take off equipment"), 22 + xtra_stock, 56);
		}
		else
		{
			prt(_("w/t) 装備する/はずす", "w/t) Wear/Take off equipment"), 22 + xtra_stock, 56);
		}

		/* Prompt */
		prt(_("コマンド:", "You may: "), 20 + xtra_stock, 0);

		request_command(TRUE);

		/* Process the command */
		store_process_command(player_ptr);

		/*
		 * Hack -- To redraw missiles damage and prices in store
		 * If player's charisma changes, or if player changes a bow, PU_BONUS is set
		 */
		need_redraw_store_inv = (player_ptr->update & PU_BONUS) ? TRUE : FALSE;

		/* Hack -- Character is still in "icky" mode */
		current_world_ptr->character_icky = TRUE;

		handle_stuff();

		/* Pack Overflow */
		if (player_ptr->inventory_list[INVEN_PACK].k_idx)
		{
			INVENTORY_IDX item = INVEN_PACK;

			object_type *o_ptr = &player_ptr->inventory_list[item];

			/* Hack -- Flee from the store */
			if (cur_store_num != STORE_HOME)
			{
				if (cur_store_num == STORE_MUSEUM)
					msg_print(_("ザックからアイテムがあふれそうなので、あわてて博物館から出た...", "Your pack is so full that you flee the Museum..."));
				else
					msg_print(_("ザックからアイテムがあふれそうなので、あわてて店から出た...", "Your pack is so full that you flee the store..."));

				/* Leave */
				leave_store = TRUE;
			}

			/* Hack -- Flee from the home */
			else if (!store_check_num(o_ptr))
			{
				msg_print(_("ザックからアイテムがあふれそうなので、あわてて家から出た...", "Your pack is so full that you flee your home..."));
				/* Leave */
				leave_store = TRUE;
			}

			/* Hack -- Drop items into the home */
			else
			{
				int item_pos;

				object_type forge;
				object_type *q_ptr;

				GAME_TEXT o_name[MAX_NLEN];

				/* Give a message */
				msg_print(_("ザックからアイテムがあふれてしまった！", "Your pack overflows!"));

				q_ptr = &forge;

				object_copy(q_ptr, o_ptr);
				object_desc(o_name, q_ptr, 0);

				msg_format(_("%sが落ちた。(%c)", "You drop %s (%c)."), o_name, index_to_label(item));

				vary_item(player_ptr, item, -255);
				handle_stuff();

				/* Let the home carry it */
				item_pos = home_carry(q_ptr);

				/* Redraw the home */
				if (item_pos >= 0)
				{
					store_top = (item_pos / store_bottom) * store_bottom;
					display_inventory();
				}
			}
		}

		/* Hack -- Redisplay store prices if charisma changes */
		/* Hack -- Redraw missiles damage if player changes bow */
		if (need_redraw_store_inv) display_inventory();

		/* Hack -- get kicked out of the store */
		if (st_ptr->store_open >= current_world_ptr->game_turn) leave_store = TRUE;
	}

	select_floor_music(player_ptr);

	player_ptr->town_num = old_town_num;

	take_turn(player_ptr, 100);

	/* Hack -- Character is no longer in "icky" mode */
	current_world_ptr->character_icky = FALSE;

	/* Hack -- Cancel automatic command */
	command_new = 0;

	/* Hack -- Cancel "see" mode */
	command_see = FALSE;

	/* Allow expanding macros */
	get_com_no_macros = FALSE;

	msg_erase();
	Term_clear();

	/* Update everything */
	player_ptr->update |= (PU_VIEW | PU_LITE | PU_MON_LITE);
	player_ptr->update |= (PU_MONSTERS);

	/* Redraw entire screen */
	player_ptr->redraw |= (PR_BASIC | PR_EXTRA | PR_EQUIPPY);
	player_ptr->redraw |= (PR_MAP);
	player_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
}



/*!
 * @brief 現在の町の店主を交代させる /
 * Shuffle one of the stores.
 * @param which 店舗種類のID
 * @return なし
 */
void store_shuffle(int which)
{
	int i, j;


	/* Ignore home */
	if (which == STORE_HOME) return;
	if (which == STORE_MUSEUM) return;


	/* Save the store index */
	cur_store_num = which;

	/* Activate that store */
	st_ptr = &town_info[p_ptr->town_num].store[cur_store_num];

	j = st_ptr->owner;
	/* Pick a new owner */
	while(1)
	{
		st_ptr->owner = (byte)randint0(MAX_OWNERS);
		if (j == st_ptr->owner) continue;
		for (i = 1;i < max_towns; i++)
		{
			if (i == p_ptr->town_num) continue;
			if (st_ptr->owner == town_info[i].store[cur_store_num].owner) break;
		}
		if (i == max_towns) break;
	}

	/* Activate the new owner */
	ot_ptr = &owners[cur_store_num][st_ptr->owner];


	/* Reset the owner data */
	st_ptr->insult_cur = 0;
	st_ptr->store_open = 0;
	st_ptr->good_buy = 0;
	st_ptr->bad_buy = 0;


	/* Hack -- discount all the items */
	for (i = 0; i < st_ptr->stock_num; i++)
	{
		object_type *o_ptr;

		o_ptr = &st_ptr->stock[i];

		if (!object_is_artifact(o_ptr))
		{
			/* Hack -- Sell all non-artifact old items for "half price" */
			o_ptr->discount = 50;

			/* Hack -- Items are no longer "fixed price" */
			o_ptr->ident &= ~(IDENT_FIXED);

			/* Mega-Hack -- Note that the item is "on sale" */
			o_ptr->inscription = quark_add(_("売出中", "on sale"));
		}
	}
}


/*!
 * @brief 店の品揃えを変化させる /
 * Maintain the inventory at the stores.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param town_num 町のID
 * @param store_num 店舗種類のID
 * @return なし
 */
void store_maint(player_type *player_ptr, int town_num, int store_num)
{
	INVENTORY_IDX j;

	cur_store_num = store_num;

	/* Ignore home */
	if (store_num == STORE_HOME) return;
	if (store_num == STORE_MUSEUM) return;

	/* Activate that store */
	st_ptr = &town_info[town_num].store[store_num];

	/* Activate the owner */
	ot_ptr = &owners[store_num][st_ptr->owner];

	/* Store keeper forgives the player */
	st_ptr->insult_cur = 0;

	/* Mega-Hack -- prune the black market */
	if (store_num == STORE_BLACK)
	{
		/* Destroy crappy black market items */
		for (j = st_ptr->stock_num - 1; j >= 0; j--)
		{
			object_type *o_ptr = &st_ptr->stock[j];

			/* Destroy crappy items */
			if (black_market_crap(o_ptr))
			{
				/* Destroy the item */
				store_item_increase(j, 0 - o_ptr->number);
				store_item_optimize(j);
			}
		}
	}

	/* Choose the number of slots to keep */
	j = st_ptr->stock_num;

	/* Sell a few items */
	j = j - randint1(STORE_TURNOVER);

	/* Never keep more than "STORE_MAX_KEEP" slots */
	if (j > STORE_MAX_KEEP) j = STORE_MAX_KEEP;

	/* Always "keep" at least "STORE_MIN_KEEP" items */
	if (j < STORE_MIN_KEEP) j = STORE_MIN_KEEP;

	/* Hack -- prevent "underflow" */
	if (j < 0) j = 0;

	/* Destroy objects until only "j" slots are left */
	while (st_ptr->stock_num > j) store_delete();


	/* Choose the number of slots to fill */
	j = st_ptr->stock_num;

	/* Buy some more items */
	j = j + randint1(STORE_TURNOVER);

	/* Never keep more than "STORE_MAX_KEEP" slots */
	if (j > STORE_MAX_KEEP) j = STORE_MAX_KEEP;

	/* Always "keep" at least "STORE_MIN_KEEP" items */
	if (j < STORE_MIN_KEEP) j = STORE_MIN_KEEP;

	/* Hack -- prevent "overflow" */
	if (j >= st_ptr->stock_size) j = st_ptr->stock_size - 1;

	/* Acquire some new items */
	while (st_ptr->stock_num < j) store_create(player_ptr);
}


/*!
 * @brief 店舗情報を初期化する /
 * Initialize the stores
 * @param town_num 町のID
 * @param store_num 店舗種類のID
 * @return なし
 */
void store_init(int town_num, int store_num)
{
	int k;
	cur_store_num = store_num;

	/* Activate that store */
	st_ptr = &town_info[town_num].store[store_num];


	/* Pick an owner */
	while(1)
	{
		int i;

		st_ptr->owner = (byte)randint0(MAX_OWNERS);
		for (i = 1;i < max_towns; i++)
		{
			if (i == town_num) continue;
			if (st_ptr->owner == town_info[i].store[store_num].owner) break;
		}
		if (i == max_towns) break;
	}

	/* Activate the new owner */
	ot_ptr = &owners[store_num][st_ptr->owner];


	/* Initialize the store */
	st_ptr->store_open = 0;
	st_ptr->insult_cur = 0;
	st_ptr->good_buy = 0;
	st_ptr->bad_buy = 0;

	/* Nothing in stock */
	st_ptr->stock_num = 0;

	/*
	 * MEGA-HACK - Last visit to store is
	 * BEFORE player birth to enable store restocking
	 */
	st_ptr->last_visit = -10L * TURNS_PER_TICK * STORE_TICKS;

	/* Clear any old items */
	for (k = 0; k < st_ptr->stock_size; k++)
	{
		object_wipe(&st_ptr->stock[k]);
	}
}


/*!
 * @brief アイテムを町のブラックマーケットに移動させる /
 * @param o_ptr 移動させたいオブジェクトの構造体参照ポインタ
 * @return なし
 */
void move_to_black_market(object_type *o_ptr)
{
	/* Not in town */
	if (!p_ptr->town_num) return;

	st_ptr = &town_info[p_ptr->town_num].store[STORE_BLACK];

	o_ptr->ident |= IDENT_STORE;

	(void)store_carry(o_ptr);

	object_wipe(o_ptr); /* Don't leave a bogus object behind... */
}

