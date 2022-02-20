#pragma once

#include <string>
#include <vector>

#include "object-enchant/tr-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trg-types.h"
#include "system/angband.h"
#include "system/system-variables.h"
#include "util/flag-group.h"

enum EGO_TYPE {
    /* Body Armor */
    EGO_A_MORGUL = 4,
    EGO_A_DEMON = 5,
    EGO_DRUID = 6,
    EGO_OLOG = 7,
    EGO_RESISTANCE = 8,
    EGO_ELVENKIND = 9,
    EGO_DWARVEN = 10,
    EGO_PERMANENCE = 11,
    EGO_TWILIGHT = 12,
    EGO_URUKISH = 13,

    /* Shields */
    EGO_ENDURE_ACID = 16,
    EGO_ENDURE_ELEC = 17,
    EGO_ENDURE_FIRE = 18,
    EGO_ENDURE_COLD = 19,
    EGO_ENDURANCE = 20,
    EGO_REFLECTION = 21,
    EGO_NIGHT_DAY = 22,
    EGO_S_PROTECTION = 238,
    EGO_S_DWARVEN = 239,

    /* Crowns and Helms */
    EGO_DARK = 23,
    EGO_BRILLIANCE = 24,
    EGO_H_PROTECTION = 25,
    EGO_H_DEMON = 26,
    EGO_MAGI = 27,
    EGO_MIGHT = 28,
    EGO_LORDLINESS = 29,
    EGO_SEEING = 30,
    EGO_INFRAVISION = 31,
    EGO_LITE = 32,
    EGO_TELEPATHY = 33,
    EGO_REGENERATION = 34,
    EGO_TELEPORTATION = 35,
    EGO_FOOL = 36,
    EGO_BASILISK = 37,
    EGO_ANCIENT_CURSE = 38,
    EGO_SICKLINESS = 39,

    /* Cloaks */
    EGO_PROTECTION = 40,
    EGO_STEALTH = 41,
    EGO_AMAN = 42,
    EGO_AURA_FIRE = 43,
    EGO_ENVELOPING = 44,
    EGO_VULNERABILITY = 45,
    EGO_IRRITATION = 46,
    EGO_AURA_ELEC = 47,
    EGO_AURA_COLD = 128,
    EGO_BAT = 129,
    EGO_NAZGUL = 240,

    /* Gloves */
    EGO_FREE_ACTION = 48,
    EGO_SLAYING = 49,
    EGO_AGILITY = 50,
    EGO_POWER = 51,
    EGO_2WEAPON = 52,
    EGO_MAGIC_MASTERY = 53,
    EGO_WEAKNESS = 54,
    EGO_CLUMSINESS = 55,

    /* Boots */
    EGO_SLOW_DESCENT = 56,
    EGO_QUIET = 57,
    EGO_MOTION = 58,
    EGO_SPEED = 59,
    EGO_JUMP = 60,
    EGO_NOISE = 61,
    EGO_SLOWNESS = 62,
    EGO_ANNOYANCE = 63,

    /* Weapons */
    EGO_HA = 64,
    EGO_DF = 65,
    EGO_BLESS_BLADE = 66,
    EGO_WEST = 68,
    EGO_ATTACKS = 69,
    EGO_SLAYING_WEAPON = 70,
    EGO_FORCE_WEAPON = 71,
    EGO_BRAND_ACID = 72,
    EGO_BRAND_ELEC = 73,
    EGO_BRAND_FIRE = 74,
    EGO_BRAND_COLD = 75,
    EGO_BRAND_POIS = 76,
    EGO_CHAOTIC = 77,
    EGO_SHARPNESS = 78,
    EGO_EARTHQUAKES = 79,
    EGO_W_FAIRLY = 83,
    EGO_W_OMNIVOROUS = 84,
    EGO_W_DARK_REVENGER = 85,
    EGO_KILL_GOOD = 86,
    EGO_WEIRD = 87,
    EGO_KILL_ANIMAL = 88,
    EGO_KILL_EVIL = 89,
    EGO_KILL_UNDEAD = 90,
    EGO_KILL_DEMON = 91,
    EGO_KILL_ORC = 92,
    EGO_KILL_TROLL = 93,
    EGO_KILL_GIANT = 94,
    EGO_KILL_DRAGON = 95,
    EGO_VAMPIRIC = 96,
    EGO_PRISM = 97,
    EGO_TRUMP = 98,
    EGO_PATTERN = 99,
    EGO_DIGGING = 100,
    EGO_DEMON = 101,
    EGO_MORGUL = 102,
    EGO_KILL_HUMAN = 103,

    /* Bows */
    EGO_ACCURACY = 104,
    EGO_VELOCITY = 105,
    EGO_EXTRA_MIGHT = 108,
    EGO_EXTRA_SHOTS = 109,

    /* Ammo */
    EGO_HURT_ANIMAL = 112,
    EGO_HURT_EVIL = 113,
    EGO_HURT_DRAGON = 119,
    EGO_SLAYING_BOLT = 120,
    EGO_LIGHTNING_BOLT = 121,
    EGO_FLAME = 122,
    EGO_FROST = 123,
    EGO_WOUNDING = 124,
    EGO_BACKBITING = 125,
    EGO_SHATTERED = 126,
    EGO_BLASTED = 127,

