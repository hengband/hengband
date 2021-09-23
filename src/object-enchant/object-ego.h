#pragma once

#include <string>
#include <vector>

#include "object-enchant/tr-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trg-types.h"
#include "system/angband.h"
#include "system/system-variables.h"
#include "util/flag-group.h"

/* Body Armor */
#define EGO_A_MORGUL            4
#define EGO_A_DEMON             5
#define EGO_DRUID               6
#define EGO_OLOG                7
#define EGO_RESISTANCE          8
#define EGO_ELVENKIND           9
#define EGO_DWARVEN             10
#define EGO_PERMANENCE          11
#define EGO_TWILIGHT             12
#define EGO_URUKISH             13

/* Shields */
#define EGO_ENDURE_ACID         16
#define EGO_ENDURE_ELEC         17
#define EGO_ENDURE_FIRE         18
#define EGO_ENDURE_COLD         19
#define EGO_ENDURANCE           20
#define EGO_REFLECTION          21
#define EGO_NIGHT_DAY           22
#define EGO_S_PROTECTION        238
#define EGO_S_DWARVEN           239

/* Crowns and Helms */
#define EGO_DARK                23
#define EGO_BRILLIANCE          24
#define EGO_H_PROTECTION        25
#define EGO_H_DEMON             26
#define EGO_MAGI                27
#define EGO_MIGHT               28
#define EGO_LORDLINESS          29
#define EGO_SEEING              30
#define EGO_INFRAVISION         31
#define EGO_LITE                32
#define EGO_TELEPATHY           33
#define EGO_REGENERATION        34
#define EGO_TELEPORTATION       35
#define EGO_FOOL                36
#define EGO_BASILISK            37
#define EGO_ANCIENT_CURSE       38
#define EGO_SICKLINESS          39

/* Cloaks */
#define EGO_PROTECTION          40
#define EGO_STEALTH             41
#define EGO_AMAN                42
#define EGO_AURA_FIRE           43
#define EGO_ENVELOPING          44
#define EGO_VULNERABILITY       45
#define EGO_IRRITATION          46
#define EGO_AURA_ELEC           47
#define EGO_AURA_COLD          128
#define EGO_BAT                129
#define EGO_NAZGUL             240

/* Gloves */
#define EGO_FREE_ACTION         48
#define EGO_SLAYING             49
#define EGO_AGILITY             50
#define EGO_POWER               51
#define EGO_2WEAPON             52
#define EGO_MAGIC_MASTERY       53
#define EGO_WEAKNESS            54
#define EGO_CLUMSINESS          55

/* Boots */
#define EGO_SLOW_DESCENT        56
#define EGO_QUIET               57
#define EGO_MOTION              58
#define EGO_SPEED               59
#define EGO_JUMP                60
#define EGO_NOISE               61
#define EGO_SLOWNESS            62
#define EGO_ANNOYANCE           63

/* Weapons */
#define EGO_HA                  64
#define EGO_DF                  65
#define EGO_BLESS_BLADE         66
#define EGO_WEST                68
#define EGO_ATTACKS             69
#define EGO_SLAYING_WEAPON      70
#define EGO_FORCE_WEAPON        71
#define EGO_BRAND_ACID          72
#define EGO_BRAND_ELEC          73
#define EGO_BRAND_FIRE          74
#define EGO_BRAND_COLD          75
#define EGO_BRAND_POIS          76
#define EGO_CHAOTIC             77
#define EGO_SHARPNESS           78
#define EGO_EARTHQUAKES         79
#define EGO_W_FAIRLY            83
#define EGO_W_OMNIVOROUS        84
#define EGO_W_DARK_REVENGER     85
#define EGO_KILL_GOOD           86
#define EGO_WEIRD               87
#define EGO_KILL_ANIMAL         88
#define EGO_KILL_EVIL           89
#define EGO_KILL_UNDEAD         90
#define EGO_KILL_DEMON          91
#define EGO_KILL_ORC            92
#define EGO_KILL_TROLL          93
#define EGO_KILL_GIANT          94
#define EGO_KILL_DRAGON         95
#define EGO_VAMPIRIC            96
#define EGO_PRISM               97
#define EGO_TRUMP               98
#define EGO_PATTERN             99
#define EGO_DIGGING             100
#define EGO_DEMON               101
#define EGO_MORGUL              102
#define EGO_KILL_HUMAN          103

/* Bows */
#define EGO_ACCURACY            104
#define EGO_VELOCITY            105
#define EGO_EXTRA_MIGHT         108
#define EGO_EXTRA_SHOTS         109

/* Ammo */
#define EGO_HURT_ANIMAL         112
#define EGO_HURT_EVIL           113
#define EGO_HURT_DRAGON         119
#define EGO_SLAYING_BOLT        120
#define EGO_LIGHTNING_BOLT      121
#define EGO_FLAME               122
#define EGO_FROST               123
#define EGO_WOUNDING            124
#define EGO_BACKBITING          125
#define EGO_SHATTERED           126
#define EGO_BLASTED             127

