#pragma once

#include "monster-attack/monster-attack-effect.h"
#include "system/angband.h"
#include "effect/attribute-types.h"

/*!
 * @note モンスターの打撃方法 / New monster blow methods
 * 打撃の種別に応じて傷と朦朧が発生するかがコメントの通りに決まる
 *
 * "Race Blow Method" の略。
 * 実装の都合上、0 から始まる連番でなければならない。
 */
enum rbm_type {
    RBM_NONE = 0,
    RBM_HIT = 1, /*!< モンスターの攻撃種別:殴る(傷/朦朧が半々) */
    RBM_TOUCH = 2, /*!< モンスターの攻撃種別:触る */
    RBM_PUNCH = 3, /*!< モンスターの攻撃種別:パンチする(朦朧) */
    RBM_KICK = 4, /*!< モンスターの攻撃種別:蹴る(朦朧) */
    RBM_CLAW = 5, /*!< モンスターの攻撃種別:ひっかく(傷) */
    RBM_BITE = 6, /*!< モンスターの攻撃種別:噛む(傷) */
    RBM_STING = 7, /*!< モンスターの攻撃種別:刺す */
    RBM_SLASH = 8, /*!< モンスターの攻撃種別:斬る(傷) */
    RBM_BUTT = 9, /*!< モンスターの攻撃種別:角で突く(朦朧) */
    RBM_CRUSH = 10, /*!< モンスターの攻撃種別:体当たりする(朦朧) */
    RBM_ENGULF = 11, /*!< モンスターの攻撃種別:飲み込む */
    RBM_CHARGE = 12, /*!< モンスターの攻撃種別:請求書を寄越す */
    RBM_CRAWL = 13, /*!< モンスターの攻撃種別:体の上を這い回る */
    RBM_DROOL = 14, /*!< モンスターの攻撃種別:よだれをたらす */
    RBM_SPIT = 15, /*!< モンスターの攻撃種別:つばを吐く */
    RBM_EXPLODE = 16, /*!< モンスターの攻撃種別:爆発する */
    RBM_GAZE = 17, /*!< モンスターの攻撃種別:にらむ */
    RBM_WAIL = 18, /*!< モンスターの攻撃種別:泣き叫ぶ */
    RBM_SPORE = 19, /*!< モンスターの攻撃種別:胞子を飛ばす */
    RBM_XXX4 = 20, /*!< モンスターの攻撃種別:未定義 */
    RBM_BEG = 21, /*!< モンスターの攻撃種別:金をせがむ */
    RBM_INSULT = 22, /*!< モンスターの攻撃種別:侮辱する */
    RBM_MOAN = 23, /*!< モンスターの攻撃種別:うめく */
    RBM_SHOW = 24, /*!< モンスターの攻撃種別:歌う */
    RBM_SHOOT = 25, /*!< モンスターの攻撃種別:射撃(非打撃) */

    NB_RBM_TYPE, /*!< enum バリアント数 */
};

typedef struct mbe_info_type {
    int power; /* The attack "power" */
    AttributeType explode_type; /* Explosion effect */
} mbe_info_type;

extern const mbe_info_type mbe_info[static_cast<int>(RaceBlowEffectType::MAX)];