    /* Lite */
    EGO_LITE_SHINE = 140,
    EGO_LITE_ILLUMINATION = 141,
    EGO_LITE_AURA_FIRE = 142,
    EGO_LITE_INFRA = 143,
    EGO_LITE_LONG = 144,
    EGO_LITE_DARKNESS = 145,
    EGO_LITE_EYE = 146,

    /* Ring */
    EGO_RING_HERO = 150,
    EGO_RING_SLAY = 151,
    EGO_RING_SUPER_AC = 152,
    EGO_RING_MAGIC_MIS = 153,
    EGO_RING_FIRE_BOLT = 154,
    EGO_RING_COLD_BOLT = 155,
    EGO_RING_ELEC_BOLT = 156,
    EGO_RING_ACID_BOLT = 157,
    EGO_RING_MANA_BOLT = 158,
    EGO_RING_FIRE_BALL = 159,
    EGO_RING_COLD_BALL = 160,
    EGO_RING_ELEC_BALL = 161,
    EGO_RING_ACID_BALL = 162,
    EGO_RING_MANA_BALL = 163,
    EGO_RING_DRAGON_F = 164,
    EGO_RING_DRAGON_C = 165,
    EGO_RING_D_SPEED = 166,
    EGO_RING_BERSERKER = 167,
    EGO_RING_HUNTER = 168,
    EGO_RING_THROW = 169,
    EGO_RING_REGEN = 170,
    EGO_RING_LITE = 171,
    EGO_RING_M_DETECT = 172,
    EGO_RING_STEALTH = 173,
    EGO_RING_TELE_AWAY = 174,
    EGO_RING_TO_H = 175,
    EGO_RING_TO_D = 176,
    EGO_RING_RES_LITE = 177,
    EGO_RING_RES_DARK = 178,
    EGO_RING_WIZARD = 179,
    EGO_RING_TRUE = 180,
    EGO_RING_DRAIN_EXP = 181,
    EGO_RING_NO_MELEE = 182,
    EGO_RING_AGGRAVATE = 183,
    EGO_RING_TY_CURSE = 184,
    EGO_RING_RES_TIME = 185,
    EGO_RING_TELEPORT = 186,
    EGO_RING_ALBINO = 187,

    /* Amulet */
    EGO_AMU_SLOW_D = 210,
    EGO_AMU_INFRA = 211,
    EGO_AMU_SEE_INVIS = 212,
    EGO_AMU_HOLD_EXP = 213,
    EGO_AMU_DRAIN_EXP = 214,
    EGO_AMU_FOOL = 215,
    EGO_AMU_AGGRAVATE = 216,
    EGO_AMU_TY_CURSE = 217,
    EGO_AMU_AC = 218,
    EGO_AMU_IDENT = 219,
    EGO_AMU_CHARM = 220,
    EGO_AMU_STEALTH = 221,
    EGO_AMU_JUMP = 222,
    EGO_AMU_TELEPORT = 223,
    EGO_AMU_D_DOOR = 224,
    EGO_AMU_DEFENDER = 225,
    EGO_AMU_RES_FIRE = 226,
    EGO_AMU_RES_FIRE_ = 227,
    EGO_AMU_RES_COLD = 228,
    EGO_AMU_RES_COLD_ = 229,
    EGO_AMU_RES_ELEC = 230,
    EGO_AMU_RES_ELEC_ = 231,
    EGO_AMU_RES_ACID = 232,
    EGO_AMU_RES_ACID_ = 233,
    EGO_AMU_LEVITATION = 234,
    EGO_AMU_GREAT = 235,
    EGO_AMU_DETECTION = 236,
    EGO_AMU_NAIVETY = 237,
    // MAX 240,

};


struct ego_generate_type {
    int mul{}; //<! 確率分子
    int dev{}; //<! 確率分母
    std::vector<tr_type> tr_flags{};
    std::vector<ItemGenerationTraitType> trg_flags{};
};

/*
 * Information about "ego-items".
 */
enum class RandomArtActType : short;
struct ego_item_type {
    EGO_IDX idx{};

    std::string name; //!< エゴの名前
    std::string text; //!< フレーバーテキスト

    INVENTORY_IDX slot{}; //!< 装備部位 / Standard slot value
    PRICE rating{}; //!< レーティングボーナス(雰囲気に影響) / Rating boost

    DEPTH level{}; //!< 生成レベル
    RARITY rarity{}; //<! レアリティ

    HIT_PROB base_to_h{}; //!< ベース命中修正
    int base_to_d{}; //!< べ^スダメージ修正
    ARMOUR_CLASS base_to_a{}; //!< ベースAC修正

    HIT_PROB max_to_h{}; //!< 最大ボーナス命中修正
    int max_to_d{}; //!< 最大ボーナスダメージ修正
    ARMOUR_CLASS max_to_a{}; //!< 最大ボーナスAC修正

    PARAMETER_VALUE max_pval{}; //!< 最大pval

    PRICE cost{}; //!< コスト

    TrFlags flags{}; //!< 能力/耐性フラグ
    EnumClassFlagGroup<ItemGenerationTraitType> gen_flags; //!< 生成時適用フラグ
    std::vector<ego_generate_type> xtra_flags{}; //!< 追加能力/耐性フラグ

    RandomArtActType act_idx{}; //!< 発動番号 / Activative ability index
};

extern std::vector<ego_item_type> e_info;

class ObjectType;
class PlayerType;
byte get_random_ego(byte slot, bool good);
void apply_ego(ObjectType *o_ptr, DEPTH lev);
