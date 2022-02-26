#pragma once

#include <map>
#include <string>
#include <vector>

#include "object-enchant/object-ego.h"
#include "object-enchant/tr-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trg-types.h"
#include "system/angband.h"
#include "util/flag-group.h"

enum class EgoType {
    NONE = 0,

    /* Body Armor */
    A_MORGUL = 4,
    A_DEMON = 5,
    DRUID = 6,
    OLOG = 7,
    RESISTANCE = 8,
    ELVENKIND = 9,
    DWARVEN = 10,
    PERMANENCE = 11,
    TWILIGHT = 12,
    URUKISH = 13,

    /* Shields */
    ENDURE_ACID = 16,
    ENDURE_ELEC = 17,
    ENDURE_FIRE = 18,
    ENDURE_COLD = 19,
    ENDURANCE = 20,
    REFLECTION = 21,
    NIGHT_DAY = 22,
    S_PROTECTION = 238,
    S_DWARVEN = 239,

    /* Crowns and Helms */
    DARK = 23,
    BRILLIANCE = 24,
    H_PROTECTION = 25,
    H_DEMON = 26,
    MAGI = 27,
    MIGHT = 28,
    LORDLINESS = 29,
    SEEING = 30,
    INFRAVISION = 31,
    LITE = 32,
    TELEPATHY = 33,
    REGENERATION = 34,
    TELEPORTATION = 35,
    FOOL = 36,
    BASILISK = 37,
    ANCIENT_CURSE = 38,
    SICKLINESS = 39,

    /* Cloaks */
    PROTECTION = 40,
    STEALTH = 41,
    AMAN = 42,
    AURA_FIRE = 43,
    ENVELOPING = 44,
    VULNERABILITY = 45,
    IRRITATION = 46,
    AURA_ELEC = 47,
    AURA_COLD = 128,
    BAT = 129,
    NAZGUL = 240,

    /* Gloves */
    FREE_ACTION = 48,
    SLAYING = 49,
    AGILITY = 50,
    POWER = 51,
    TWO_WEAPON = 52,
    MAGIC_MASTERY = 53,
    WEAKNESS = 54,
    CLUMSINESS = 55,

    /* Boots */
    SLOW_DESCENT = 56,
    QUIET = 57,
    MOTION = 58,
    SPEED = 59,
    JUMP = 60,
    NOISE = 61,
    SLOWNESS = 62,
    ANNOYANCE = 63,

    /* Weapons */
    HA = 64,
    DF = 65,
    BLESS_BLADE = 66,
    WEST = 68,
    ATTACKS = 69,
    SLAYING_WEAPON = 70,
    FORCE_WEAPON = 71,
    BRAND_ACID = 72,
    BRAND_ELEC = 73,
    BRAND_FIRE = 74,
    BRAND_COLD = 75,
    BRAND_POIS = 76,
    CHAOTIC = 77,
    SHARPNESS = 78,
    EARTHQUAKES = 79,
    W_FAIRLY = 83,
    W_OMNIVOROUS = 84,
    W_DARK_REVENGER = 85,
    KILL_GOOD = 86,
    WEIRD = 87,
    KILL_ANIMAL = 88,
    KILL_EVIL = 89,
    KILL_UNDEAD = 90,
    KILL_DEMON = 91,
    KILL_ORC = 92,
    KILL_TROLL = 93,
    KILL_GIANT = 94,
    KILL_DRAGON = 95,
    VAMPIRIC = 96,
    PRISM = 97,
    TRUMP = 98,
    PATTERN = 99,
    DIGGING = 100,
    DEMON = 101,
    MORGUL = 102,
    KILL_HUMAN = 103,

    /* Bows */
    ACCURACY = 104,
    VELOCITY = 105,
    EXTRA_MIGHT = 108,
    EXTRA_SHOTS = 109,

    /* Ammo */
    HURT_ANIMAL = 112,
    HURT_EVIL = 113,
    HURT_DRAGON = 119,
    SLAYING_BOLT = 120,
    LIGHTNING_BOLT = 121,
    FLAME = 122,
    FROST = 123,
    WOUNDING = 124,
    BACKBITING = 125,
    SHATTERED = 126,
    BLASTED = 127,

    /* Lite */
    LITE_SHINE = 140,
    LITE_ILLUMINATION = 141,
    LITE_AURA_FIRE = 142,
    LITE_INFRA = 143,
    LITE_LONG = 144,
    LITE_DARKNESS = 145,
    LITE_EYE = 146,

    /* Ring */
    RING_HERO = 150,
    RING_SLAY = 151,
    RING_SUPER_AC = 152,
    RING_MAGIC_MIS = 153,
    RING_FIRE_BOLT = 154,
    RING_COLD_BOLT = 155,
    RING_ELEC_BOLT = 156,
    RING_ACID_BOLT = 157,
    RING_MANA_BOLT = 158,
    RING_FIRE_BALL = 159,
    RING_COLD_BALL = 160,
    RING_ELEC_BALL = 161,
    RING_ACID_BALL = 162,
    RING_MANA_BALL = 163,
    RING_DRAGON_F = 164,
    RING_DRAGON_C = 165,
    RING_D_SPEED = 166,
    RING_BERSERKER = 167,
    RING_HUNTER = 168,
    RING_THROW = 169,
    RING_REGEN = 170,
    RING_LITE = 171,
    RING_M_DETECT = 172,
    RING_STEALTH = 173,
    RING_TELE_AWAY = 174,
    RING_TO_H = 175,
    RING_TO_D = 176,
    RING_RES_LITE = 177,
    RING_RES_DARK = 178,
    RING_WIZARD = 179,
    RING_TRUE = 180,
    RING_DRAIN_EXP = 181,
    RING_NO_MELEE = 182,
    RING_AGGRAVATE = 183,
    RING_TY_CURSE = 184,
    RING_RES_TIME = 185,
    RING_TELEPORT = 186,
    RING_ALBINO = 187,

    /* Amulet */
    AMU_SLOW_D = 210,
    AMU_INFRA = 211,
    AMU_SEE_INVIS = 212,
    AMU_HOLD_EXP = 213,
    AMU_DRAIN_EXP = 214,
    AMU_FOOL = 215,
    AMU_AGGRAVATE = 216,
    AMU_TY_CURSE = 217,
    AMU_AC = 218,
    AMU_IDENT = 219,
    AMU_CHARM = 220,
    AMU_STEALTH = 221,
    AMU_JUMP = 222,
    AMU_TELEPORT = 223,
    AMU_D_DOOR = 224,
    AMU_DEFENDER = 225,
    AMU_RES_FIRE = 226,
    AMU_RES_FIRE_ = 227,
    AMU_RES_COLD = 228,
    AMU_RES_COLD_ = 229,
    AMU_RES_ELEC = 230,
    AMU_RES_ELEC_ = 231,
    AMU_RES_ACID = 232,
    AMU_RES_ACID_ = 233,
    AMU_LEVITATION = 234,
    AMU_GREAT = 235,
    AMU_DETECTION = 236,
    AMU_NAIVETY = 237,
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
    EgoType idx{};

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

extern std::map<EgoType, ego_item_type> e_info;

class ObjectType;
class PlayerType;
EgoType get_random_ego(byte slot, bool good);
void apply_ego(ObjectType *o_ptr, DEPTH lev);
