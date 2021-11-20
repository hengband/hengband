#pragma once

#include "system/angband.h"

#include <array>

/*!
 * @todo TRが何の略か分かる人、補足求む
 */
enum tr_type : int32_t {
    TR_STR = 0, /* STR += "pval" */
    TR_INT = 1, /* INT += "pval" */
    TR_WIS = 2, /* WIS += "pval" */
    TR_DEX = 3, /* DEX += "pval" */
    TR_CON = 4, /* CON += "pval" */
    TR_CHR = 5, /* CHR += "pval" */
    TR_MAGIC_MASTERY = 6, /* 魔道具使用能力向上 */
    TR_FORCE_WEAPON = 7, /* Later */
    TR_STEALTH = 8, /* Stealth += "pval" */
    TR_SEARCH = 9, /* Search += "pval" */
    TR_INFRA = 10, /* Infra += "pval" */
    TR_TUNNEL = 11, /* Tunnel += "pval" */
    TR_SPEED = 12, /* Speed += "pval" */
    TR_BLOWS = 13, /* Blows += "pval" */
    TR_CHAOTIC = 14,
    TR_VAMPIRIC = 15,
    TR_SLAY_ANIMAL = 16,
    TR_SLAY_EVIL = 17,
    TR_SLAY_UNDEAD = 18,
    TR_SLAY_DEMON = 19,
    TR_SLAY_ORC = 20,
    TR_SLAY_TROLL = 21,
    TR_SLAY_GIANT = 22,
    TR_SLAY_DRAGON = 23,
    TR_KILL_DRAGON = 24, /* Execute Dragon */
    TR_VORPAL = 25, /* Later */
    TR_EARTHQUAKE = 26, //!< 地震を起こす / Cause earthquake
    TR_BRAND_POIS = 27,
    TR_BRAND_ACID = 28,
    TR_BRAND_ELEC = 29,
    TR_BRAND_FIRE = 30,
    TR_BRAND_COLD = 31,

    TR_SUST_STR = 32,
    TR_SUST_INT = 33,
    TR_SUST_WIS = 34,
    TR_SUST_DEX = 35,
    TR_SUST_CON = 36,
    TR_SUST_CHR = 37,
    TR_RIDING = 38,
    TR_EASY_SPELL = 39, /* 呪文失敗率減少 */
    TR_IM_ACID = 40,
    TR_IM_ELEC = 41,
    TR_IM_FIRE = 42,
    TR_IM_COLD = 43,
    TR_THROW = 44, /* 強力投擲ではなく、投げやすい武器 */
    TR_REFLECT = 45, /* Reflect 'bolts' */
    TR_FREE_ACT = 46, /* 耐麻痺 */
    TR_HOLD_EXP = 47, /* 経験値維持 */
    TR_RES_ACID = 48,
    TR_RES_ELEC = 49,
    TR_RES_FIRE = 50,
    TR_RES_COLD = 51,
    TR_RES_POIS = 52,
    TR_RES_FEAR = 53, /* Added for Zangband */
    TR_RES_LITE = 54,
    TR_RES_DARK = 55,
    TR_RES_BLIND = 56,
    TR_RES_CONF = 57,
    TR_RES_SOUND = 58,
    TR_RES_SHARDS = 59,
    TR_RES_NETHER = 60,
    TR_RES_NEXUS = 61,
    TR_RES_CHAOS = 62,
    TR_RES_DISEN = 63,

    TR_SH_FIRE = 64, /* Immolation (Fire) */
    TR_SH_ELEC = 65, /* Electric Sheath */
    TR_SLAY_HUMAN = 66, /* Slay human */
    TR_SH_COLD = 67, /* cold aura */
    TR_NO_TELE = 68, /* 反テレポート */
    TR_NO_MAGIC = 69, /* 反魔法 */
    TR_DEC_MANA = 70, /* 消費魔力減少 */
    TR_TY_CURSE = 71, /* The Ancient Curse */
    TR_WARNING = 72, /* Warning */
    TR_HIDE_TYPE = 73, /* Hide "pval" description */
    TR_SHOW_MODS = 74, /* Always show Tohit/Todam */
    TR_SLAY_GOOD = 75, //!< 善良スレイ(/善)
    TR_LEVITATION = 76, /* Feather Falling */
    TR_LITE_1 = 77, /* Light Radius 1*/
    TR_SEE_INVIS = 78, /* See Invisible */
    TR_TELEPATHY = 79, /* Telepathy */
    TR_SLOW_DIGEST = 80, /* Item slows down digestion */
    TR_REGEN = 81, /* Item induces regeneration */
    TR_XTRA_MIGHT = 82, /* Bows get extra multiplier */
    TR_XTRA_SHOTS = 83, /* Bows get extra shots */
    TR_IGNORE_ACID = 84, /* Item ignores Acid Damage */
    TR_IGNORE_ELEC = 85, /* Item ignores Elec Damage */
    TR_IGNORE_FIRE = 86, /* Item ignores Fire Damage */
    TR_IGNORE_COLD = 87, /* Item ignores Cold Damage */
    TR_ACTIVATE = 88, /* Item can be activated */
    TR_DRAIN_EXP = 89, /* Item drains Experience */
    TR_TELEPORT = 90, /* Item teleports player */
    TR_AGGRAVATE = 91, /* Item aggravates monsters */
    TR_BLESSED = 92, /* Item is Blessed */
    TR_XXX_93 = 93, //!< 未使用 / Unused
    TR_XXX_94 = 94, //!< 未使用 / Unused
    TR_KILL_GOOD = 95, //!< 善良スレイ(X善)

