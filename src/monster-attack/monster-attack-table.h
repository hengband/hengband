#pragma once

#include "effect/attribute-types.h"
#include "monster-attack/monster-attack-effect.h"
#include "system/angband.h"

/*!
 * @note モンスターの打撃方法 / New monster blow methods
 * 打撃の種別に応じて傷と朦朧が発生するかがコメントの通りに決まる
 *
 * "Race Blow Method" の略。
 * 実装の都合上、0 から始まる連番でなければならない。
 */
enum class RaceBlowMethodType {
    NONE = 0,
    HIT = 1, /*!< モンスターの攻撃種別:殴る(傷/朦朧が半々) */
    TOUCH = 2, /*!< モンスターの攻撃種別:触る */
    PUNCH = 3, /*!< モンスターの攻撃種別:パンチする(朦朧) */
    KICK = 4, /*!< モンスターの攻撃種別:蹴る(朦朧) */
    CLAW = 5, /*!< モンスターの攻撃種別:ひっかく(傷) */
    BITE = 6, /*!< モンスターの攻撃種別:噛む(傷) */
    STING = 7, /*!< モンスターの攻撃種別:刺す */
    SLASH = 8, /*!< モンスターの攻撃種別:斬る(傷) */
    BUTT = 9, /*!< モンスターの攻撃種別:角で突く(朦朧) */
    CRUSH = 10, /*!< モンスターの攻撃種別:体当たりする(朦朧) */
    ENGULF = 11, /*!< モンスターの攻撃種別:飲み込む */
    CHARGE = 12, /*!< モンスターの攻撃種別:請求書を寄越す */
    CRAWL = 13, /*!< モンスターの攻撃種別:体の上を這い回る */
    DROOL = 14, /*!< モンスターの攻撃種別:よだれをたらす */
    SPIT = 15, /*!< モンスターの攻撃種別:つばを吐く */
    EXPLODE = 16, /*!< モンスターの攻撃種別:爆発する */
    GAZE = 17, /*!< モンスターの攻撃種別:にらむ */
    WAIL = 18, /*!< モンスターの攻撃種別:泣き叫ぶ */
    SPORE = 19, /*!< モンスターの攻撃種別:胞子を飛ばす */
    XXX4 = 20, /*!< モンスターの攻撃種別:未定義 */
    BEG = 21, /*!< モンスターの攻撃種別:金をせがむ */
    INSULT = 22, /*!< モンスターの攻撃種別:侮辱する */
    MOAN = 23, /*!< モンスターの攻撃種別:うめく */
    SHOW = 24, /*!< モンスターの攻撃種別:歌う */
    SHOOT = 25, /*!< モンスターの攻撃種別:射撃(非打撃) */

    MAX, /*!< enum バリアント数 */
};

struct mbe_info_type {
    int power; /* The attack "power" */
    AttributeType explode_type; /* Explosion effect */
};

extern const mbe_info_type mbe_info[static_cast<int>(RaceBlowEffectType::MAX)];
