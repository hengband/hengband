#include "store/articles-on-sale.h"
#include "object/tval-types.h"
#include "store/store-owners.h"
#include "sv-definition/sv-amulet-types.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-bow-types.h"
#include "sv-definition/sv-digging-types.h"
#include "sv-definition/sv-food-types.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-potion-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-ring-types.h"
#include "sv-definition/sv-rod-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "sv-definition/sv-staff-types.h"
#include "sv-definition/sv-wand-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/baseitem-info.h"

/*!
 * @brief 店舗で常時販売するオブジェクトを定義する
 * @details
 * 上から優先して配置する。
 * 重複して同じ商品を設定した場合、数量が増える。
 * 17エントリーまで設定可能。
 * 種類が多すぎる場合、店舗を埋めつくすので注意。
 */
const std::map<StoreSaleType, std::vector<BaseitemKey>> store_regular_sale_table = {
    { StoreSaleType::GENERAL,
        {
            { ItemKindType::FOOD, SV_FOOD_RATION },
            { ItemKindType::LITE, SV_LITE_TORCH },
            { ItemKindType::LITE, SV_LITE_LANTERN },
            { ItemKindType::FLASK, SV_FLASK_OIL },
            { ItemKindType::POTION, SV_POTION_WATER },
            { ItemKindType::SPIKE, 0 },
        } },
    { StoreSaleType::ARMOURY,
        {} },
    { StoreSaleType::WEAPON,
        {
            { ItemKindType::HISSATSU_BOOK, 0 },
            { ItemKindType::SHOT, SV_AMMO_NORMAL },
            { ItemKindType::SHOT, SV_AMMO_NORMAL },
            { ItemKindType::ARROW, SV_AMMO_NORMAL },
            { ItemKindType::ARROW, SV_AMMO_NORMAL },
            { ItemKindType::BOLT, SV_AMMO_NORMAL },
            { ItemKindType::BOLT, SV_AMMO_NORMAL },
        } },
    { StoreSaleType::TEMPLE,
        {
            { ItemKindType::POTION, SV_POTION_CURE_CRITICAL },
            { ItemKindType::POTION, SV_POTION_CURE_SERIOUS },
            { ItemKindType::POTION, SV_POTION_HEROISM },
            { ItemKindType::SCROLL, SV_SCROLL_WORD_OF_RECALL },
            { ItemKindType::SCROLL, SV_SCROLL_REMOVE_CURSE },
            { ItemKindType::LIFE_BOOK, 0 },
            { ItemKindType::CRUSADE_BOOK, 0 },
        } },
    { StoreSaleType::ALCHEMIST,
        {
            { ItemKindType::SCROLL, SV_SCROLL_PHASE_DOOR },
            { ItemKindType::SCROLL, SV_SCROLL_PHASE_DOOR },
            { ItemKindType::SCROLL, SV_SCROLL_TELEPORT },
            { ItemKindType::SCROLL, SV_SCROLL_TELEPORT },
            { ItemKindType::SCROLL, SV_SCROLL_RECHARGING },
            { ItemKindType::SCROLL, SV_SCROLL_IDENTIFY },
            { ItemKindType::SCROLL, SV_SCROLL_DETECT_GOLD },
        } },
    { StoreSaleType::MAGIC,
        {
            { ItemKindType::STAFF, SV_STAFF_IDENTIFY },
            { ItemKindType::STAFF, SV_STAFF_IDENTIFY },
            { ItemKindType::STAFF, SV_STAFF_MAPPING },
            { ItemKindType::ARCANE_BOOK, 0 },
            { ItemKindType::SORCERY_BOOK, 0 },
        } },
    { StoreSaleType::BLACK,
        {} },
    { StoreSaleType::HOME,
        {} },
    { StoreSaleType::BOOK,
        {
            { ItemKindType::SORCERY_BOOK, 0 },
            { ItemKindType::NATURE_BOOK, 0 },
            { ItemKindType::CHAOS_BOOK, 0 },
            { ItemKindType::DEATH_BOOK, 0 },
            { ItemKindType::TRUMP_BOOK, 0 },
            { ItemKindType::ARCANE_BOOK, 0 },
            { ItemKindType::CRAFT_BOOK, 0 },
            { ItemKindType::DEMON_BOOK, 0 },
            { ItemKindType::MUSIC_BOOK, 0 },
            { ItemKindType::HEX_BOOK, 0 },
        } },
    { StoreSaleType::MUSEUM,
        {} },
};