    TR_KILL_ANIMAL = 96,
    TR_KILL_EVIL = 97,
    TR_KILL_UNDEAD = 98,
    TR_KILL_DEMON = 99,
    TR_KILL_ORC = 100,
    TR_KILL_TROLL = 101,
    TR_KILL_GIANT = 102,
    TR_KILL_HUMAN = 103,
    TR_ESP_ANIMAL = 104,
    TR_ESP_UNDEAD = 105,
    TR_ESP_DEMON = 106,
    TR_ESP_ORC = 107,
    TR_ESP_TROLL = 108,
    TR_ESP_GIANT = 109,
    TR_ESP_DRAGON = 110,
    TR_ESP_HUMAN = 111,
    TR_ESP_EVIL = 112,
    TR_ESP_GOOD = 113,
    TR_ESP_NONLIVING = 114,
    TR_ESP_UNIQUE = 115,
    TR_FULL_NAME = 116,
    TR_FIXED_FLAVOR = 117,
    TR_ADD_L_CURSE = 118,
    TR_ADD_H_CURSE = 119,
    TR_DRAIN_HP = 120,
    TR_DRAIN_MANA = 121,
    TR_LITE_2 = 122,
    TR_LITE_3 = 123,
    TR_LITE_M1 = 124, /* Permanent decrease Light Area (-1) */
    TR_LITE_M2 = 125, /* Permanent decrease Light Area (-1) */
    TR_LITE_M3 = 126, /* Permanent decrease Light Area (-1) */
    TR_LITE_FUEL = 127, /* Lights need Fuels */

    TR_CALL_ANIMAL = 128,
    TR_CALL_DEMON = 129,
    TR_CALL_DRAGON = 130,
    TR_CALL_UNDEAD = 131,
    TR_COWARDICE = 132,
    TR_LOW_MELEE = 133,
    TR_LOW_AC = 134,
    TR_HARD_SPELL = 135,
    TR_FAST_DIGEST = 136,
    TR_SLOW_REGEN = 137,
    TR_MIGHTY_THROW = 138,
    TR_EASY2_WEAPON = 139,
    TR_DOWN_SAVING = 140,
    TR_NO_AC = 141,
    TR_HEAVY_SPELL = 142,
    TR_RES_TIME = 143,
    TR_RES_WATER = 144,
    TR_INVULN_ARROW = 145,
    TR_DARK_SOURCE = 146,
    TR_SUPPORTIVE = 147,
    TR_RES_CURSE = 148,
    TR_BERS_RAGE = 149, //!< 狂戦士化の発作
    TR_BRAND_MAGIC = 150, //!< 魔術属性
    TR_IMPACT = 151, //!< クリティカル率アップ / Increase critical hit ratio
    TR_VUL_ACID = 152, //!< 酸弱点
    TR_VUL_COLD = 153, //!< 冷気弱点
    TR_VUL_ELEC = 154, //!< 電撃弱点
    TR_VUL_FIRE = 155, //!< 火炎弱点
    TR_VUL_LITE = 156, //!< 閃光弱点
    TR_IM_DARK = 157, //!< 暗黒免疫

    TR_SELF_FIRE = 158, //!< マイナスフラグ - 持続火炎ダメージ
    TR_SELF_ELEC = 159, //!< マイナスフラグ - 持続電撃ダメージ
    TR_SELF_COLD = 160, //!< マイナスフラグ - 持続冷気ダメージ

    TR_PERSITENT_CURSE = 161, //!< 頻繁に自身を呪いなおすフラグ
    TR_VUL_CURSE = 162, //!< 呪力弱点

    TR_FLAG_MAX = 163,
};

/** 能力値(STR,INT,WIS,DEX,CON,CHR)のpvalを増減させるフラグのリスト */
inline constexpr std::array<tr_type, 6> TR_STATUS_LIST = {
    TR_STR,
    TR_INT,
    TR_WIS,
    TR_DEX,
    TR_CON,
    TR_CHR,
};

/** 能力値(STR,INT,WIS,DEX,CON,CHR)を維持するフラグのリスト */
inline constexpr std::array<tr_type, 6> TR_SUST_STATUS_LIST = {
    TR_SUST_STR,
    TR_SUST_INT,
    TR_SUST_WIS,
    TR_SUST_DEX,
    TR_SUST_CON,
    TR_SUST_CHR,
};
