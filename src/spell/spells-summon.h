#pragma once

#include "system/angband.h"

/*
 * summon_specificで取り扱われる、召喚の種別定義 / Legal restrictions for "summon_specific()"
 */
#define SUMMON_ANT                  11 /*!< 召喚タイプ: アリ */
#define SUMMON_SPIDER               12 /*!< 召喚タイプ: 蜘蛛 */
#define SUMMON_HOUND                13 /*!< 召喚タイプ: ハウンド */
#define SUMMON_HYDRA                14 /*!< 召喚タイプ: ヒドラ */
#define SUMMON_ANGEL                15 /*!< 召喚タイプ: 天使 */
#define SUMMON_DEMON                16 /*!< 召喚タイプ: 悪魔 */
#define SUMMON_UNDEAD               17 /*!< 召喚タイプ: アンデッド */
#define SUMMON_DRAGON               18 /*!< 召喚タイプ: ドラゴン */
#define SUMMON_HI_UNDEAD            21 /*!< 召喚タイプ: 強力なアンデッド */
#define SUMMON_HI_DRAGON            22 /*!< 召喚タイプ: 古代ドラゴン */
#define SUMMON_HI_DEMON             23 /*!< 召喚タイプ: 上級デーモン */
#define SUMMON_AMBERITES            31 /*!< 召喚タイプ: アンバーの王族 */
#define SUMMON_UNIQUE               32 /*!< 召喚タイプ: ユニーク */
#define SUMMON_MOLD                 33 /*!< 召喚タイプ: カビ */
#define SUMMON_BAT                  34 /*!< 召喚タイプ: コウモリ */
#define SUMMON_QUYLTHULG            35 /*!< 召喚タイプ: クイルスルグ */
#define SUMMON_XXX1                 36 /*!< 召喚タイプ: 未使用 */
#define SUMMON_COIN_MIMIC           37 /*!< 召喚タイプ: クリーピング・コイン */
#define SUMMON_MIMIC                38 /*!< 召喚タイプ: ミミック */
#define SUMMON_CYBER                39 /*!< 召喚タイプ: サイバーデーモン */
#define SUMMON_KIN                  40 /*!< 召喚タイプ: 召喚者の同族 */
#define SUMMON_DAWN                 41 /*!< 召喚タイプ: 暁の戦士 */
#define SUMMON_ANIMAL               42 /*!< 召喚タイプ: 自然界の動物 */
#define SUMMON_ANIMAL_RANGER        43 /*!< 召喚タイプ: レンジャー向け自然界の動物 */
 /*#define SUMMON_HI_UNDEAD_NO_UNIQUES 44*/
 /*#define SUMMON_HI_DRAGON_NO_UNIQUES 45*/
 /*#define SUMMON_NO_UNIQUES           46*/
#define SUMMON_PHANTOM              47 /*!< 召喚タイプ: ゴースト */
/*#define SUMMON_ELEMENTAL_NO_UNIQUES 48*/
#define SUMMON_BLUE_HORROR          49 /*!< 召喚タイプ: ブルー・ホラー */
#define SUMMON_LIVING               50 /*!< 召喚タイプ: 生命のあるモンスター */
#define SUMMON_HI_DRAGON_LIVING     51 /*!< 召喚タイプ: 生命のある古代ドラゴン */
#define SUMMON_GOLEM                52 /*!< 召喚タイプ: ゴーレム */
#define SUMMON_ELEMENTAL            53 /*!< 召喚タイプ: エレメンタル */
#define SUMMON_VORTEX               54 /*!< 召喚タイプ: ボルテックス */
#define SUMMON_HYBRID               55 /*!< 召喚タイプ: 混合生物 */
#define SUMMON_BIRD                 56 /*!< 召喚タイプ: 鳥 */
/*#define SUMMON_AQUATIC_NO_UNIQUES   57*/
#define SUMMON_KAMIKAZE             58 /*!< 召喚タイプ: 自爆モンスター */
#define SUMMON_KAMIKAZE_LIVING      59 /*!< 召喚タイプ: 生命のある自爆モンスター */
#define SUMMON_MANES                60 /*!< 召喚タイプ: 古代の死霊 */
#define SUMMON_LOUSE                61 /*!< 召喚タイプ: シラミ */
#define SUMMON_GUARDIANS            62 /*!< 召喚タイプ: ダンジョンの主 */
#define SUMMON_KNIGHTS              63 /*!< 召喚タイプ: 聖戦用騎士系モンスター */
#define SUMMON_EAGLES               64 /*!< 召喚タイプ: 鷲系モンスター */
#define SUMMON_PIRANHAS             65 /*!< 召喚タイプ: ピラニア・トラップ用 */
#define SUMMON_ARMAGE_GOOD          66 /*!< 召喚タイプ: ハルマゲドン・トラップ用天使陣営 */
#define SUMMON_ARMAGE_EVIL          67 /*!< 召喚タイプ: ハルマゲドン・トラップ用悪魔陣営 */

bool trump_summoning(player_type *caster_ptr, int num, bool pet, POSITION y, POSITION x, DEPTH lev, int type, BIT_FLAGS mode);
bool cast_summon_demon(player_type *creature_ptr, int power);
bool cast_summon_undead(player_type *creature_ptr, int power);
bool cast_summon_hound(player_type *creature_ptr, int power);
bool cast_summon_elemental(player_type *creature_ptr, int power);
bool cast_summon_octopus(player_type *creature_ptr);
bool item_tester_offer(player_type *creature_ptr, object_type *o_ptr);
bool cast_summon_greater_demon(player_type *caster_ptr);
bool summon_kin_player(player_type *creature_ptr, DEPTH level, POSITION y, POSITION x, BIT_FLAGS mode);
void mitokohmon(player_type *kohmon_ptr);
int summon_cyber(player_type *creature_ptr, MONSTER_IDX who, POSITION y, POSITION x);
int activate_hi_summon(player_type *caster_ptr, POSITION y, POSITION x, bool can_pet);
void cast_invoke_spirits(player_type *caster_ptr, DIRECTION dir);