/* Lite */
#define EGO_LITE_SHINE          140
#define EGO_LITE_ILLUMINATION   141
#define EGO_LITE_AURA_FIRE      142
#define EGO_LITE_INFRA          143
#define EGO_LITE_LONG           144
#define EGO_LITE_DARKNESS       145
#define EGO_LITE_EYE            146

/* Ring */
#define EGO_RING_HERO           150
#define EGO_RING_SLAY           151
#define EGO_RING_SUPER_AC       152
#define EGO_RING_MAGIC_MIS      153
#define EGO_RING_FIRE_BOLT      154
#define EGO_RING_COLD_BOLT      155
#define EGO_RING_ELEC_BOLT      156
#define EGO_RING_ACID_BOLT      157
#define EGO_RING_MANA_BOLT      158
#define EGO_RING_FIRE_BALL      159
#define EGO_RING_COLD_BALL      160
#define EGO_RING_ELEC_BALL      161
#define EGO_RING_ACID_BALL      162
#define EGO_RING_MANA_BALL      163
#define EGO_RING_DRAGON_F       164
#define EGO_RING_DRAGON_C       165
#define EGO_RING_D_SPEED        166
#define EGO_RING_BERSERKER      167
#define EGO_RING_HUNTER         168
#define EGO_RING_THROW          169
#define EGO_RING_REGEN          170
#define EGO_RING_LITE           171
#define EGO_RING_M_DETECT       172
#define EGO_RING_STEALTH        173
#define EGO_RING_TELE_AWAY      174
#define EGO_RING_TO_H           175
#define EGO_RING_TO_D           176
#define EGO_RING_RES_LITE       177
#define EGO_RING_RES_DARK       178
#define EGO_RING_WIZARD         179
#define EGO_RING_TRUE           180
#define EGO_RING_DRAIN_EXP      181
#define EGO_RING_NO_MELEE       182
#define EGO_RING_AGGRAVATE      183
#define EGO_RING_TY_CURSE       184
#define EGO_RING_RES_TIME       185
#define EGO_RING_TELEPORT       186
#define EGO_RING_ALBINO         187

/* Amulet */
#define EGO_AMU_SLOW_D          210
#define EGO_AMU_INFRA           211
#define EGO_AMU_SEE_INVIS       212
#define EGO_AMU_HOLD_EXP        213
#define EGO_AMU_DRAIN_EXP       214
#define EGO_AMU_FOOL            215
#define EGO_AMU_AGGRAVATE       216
#define EGO_AMU_TY_CURSE        217
#define EGO_AMU_AC              218
#define EGO_AMU_IDENT           219
#define EGO_AMU_CHARM           220
#define EGO_AMU_STEALTH         221
#define EGO_AMU_JUMP            222
#define EGO_AMU_TELEPORT        223
#define EGO_AMU_D_DOOR          224
#define EGO_AMU_DEFENDER        225
#define EGO_AMU_RES_FIRE        226
#define EGO_AMU_RES_FIRE_       227
#define EGO_AMU_RES_COLD        228
#define EGO_AMU_RES_COLD_       229
#define EGO_AMU_RES_ELEC        230
#define EGO_AMU_RES_ELEC_       231
#define EGO_AMU_RES_ACID        232
#define EGO_AMU_RES_ACID_       233
#define EGO_AMU_LEVITATION      234
#define EGO_AMU_GREAT           235
#define EGO_AMU_DETECTION       236
#define EGO_AMU_NAIVETY         237
// MAX 240

struct ego_generate_type {
    int mul{}; //<! 確率分子
    int dev{}; //<! 確率分母
    std::vector<tr_type> tr_flags{};
    std::vector<TRG> trg_flags{};
};

/*
 * Information about "ego-items".
 */
struct ego_item_type {
    EGO_IDX idx{};

    std::string name; //!< エゴの名前
    std::string text; //!< フレーバーテキスト

    INVENTORY_IDX slot{}; //!< 装備部位 / Standard slot value
    PRICE rating{}; //!< レーティングボーナス(雰囲気に影響) / Rating boost

    DEPTH level{}; //!< 生成レベル
    RARITY rarity{}; //<! レアリティ

    HIT_PROB base_to_h{}; //!< ベース命中修正
    HIT_POINT base_to_d{}; //!< べ^スダメージ修正
    ARMOUR_CLASS base_to_a{}; //!< ベースAC修正

    HIT_PROB max_to_h{}; //!< 最大ボーナス命中修正
    HIT_POINT max_to_d{}; //!< 最大ボーナスダメージ修正
    ARMOUR_CLASS max_to_a{}; //!< 最大ボーナスAC修正

    PARAMETER_VALUE max_pval{}; //!< 最大pval

    PRICE cost{}; //!< コスト

    TrFlags flags{}; //!< 能力/耐性フラグ
    EnumClassFlagGroup<TRG> gen_flags; //!< 生成時適用フラグ
    std::vector<ego_generate_type> xtra_flags{}; //!< 追加能力/耐性フラグ

    IDX act_idx{}; //!< 発動番号 / Activative ability index
};

extern std::vector<ego_item_type> e_info;

struct object_type;;
struct player_type;
byte get_random_ego(byte slot, bool good);
void apply_ego(object_type *o_ptr, DEPTH lev);