/*!
 * @brief 店舗でランダム販売するオブジェクトを定義する
 * @details tval/svalのペア
 */
const std::map<StoreSaleType, std::vector<BaseitemKey>> store_sale_table = {
    { StoreSaleType::GENERAL,
        {
            /* General Store */
            { ItemKindType::FOOD, SV_FOOD_RATION },
            { ItemKindType::FOOD, SV_FOOD_RATION },
            { ItemKindType::FOOD, SV_FOOD_RATION },
            { ItemKindType::FOOD, SV_FOOD_RATION },

            { ItemKindType::FOOD, SV_FOOD_BISCUIT },
            { ItemKindType::FOOD, SV_FOOD_JERKY },
            { ItemKindType::FOOD, SV_FOOD_PINT_OF_WINE },
            { ItemKindType::FOOD, SV_FOOD_PINT_OF_ALE },

            { ItemKindType::LITE, SV_LITE_TORCH },
            { ItemKindType::LITE, SV_LITE_TORCH },
            { ItemKindType::LITE, SV_LITE_LANTERN },
            { ItemKindType::LITE, SV_LITE_LANTERN },

            { ItemKindType::POTION, SV_POTION_WATER },
            { ItemKindType::POTION, SV_POTION_WATER },
            { ItemKindType::FOOD, SV_FOOD_WAYBREAD },
            { ItemKindType::FOOD, SV_FOOD_WAYBREAD },

            { ItemKindType::FLASK, 0 },
            { ItemKindType::FLASK, 0 },
            { ItemKindType::SPIKE, 0 },
            { ItemKindType::SPIKE, 0 },

            { ItemKindType::SPIKE, 0 },
            { ItemKindType::SPIKE, 0 },
            { ItemKindType::SHOT, SV_AMMO_NORMAL },
            { ItemKindType::SHOT, SV_AMMO_NORMAL },

            { ItemKindType::ARROW, SV_AMMO_NORMAL },
            { ItemKindType::ARROW, SV_AMMO_NORMAL },
            { ItemKindType::BOLT, SV_AMMO_NORMAL },
            { ItemKindType::BOLT, SV_AMMO_NORMAL },

            { ItemKindType::DIGGING, SV_SHOVEL },
            { ItemKindType::DIGGING, SV_SHOVEL },
            { ItemKindType::DIGGING, SV_PICK },
            { ItemKindType::CLOAK, SV_CLOAK },

            { ItemKindType::CLOAK, SV_CLOAK },
            { ItemKindType::CLOAK, SV_FUR_CLOAK },
            { ItemKindType::CAPTURE, 0 },
            { ItemKindType::CAPTURE, 0 },

            { ItemKindType::FIGURINE, 0 },
            { ItemKindType::WHISTLE, 1 },
            { ItemKindType::ROD, SV_ROD_PESTICIDE },
            { ItemKindType::STATUE },
        } },

    {
        StoreSaleType::ARMOURY,
        { { ItemKindType::BOOTS, SV_PAIR_OF_SOFT_LEATHER_BOOTS },
            { ItemKindType::BOOTS, SV_PAIR_OF_SOFT_LEATHER_BOOTS },
            { ItemKindType::BOOTS, SV_PAIR_OF_HARD_LEATHER_BOOTS },
            { ItemKindType::BOOTS, SV_PAIR_OF_HARD_LEATHER_BOOTS },

            { ItemKindType::HELM, SV_HARD_LEATHER_CAP },
            { ItemKindType::HELM, SV_HARD_LEATHER_CAP },
            { ItemKindType::HELM, SV_METAL_CAP },
            { ItemKindType::HELM, SV_IRON_HELM },

            { ItemKindType::SOFT_ARMOR, SV_ROBE },
            { ItemKindType::SOFT_ARMOR, SV_ROBE },
            { ItemKindType::SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
            { ItemKindType::SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },

            { ItemKindType::SOFT_ARMOR, SV_HARD_LEATHER_ARMOR },
            { ItemKindType::SOFT_ARMOR, SV_HARD_LEATHER_ARMOR },
            { ItemKindType::SOFT_ARMOR, SV_HARD_STUDDED_LEATHER },
            { ItemKindType::SOFT_ARMOR, SV_HARD_STUDDED_LEATHER },

            { ItemKindType::SOFT_ARMOR, SV_RHINO_HIDE_ARMOR },
            { ItemKindType::SOFT_ARMOR, SV_LEATHER_SCALE_MAIL },
            { ItemKindType::HARD_ARMOR, SV_METAL_SCALE_MAIL },
            { ItemKindType::HARD_ARMOR, SV_CHAIN_MAIL },

            { ItemKindType::HARD_ARMOR, SV_DOUBLE_RING_MAIL },
            { ItemKindType::HARD_ARMOR, SV_AUGMENTED_CHAIN_MAIL },
            { ItemKindType::HARD_ARMOR, SV_BAR_CHAIN_MAIL },
            { ItemKindType::HARD_ARMOR, SV_DOUBLE_CHAIN_MAIL },

            { ItemKindType::HARD_ARMOR, SV_METAL_BRIGANDINE_ARMOUR },
            { ItemKindType::HARD_ARMOR, SV_SPLINT_MAIL },
            { ItemKindType::GLOVES, SV_SET_OF_LEATHER_GLOVES },
            { ItemKindType::GLOVES, SV_SET_OF_LEATHER_GLOVES },

            { ItemKindType::GLOVES, SV_SET_OF_GAUNTLETS },
            { ItemKindType::SHIELD, SV_SMALL_LEATHER_SHIELD },
            { ItemKindType::SHIELD, SV_LARGE_LEATHER_SHIELD },
            { ItemKindType::SHIELD, SV_SMALL_METAL_SHIELD },

            { ItemKindType::BOOTS, SV_PAIR_OF_HARD_LEATHER_BOOTS },
            { ItemKindType::BOOTS, SV_PAIR_OF_HARD_LEATHER_BOOTS },
            { ItemKindType::HELM, SV_HARD_LEATHER_CAP },
            { ItemKindType::HELM, SV_HARD_LEATHER_CAP },

            { ItemKindType::SOFT_ARMOR, SV_ROBE },
            { ItemKindType::SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
            { ItemKindType::SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
            { ItemKindType::SOFT_ARMOR, SV_HARD_LEATHER_ARMOR },

            { ItemKindType::SOFT_ARMOR, SV_LEATHER_JACK },
            { ItemKindType::HARD_ARMOR, SV_METAL_SCALE_MAIL },
            { ItemKindType::HARD_ARMOR, SV_CHAIN_MAIL },
            { ItemKindType::HARD_ARMOR, SV_CHAIN_MAIL },

            { ItemKindType::GLOVES, SV_SET_OF_LEATHER_GLOVES },
            { ItemKindType::GLOVES, SV_SET_OF_GAUNTLETS },
            { ItemKindType::SHIELD, SV_SMALL_LEATHER_SHIELD },
            { ItemKindType::SHIELD, SV_SMALL_LEATHER_SHIELD } },
    },
    { StoreSaleType::WEAPON,
        {
            { ItemKindType::SWORD, SV_DAGGER },
            { ItemKindType::SWORD, SV_MAIN_GAUCHE },
            { ItemKindType::SWORD, SV_RAPIER },
            { ItemKindType::SWORD, SV_SMALL_SWORD },

            { ItemKindType::SWORD, SV_SHORT_SWORD },
            { ItemKindType::SWORD, SV_SABRE },
            { ItemKindType::SWORD, SV_CUTLASS },
            { ItemKindType::SWORD, SV_TULWAR },

            { ItemKindType::SWORD, SV_BROAD_SWORD },
            { ItemKindType::SWORD, SV_LONG_SWORD },
            { ItemKindType::SWORD, SV_SCIMITAR },
            { ItemKindType::SWORD, SV_KATANA },

            { ItemKindType::SWORD, SV_BASTARD_SWORD },
            { ItemKindType::POLEARM, SV_SPEAR },
            { ItemKindType::POLEARM, SV_AWL_PIKE },
            { ItemKindType::POLEARM, SV_TRIDENT },

            { ItemKindType::POLEARM, SV_PIKE },
            { ItemKindType::POLEARM, SV_BEAKED_AXE },
            { ItemKindType::POLEARM, SV_BROAD_AXE },
            { ItemKindType::POLEARM, SV_LANCE },

            { ItemKindType::POLEARM, SV_BATTLE_AXE },
            { ItemKindType::POLEARM, SV_HATCHET },
            { ItemKindType::BOW, SV_SLING },
            { ItemKindType::BOW, SV_SHORT_BOW },

            { ItemKindType::BOW, SV_LIGHT_XBOW },
            { ItemKindType::SHOT, SV_AMMO_NORMAL },
            { ItemKindType::SHOT, SV_AMMO_NORMAL },
            { ItemKindType::ARROW, SV_AMMO_NORMAL },

            { ItemKindType::ARROW, SV_AMMO_NORMAL },
            { ItemKindType::BOLT, SV_AMMO_NORMAL },
            { ItemKindType::BOLT, SV_AMMO_NORMAL },
            { ItemKindType::BOW, SV_LIGHT_XBOW },

            { ItemKindType::ARROW, SV_AMMO_NORMAL },
            { ItemKindType::BOLT, SV_AMMO_NORMAL },
            { ItemKindType::BOW, SV_SHORT_BOW },
            { ItemKindType::BOW, SV_LIGHT_XBOW },

            { ItemKindType::SWORD, SV_DAGGER },
            { ItemKindType::SWORD, SV_TANTO },
            { ItemKindType::SWORD, SV_RAPIER },
            { ItemKindType::SWORD, SV_SMALL_SWORD },

            { ItemKindType::SWORD, SV_SHORT_SWORD },
            { ItemKindType::SWORD, SV_LONG_SWORD },
            { ItemKindType::SWORD, SV_SCIMITAR },
            { ItemKindType::SWORD, SV_BROAD_SWORD },

            { ItemKindType::HISSATSU_BOOK, 0 },
            { ItemKindType::HISSATSU_BOOK, 0 },
            { ItemKindType::HISSATSU_BOOK, 1 },
            { ItemKindType::HISSATSU_BOOK, 1 },
        } },

    { StoreSaleType::TEMPLE,
        {
            { ItemKindType::HAFTED, SV_NUNCHAKU },
            { ItemKindType::HAFTED, SV_QUARTERSTAFF },
            { ItemKindType::HAFTED, SV_MACE },
            { ItemKindType::HAFTED, SV_BO_STAFF },

            { ItemKindType::HAFTED, SV_WAR_HAMMER },
            { ItemKindType::HAFTED, SV_WAR_HAMMER },
            { ItemKindType::HAFTED, SV_MORNING_STAR },
            { ItemKindType::HAFTED, SV_FLAIL },

            { ItemKindType::HAFTED, SV_LEAD_FILLED_MACE },
            { ItemKindType::SCROLL, SV_SCROLL_REMOVE_CURSE },
            { ItemKindType::SCROLL, SV_SCROLL_BLESSING },
            { ItemKindType::SCROLL, SV_SCROLL_HOLY_CHANT },

            { ItemKindType::SCROLL, SV_SCROLL_WORD_OF_RECALL },
            { ItemKindType::SCROLL, SV_SCROLL_WORD_OF_RECALL },
            { ItemKindType::SCROLL, SV_SCROLL_WORD_OF_RECALL },
            { ItemKindType::POTION, SV_POTION_CURE_LIGHT },

            { ItemKindType::POTION, SV_POTION_HEROISM },
            { ItemKindType::POTION, SV_POTION_HEROISM },
            { ItemKindType::POTION, SV_POTION_CURE_SERIOUS },
            { ItemKindType::POTION, SV_POTION_CURE_SERIOUS },

            { ItemKindType::POTION, SV_POTION_CURE_CRITICAL },
            { ItemKindType::POTION, SV_POTION_CURE_CRITICAL },
            { ItemKindType::POTION, SV_POTION_CURE_CRITICAL },
            { ItemKindType::POTION, SV_POTION_CURE_CRITICAL },

            { ItemKindType::POTION, SV_POTION_RESTORE_EXP },
            { ItemKindType::POTION, SV_POTION_RESTORE_EXP },
            { ItemKindType::POTION, SV_POTION_RESTORE_EXP },
            { ItemKindType::POTION, SV_POTION_RESTORE_EXP },

            { ItemKindType::LIFE_BOOK, 0 },
            { ItemKindType::LIFE_BOOK, 1 },
            { ItemKindType::LIFE_BOOK, 1 },
            { ItemKindType::LIFE_BOOK, 1 },

            { ItemKindType::CRUSADE_BOOK, 0 },
            { ItemKindType::CRUSADE_BOOK, 1 },
            { ItemKindType::CRUSADE_BOOK, 1 },
            { ItemKindType::CRUSADE_BOOK, 1 },

            { ItemKindType::HAFTED, SV_WHIP },
            { ItemKindType::HAFTED, SV_MACE },
            { ItemKindType::HAFTED, SV_BALL_AND_CHAIN },
            { ItemKindType::HAFTED, SV_WAR_HAMMER },

            { ItemKindType::POTION, SV_POTION_RESIST_HEAT },
            { ItemKindType::POTION, SV_POTION_RESIST_COLD },
            { ItemKindType::FIGURINE, 0 },
            { ItemKindType::STATUE },

            { ItemKindType::SCROLL, SV_SCROLL_REMOVE_CURSE },
            { ItemKindType::SCROLL, SV_SCROLL_REMOVE_CURSE },
            { ItemKindType::SCROLL, SV_SCROLL_STAR_REMOVE_CURSE },
            { ItemKindType::SCROLL, SV_SCROLL_STAR_REMOVE_CURSE },
        } },

    { StoreSaleType::ALCHEMIST,
        {
            { ItemKindType::SCROLL, SV_SCROLL_ENCHANT_WEAPON_TO_HIT },
            { ItemKindType::SCROLL, SV_SCROLL_ENCHANT_WEAPON_TO_DAM },
            { ItemKindType::SCROLL, SV_SCROLL_ENCHANT_ARMOR },
            { ItemKindType::SCROLL, SV_SCROLL_IDENTIFY },

            { ItemKindType::SCROLL, SV_SCROLL_IDENTIFY },
            { ItemKindType::SCROLL, SV_SCROLL_IDENTIFY },
            { ItemKindType::SCROLL, SV_SCROLL_IDENTIFY },
            { ItemKindType::SCROLL, SV_SCROLL_LIGHT },

            { ItemKindType::SCROLL, SV_SCROLL_PHASE_DOOR },
            { ItemKindType::SCROLL, SV_SCROLL_PHASE_DOOR },
            { ItemKindType::SCROLL, SV_SCROLL_TELEPORT },
            { ItemKindType::SCROLL, SV_SCROLL_MONSTER_CONFUSION },

            { ItemKindType::SCROLL, SV_SCROLL_MAPPING },
            { ItemKindType::SCROLL, SV_SCROLL_DETECT_GOLD },
            { ItemKindType::SCROLL, SV_SCROLL_DETECT_ITEM },
            { ItemKindType::SCROLL, SV_SCROLL_DETECT_TRAP },

            { ItemKindType::SCROLL, SV_SCROLL_DETECT_INVIS },
            { ItemKindType::SCROLL, SV_SCROLL_RECHARGING },
            { ItemKindType::SCROLL, SV_SCROLL_TELEPORT },
            { ItemKindType::SCROLL, SV_SCROLL_WORD_OF_RECALL },

            { ItemKindType::SCROLL, SV_SCROLL_WORD_OF_RECALL },
            { ItemKindType::SCROLL, SV_SCROLL_WORD_OF_RECALL },
            { ItemKindType::SCROLL, SV_SCROLL_WORD_OF_RECALL },
            { ItemKindType::SCROLL, SV_SCROLL_TELEPORT },

            { ItemKindType::SCROLL, SV_SCROLL_TELEPORT },
            { ItemKindType::POTION, SV_POTION_RES_STR },
            { ItemKindType::POTION, SV_POTION_RES_INT },
            { ItemKindType::POTION, SV_POTION_RES_WIS },

            { ItemKindType::POTION, SV_POTION_RES_DEX },
            { ItemKindType::POTION, SV_POTION_RES_CON },
            { ItemKindType::POTION, SV_POTION_RES_CHR },
            { ItemKindType::SCROLL, SV_SCROLL_IDENTIFY },

            { ItemKindType::SCROLL, SV_SCROLL_IDENTIFY },
            { ItemKindType::SCROLL, SV_SCROLL_STAR_IDENTIFY },
            { ItemKindType::SCROLL, SV_SCROLL_STAR_IDENTIFY },
            { ItemKindType::SCROLL, SV_SCROLL_LIGHT },

            { ItemKindType::POTION, SV_POTION_RES_STR },
            { ItemKindType::POTION, SV_POTION_RES_INT },
            { ItemKindType::POTION, SV_POTION_RES_WIS },
            { ItemKindType::POTION, SV_POTION_RES_DEX },

            { ItemKindType::POTION, SV_POTION_RES_CON },
            { ItemKindType::POTION, SV_POTION_RES_CHR },
            { ItemKindType::SCROLL, SV_SCROLL_ENCHANT_ARMOR },
            { ItemKindType::SCROLL, SV_SCROLL_ENCHANT_ARMOR },

            { ItemKindType::SCROLL, SV_SCROLL_RECHARGING },
            { ItemKindType::SCROLL, SV_SCROLL_PHASE_DOOR },
            { ItemKindType::SCROLL, SV_SCROLL_ENCHANT_WEAPON_TO_HIT },
            { ItemKindType::SCROLL, SV_SCROLL_ENCHANT_WEAPON_TO_DAM },
        } },

    { StoreSaleType::MAGIC,
        {
            { ItemKindType::RING, SV_RING_PROTECTION },
            { ItemKindType::RING, SV_RING_LEVITATION_FALL },
            { ItemKindType::RING, SV_RING_PROTECTION },
            { ItemKindType::RING, SV_RING_RESIST_FIRE },

            { ItemKindType::RING, SV_RING_RESIST_COLD },
            { ItemKindType::AMULET, SV_AMULET_CHARISMA },
            { ItemKindType::RING, SV_RING_WARNING },
            { ItemKindType::AMULET, SV_AMULET_RESIST_ACID },

            { ItemKindType::AMULET, SV_AMULET_SEARCHING },
            { ItemKindType::WAND, SV_WAND_SLOW_MONSTER },
            { ItemKindType::WAND, SV_WAND_CONFUSE_MONSTER },
            { ItemKindType::WAND, SV_WAND_SLEEP_MONSTER },

            { ItemKindType::WAND, SV_WAND_MAGIC_MISSILE },
            { ItemKindType::WAND, SV_WAND_STINKING_CLOUD },
            { ItemKindType::WAND, SV_WAND_WONDER },
            { ItemKindType::WAND, SV_WAND_DISARMING },

            { ItemKindType::STAFF, SV_STAFF_LITE },
            { ItemKindType::STAFF, SV_STAFF_MAPPING },
            { ItemKindType::STAFF, SV_STAFF_DETECT_TRAP },
            { ItemKindType::STAFF, SV_STAFF_DETECT_DOOR },

            { ItemKindType::STAFF, SV_STAFF_DETECT_GOLD },
            { ItemKindType::STAFF, SV_STAFF_DETECT_ITEM },
            { ItemKindType::STAFF, SV_STAFF_DETECT_INVIS },
            { ItemKindType::STAFF, SV_STAFF_DETECT_EVIL },

            { ItemKindType::STAFF, SV_STAFF_TELEPORTATION },
            { ItemKindType::STAFF, SV_STAFF_TELEPORTATION },
            { ItemKindType::STAFF, SV_STAFF_TELEPORTATION },
            { ItemKindType::STAFF, SV_STAFF_TELEPORTATION },

            { ItemKindType::STAFF, SV_STAFF_IDENTIFY },
            { ItemKindType::STAFF, SV_STAFF_IDENTIFY },
            { ItemKindType::STAFF, SV_STAFF_IDENTIFY },

            { ItemKindType::STAFF, SV_STAFF_IDENTIFY },
            { ItemKindType::STAFF, SV_STAFF_REMOVE_CURSE },
            { ItemKindType::STAFF, SV_STAFF_CURE_LIGHT },
            { ItemKindType::STAFF, SV_STAFF_PROBING },

            { ItemKindType::FIGURINE, 0 },

            { ItemKindType::SORCERY_BOOK, 0 },
            { ItemKindType::SORCERY_BOOK, 0 },
            { ItemKindType::SORCERY_BOOK, 1 },
            { ItemKindType::SORCERY_BOOK, 1 },

            { ItemKindType::ARCANE_BOOK, 0 },
            { ItemKindType::ARCANE_BOOK, 0 },
            { ItemKindType::ARCANE_BOOK, 1 },
            { ItemKindType::ARCANE_BOOK, 1 },

            { ItemKindType::ARCANE_BOOK, 2 },
            { ItemKindType::ARCANE_BOOK, 2 },
            { ItemKindType::ARCANE_BOOK, 3 },
            { ItemKindType::ARCANE_BOOK, 3 },
        } },

    { StoreSaleType::BLACK,
        {} },

    { StoreSaleType::HOME,
        {} },

    { StoreSaleType::BOOK,
        {
            { ItemKindType::SORCERY_BOOK, 0 },
            { ItemKindType::SORCERY_BOOK, 0 },
            { ItemKindType::SORCERY_BOOK, 1 },
            { ItemKindType::SORCERY_BOOK, 1 },

            { ItemKindType::NATURE_BOOK, 0 },
            { ItemKindType::NATURE_BOOK, 0 },
            { ItemKindType::NATURE_BOOK, 1 },
            { ItemKindType::NATURE_BOOK, 1 },

            { ItemKindType::CHAOS_BOOK, 0 },
            { ItemKindType::CHAOS_BOOK, 0 },
            { ItemKindType::CHAOS_BOOK, 1 },
            { ItemKindType::CHAOS_BOOK, 1 },

            { ItemKindType::DEATH_BOOK, 0 },
            { ItemKindType::DEATH_BOOK, 0 },
            { ItemKindType::DEATH_BOOK, 1 },
            { ItemKindType::DEATH_BOOK, 1 },

            { ItemKindType::TRUMP_BOOK, 0 },
            { ItemKindType::TRUMP_BOOK, 0 },
            { ItemKindType::TRUMP_BOOK, 1 },
            { ItemKindType::TRUMP_BOOK, 1 },

            { ItemKindType::ARCANE_BOOK, 0 },
            { ItemKindType::ARCANE_BOOK, 1 },
            { ItemKindType::ARCANE_BOOK, 2 },
            { ItemKindType::ARCANE_BOOK, 3 },

            { ItemKindType::CRAFT_BOOK, 0 },
            { ItemKindType::CRAFT_BOOK, 0 },
            { ItemKindType::CRAFT_BOOK, 1 },
            { ItemKindType::CRAFT_BOOK, 1 },

            { ItemKindType::DEMON_BOOK, 0 },
            { ItemKindType::DEMON_BOOK, 0 },
            { ItemKindType::DEMON_BOOK, 1 },
            { ItemKindType::DEMON_BOOK, 1 },

            { ItemKindType::MUSIC_BOOK, 0 },
            { ItemKindType::MUSIC_BOOK, 0 },
            { ItemKindType::MUSIC_BOOK, 1 },
            { ItemKindType::MUSIC_BOOK, 1 },

            { ItemKindType::HEX_BOOK, 0 },
            { ItemKindType::HEX_BOOK, 0 },
            { ItemKindType::HEX_BOOK, 1 },
            { ItemKindType::HEX_BOOK, 1 },
        } },

    { StoreSaleType::MUSEUM,
        {} }
};
