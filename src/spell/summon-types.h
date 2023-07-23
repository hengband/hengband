#pragma once

/* summon_specificで取り扱われる、召喚の種別定義 / Legal restrictions for "summon_specific()" */
enum summon_type : int {
    SUMMON_NONE = 0,
    SUMMON_ANT = 11, /*!< 召喚タイプ: アリ */
    SUMMON_SPIDER = 12, /*!< 召喚タイプ: 蜘蛛 */
    SUMMON_HOUND = 13, /*!< 召喚タイプ: ハウンド */
    SUMMON_HYDRA = 14, /*!< 召喚タイプ: ヒドラ */
    SUMMON_ANGEL = 15, /*!< 召喚タイプ: 天使 */
    SUMMON_DEMON = 16, /*!< 召喚タイプ: 悪魔 */
    SUMMON_UNDEAD = 17, /*!< 召喚タイプ: アンデッド */
    SUMMON_DRAGON = 18, /*!< 召喚タイプ: ドラゴン */
    SUMMON_HI_UNDEAD = 21, /*!< 召喚タイプ: 強力なアンデッド */
    SUMMON_HI_DRAGON = 22, /*!< 召喚タイプ: 古代ドラゴン */
    SUMMON_HI_DEMON = 23, /*!< 召喚タイプ: 上級デーモン */
    SUMMON_AMBERITES = 31, /*!< 召喚タイプ: アンバーの王族 */
    SUMMON_UNIQUE = 32, /*!< 召喚タイプ: ユニーク */
    SUMMON_MOLD = 33, /*!< 召喚タイプ: カビ */
    SUMMON_BAT = 34, /*!< 召喚タイプ: コウモリ */
    SUMMON_QUYLTHULG = 35, /*!< 召喚タイプ: クイルスルグ */
    SUMMON_XXX1 = 36, /*!< 召喚タイプ: 未使用 */
    SUMMON_COIN_MIMIC = 37, /*!< 召喚タイプ: クリーピング・コイン */
    SUMMON_MIMIC = 38, /*!< 召喚タイプ: ミミック */
    SUMMON_CYBER = 39, /*!< 召喚タイプ: サイバーデーモン */
    SUMMON_KIN = 40, /*!< 召喚タイプ: 召喚者の同族 */
    SUMMON_DAWN = 41, /*!< 召喚タイプ: 暁の戦士 */
    SUMMON_ANIMAL = 42, /*!< 召喚タイプ: 自然界の動物 */
    SUMMON_ANIMAL_RANGER = 43, /*!< 召喚タイプ: レンジャー向け自然界の動物 */
    SUMMON_SMALL_MOAI = 44, /*!< 召喚タイプ: プチモアイ */
    SUMMON_PYRAMID = 45, /*!< 召喚タイプ: ピラミッド */
    SUMMON_PHANTOM = 47, /*!< 召喚タイプ: ゴースト */
    SUMMON_TOTEM_MOAI = 48, /*!< 召喚タイプ: トーテムモアイ */
    SUMMON_BLUE_HORROR = 49, /*!< 召喚タイプ: ブルー・ホラー */
    SUMMON_LIVING = 50, /*!< 召喚タイプ: 生命のあるモンスター */
    SUMMON_HI_DRAGON_LIVING = 51, /*!< 召喚タイプ: 生命のある古代ドラゴン */
    SUMMON_GOLEM = 52, /*!< 召喚タイプ: ゴーレム */
    SUMMON_ELEMENTAL = 53, /*!< 召喚タイプ: エレメンタル */
    SUMMON_VORTEX = 54, /*!< 召喚タイプ: ボルテックス */
    SUMMON_HYBRID = 55, /*!< 召喚タイプ: 混合生物 */
    SUMMON_BIRD = 56, /*!< 召喚タイプ: 鳥 */
    SUMMON_KAMIKAZE = 58, /*!< 召喚タイプ: 自爆モンスター */
    SUMMON_KAMIKAZE_LIVING = 59, /*!< 召喚タイプ: 生命のある自爆モンスター */
    SUMMON_MANES = 60, /*!< 召喚タイプ: 古代の死霊 */
    SUMMON_LOUSE = 61, /*!< 召喚タイプ: シラミ */
    SUMMON_GUARDIANS = 62, /*!< 召喚タイプ: ダンジョンの主 */
    SUMMON_KNIGHTS = 63, /*!< 召喚タイプ: 聖戦用騎士系モンスター */
    SUMMON_EAGLES = 64, /*!< 召喚タイプ: 鷲系モンスター */
    SUMMON_PIRANHAS = 65, /*!< 召喚タイプ: ピラニア・トラップ用 */
    SUMMON_ARMAGE_GOOD = 66, /*!< 召喚タイプ: ハルマゲドン・トラップ用天使陣営 */
    SUMMON_ARMAGE_EVIL = 67, /*!< 召喚タイプ: ハルマゲドン・トラップ用悪魔陣営 */
    SUMMON_APOCRYPHA_FOLLOWERS = 68, /*!< 召喚タイプ: 信者 */
    SUMMON_APOCRYPHA_DRAGONS = 69, /*!< 召喚タイプ: 強力な古代ドラゴン */
    SUMMON_VESPOID = 70, /*!< 召喚タイプ: ランゴスタ */
    SUMMON_ANTI_TIGERS = 71, /*!< 召喚タイプ: トラ以外 */
    SUMMON_DEAD_UNIQUE = 72, /*!< 召喚タイプ: 撃破済みユニーク */
};
