#include "store/articles-on-sale.h"
#include "sv-definition/sv-amulet-types.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-bow-types.h"
#include "sv-definition/sv-digging-types.h"
#include "sv-definition/sv-food-types.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-potion-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "sv-definition/sv-ring-types.h"
#include "sv-definition/sv-rod-types.h"
#include "sv-definition/sv-staff-types.h"
#include "sv-definition/sv-wand-types.h"
#include "sv-definition/sv-weapon-types.h"

/*!
 * @brief 店舗で常時販売するオブジェクトを定義する 
 * @details
 * 上から優先して配置する。
 * 重複して同じ商品を設定した場合、数量が増える。
 * 17エントリーまで設定可能。(最後はTV_NONE で止める)
 * 種類が多すぎる場合、店舗を埋めつくすので注意。
 */
store_stock_item_type store_regular_table[MAX_STORES][STORE_MAX_KEEP] =
{ 
	{
        /* General Store */
        { ItemPrimaryType::TV_FOOD, SV_FOOD_RATION },
        { ItemPrimaryType::TV_LITE, SV_LITE_TORCH },
        { ItemPrimaryType::TV_LITE, SV_LITE_LANTERN },
        { ItemPrimaryType::TV_FLASK, SV_FLASK_OIL },
        { ItemPrimaryType::TV_POTION, SV_POTION_WATER },
        { ItemPrimaryType::TV_SPIKE, 0 },
        { ItemPrimaryType::TV_NONE, 0 },
    },
    {
        /* Armoury */
        { ItemPrimaryType::TV_NONE, 0 },
    },
    {
        /* Weaponsmith */
        { ItemPrimaryType::TV_HISSATSU_BOOK, 0 },
        { ItemPrimaryType::TV_SHOT, SV_AMMO_NORMAL },
        { ItemPrimaryType::TV_SHOT, SV_AMMO_NORMAL },
        { ItemPrimaryType::TV_ARROW, SV_AMMO_NORMAL },
        { ItemPrimaryType::TV_ARROW, SV_AMMO_NORMAL },
        { ItemPrimaryType::TV_BOLT, SV_AMMO_NORMAL },
        { ItemPrimaryType::TV_BOLT, SV_AMMO_NORMAL },
        { ItemPrimaryType::TV_NONE, 0 },
    },
    {
        /* Temple */
        { ItemPrimaryType::TV_POTION, SV_POTION_CURE_CRITICAL },
        { ItemPrimaryType::TV_POTION, SV_POTION_CURE_SERIOUS },
        { ItemPrimaryType::TV_POTION, SV_POTION_HEROISM },
        { ItemPrimaryType::TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
        { ItemPrimaryType::TV_SCROLL, SV_SCROLL_REMOVE_CURSE },
        { ItemPrimaryType::TV_LIFE_BOOK, 0 },
        { ItemPrimaryType::TV_CRUSADE_BOOK, 0 },
        { ItemPrimaryType::TV_NONE, 0 },
    },
    {
        /* Alchemy shop */
        { ItemPrimaryType::TV_SCROLL, SV_SCROLL_PHASE_DOOR },
        { ItemPrimaryType::TV_SCROLL, SV_SCROLL_PHASE_DOOR },
        { ItemPrimaryType::TV_SCROLL, SV_SCROLL_TELEPORT },
        { ItemPrimaryType::TV_SCROLL, SV_SCROLL_TELEPORT },
        { ItemPrimaryType::TV_SCROLL, SV_SCROLL_RECHARGING },
        { ItemPrimaryType::TV_SCROLL, SV_SCROLL_IDENTIFY },
        { ItemPrimaryType::TV_SCROLL, SV_SCROLL_DETECT_GOLD },
        { ItemPrimaryType::TV_NONE, 0 },
    },
    {
        /* Magic User */
        { ItemPrimaryType::TV_STAFF, SV_STAFF_IDENTIFY },
        { ItemPrimaryType::TV_STAFF, SV_STAFF_IDENTIFY },
        { ItemPrimaryType::TV_STAFF, SV_STAFF_MAPPING },
        { ItemPrimaryType::TV_ARCANE_BOOK, 0 },
        { ItemPrimaryType::TV_SORCERY_BOOK, 0 },
        { ItemPrimaryType::TV_NONE, 0 },
    },
    {
        /* Blackmarket */
        { ItemPrimaryType::TV_NONE, 0 },
    },
    {
        /* Home */
        { ItemPrimaryType::TV_NONE, 0 },
    },
    {
        /* Bookstore */
        { ItemPrimaryType::TV_SORCERY_BOOK, 0 },
        { ItemPrimaryType::TV_NATURE_BOOK, 0 },
        { ItemPrimaryType::TV_CHAOS_BOOK, 0 },
        { ItemPrimaryType::TV_DEATH_BOOK, 0 },
        { ItemPrimaryType::TV_TRUMP_BOOK, 0 },
        { ItemPrimaryType::TV_ARCANE_BOOK, 0 },
        { ItemPrimaryType::TV_CRAFT_BOOK, 0 },
        { ItemPrimaryType::TV_DEMON_BOOK, 0 },
        { ItemPrimaryType::TV_MUSIC_BOOK, 0 },
        { ItemPrimaryType::TV_HEX_BOOK, 0 },
        { ItemPrimaryType::TV_NONE, 0 },
    },
    {
        /* Museum */
        { ItemPrimaryType::TV_NONE, 0 },
    },
};

/*!
 * @brief 店舗でランダム販売するオブジェクトを定義する
 * @details tval/svalのペア
 */
store_stock_item_type store_table[MAX_STORES][STORE_CHOICES] =
{
	{
		/* General Store */
		{ ItemPrimaryType::TV_FOOD, SV_FOOD_RATION },
		{ ItemPrimaryType::TV_FOOD, SV_FOOD_RATION },
		{ ItemPrimaryType::TV_FOOD, SV_FOOD_RATION },
		{ ItemPrimaryType::TV_FOOD, SV_FOOD_RATION },

		{ ItemPrimaryType::TV_FOOD, SV_FOOD_BISCUIT },
		{ ItemPrimaryType::TV_FOOD, SV_FOOD_JERKY },
		{ ItemPrimaryType::TV_FOOD, SV_FOOD_PINT_OF_WINE },
		{ ItemPrimaryType::TV_FOOD, SV_FOOD_PINT_OF_ALE },

		{ ItemPrimaryType::TV_LITE, SV_LITE_TORCH },
		{ ItemPrimaryType::TV_LITE, SV_LITE_TORCH },
		{ ItemPrimaryType::TV_LITE, SV_LITE_LANTERN },
		{ ItemPrimaryType::TV_LITE, SV_LITE_LANTERN },

		{ ItemPrimaryType::TV_POTION, SV_POTION_WATER },
		{ ItemPrimaryType::TV_POTION, SV_POTION_WATER },
		{ ItemPrimaryType::TV_FOOD, SV_FOOD_WAYBREAD },
		{ ItemPrimaryType::TV_FOOD, SV_FOOD_WAYBREAD },

		{ ItemPrimaryType::TV_FLASK, 0 },
		{ ItemPrimaryType::TV_FLASK, 0 },
		{ ItemPrimaryType::TV_SPIKE, 0 },
		{ ItemPrimaryType::TV_SPIKE, 0 },

		{ ItemPrimaryType::TV_SPIKE, 0 },
		{ ItemPrimaryType::TV_SPIKE, 0 },
		{ ItemPrimaryType::TV_SHOT, SV_AMMO_NORMAL },
		{ ItemPrimaryType::TV_SHOT, SV_AMMO_NORMAL },

		{ ItemPrimaryType::TV_ARROW, SV_AMMO_NORMAL },
		{ ItemPrimaryType::TV_ARROW, SV_AMMO_NORMAL },
		{ ItemPrimaryType::TV_BOLT, SV_AMMO_NORMAL },
		{ ItemPrimaryType::TV_BOLT, SV_AMMO_NORMAL },

		{ ItemPrimaryType::TV_DIGGING, SV_SHOVEL },
		{ ItemPrimaryType::TV_DIGGING, SV_SHOVEL },
		{ ItemPrimaryType::TV_DIGGING, SV_PICK },
		{ ItemPrimaryType::TV_CLOAK, SV_CLOAK },

		{ ItemPrimaryType::TV_CLOAK, SV_CLOAK },
		{ ItemPrimaryType::TV_CLOAK, SV_FUR_CLOAK },
		{ ItemPrimaryType::TV_CAPTURE, 0 },
		{ ItemPrimaryType::TV_CAPTURE, 0 },

		{ ItemPrimaryType::TV_FIGURINE, 0 },
        { ItemPrimaryType::TV_WHISTLE, 1 },
        { ItemPrimaryType::TV_ROD, SV_ROD_PESTICIDE },
        { ItemPrimaryType::TV_STATUE, SV_ANY },

		{ ItemPrimaryType::TV_NONE, 0 },
        { ItemPrimaryType::TV_NONE, 0 },
        { ItemPrimaryType::TV_NONE, 0 },
        { ItemPrimaryType::TV_NONE, 0 },

		{ ItemPrimaryType::TV_NONE, 0 },
        { ItemPrimaryType::TV_NONE, 0 },
        { ItemPrimaryType::TV_NONE, 0 },
        { ItemPrimaryType::TV_NONE, 0 },
	},

	{
		/* Armoury */
		{ ItemPrimaryType::TV_BOOTS, SV_PAIR_OF_SOFT_LEATHER_BOOTS },
		{ ItemPrimaryType::TV_BOOTS, SV_PAIR_OF_SOFT_LEATHER_BOOTS },
		{ ItemPrimaryType::TV_BOOTS, SV_PAIR_OF_HARD_LEATHER_BOOTS },
		{ ItemPrimaryType::TV_BOOTS, SV_PAIR_OF_HARD_LEATHER_BOOTS },

		{ ItemPrimaryType::TV_HELM, SV_HARD_LEATHER_CAP },
		{ ItemPrimaryType::TV_HELM, SV_HARD_LEATHER_CAP },
		{ ItemPrimaryType::TV_HELM, SV_METAL_CAP },
		{ ItemPrimaryType::TV_HELM, SV_IRON_HELM },

		{ ItemPrimaryType::TV_SOFT_ARMOR, SV_ROBE },
		{ ItemPrimaryType::TV_SOFT_ARMOR, SV_ROBE },
		{ ItemPrimaryType::TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
		{ ItemPrimaryType::TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },

		{ ItemPrimaryType::TV_SOFT_ARMOR, SV_HARD_LEATHER_ARMOR },
		{ ItemPrimaryType::TV_SOFT_ARMOR, SV_HARD_LEATHER_ARMOR },
		{ ItemPrimaryType::TV_SOFT_ARMOR, SV_HARD_STUDDED_LEATHER },
		{ ItemPrimaryType::TV_SOFT_ARMOR, SV_HARD_STUDDED_LEATHER },

		{ ItemPrimaryType::TV_SOFT_ARMOR, SV_RHINO_HIDE_ARMOR },
		{ ItemPrimaryType::TV_SOFT_ARMOR, SV_LEATHER_SCALE_MAIL },
		{ ItemPrimaryType::TV_HARD_ARMOR, SV_METAL_SCALE_MAIL },
		{ ItemPrimaryType::TV_HARD_ARMOR, SV_CHAIN_MAIL },

		{ ItemPrimaryType::TV_HARD_ARMOR, SV_DOUBLE_RING_MAIL },
		{ ItemPrimaryType::TV_HARD_ARMOR, SV_AUGMENTED_CHAIN_MAIL },
		{ ItemPrimaryType::TV_HARD_ARMOR, SV_BAR_CHAIN_MAIL },
		{ ItemPrimaryType::TV_HARD_ARMOR, SV_DOUBLE_CHAIN_MAIL },

		{ ItemPrimaryType::TV_HARD_ARMOR, SV_METAL_BRIGANDINE_ARMOUR },
		{ ItemPrimaryType::TV_HARD_ARMOR, SV_SPLINT_MAIL },
		{ ItemPrimaryType::TV_GLOVES, SV_SET_OF_LEATHER_GLOVES },
		{ ItemPrimaryType::TV_GLOVES, SV_SET_OF_LEATHER_GLOVES },

		{ ItemPrimaryType::TV_GLOVES, SV_SET_OF_GAUNTLETS },
		{ ItemPrimaryType::TV_SHIELD, SV_SMALL_LEATHER_SHIELD },
		{ ItemPrimaryType::TV_SHIELD, SV_LARGE_LEATHER_SHIELD },
		{ ItemPrimaryType::TV_SHIELD, SV_SMALL_METAL_SHIELD },

		{ ItemPrimaryType::TV_BOOTS, SV_PAIR_OF_HARD_LEATHER_BOOTS },
		{ ItemPrimaryType::TV_BOOTS, SV_PAIR_OF_HARD_LEATHER_BOOTS },
		{ ItemPrimaryType::TV_HELM, SV_HARD_LEATHER_CAP },
		{ ItemPrimaryType::TV_HELM, SV_HARD_LEATHER_CAP },

		{ ItemPrimaryType::TV_SOFT_ARMOR, SV_ROBE },
		{ ItemPrimaryType::TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
		{ ItemPrimaryType::TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
		{ ItemPrimaryType::TV_SOFT_ARMOR, SV_HARD_LEATHER_ARMOR },

		{ ItemPrimaryType::TV_SOFT_ARMOR, SV_LEATHER_JACK },
		{ ItemPrimaryType::TV_HARD_ARMOR, SV_METAL_SCALE_MAIL },
		{ ItemPrimaryType::TV_HARD_ARMOR, SV_CHAIN_MAIL },
		{ ItemPrimaryType::TV_HARD_ARMOR, SV_CHAIN_MAIL },

		{ ItemPrimaryType::TV_GLOVES, SV_SET_OF_LEATHER_GLOVES },
		{ ItemPrimaryType::TV_GLOVES, SV_SET_OF_GAUNTLETS },
		{ ItemPrimaryType::TV_SHIELD, SV_SMALL_LEATHER_SHIELD },
		{ ItemPrimaryType::TV_SHIELD, SV_SMALL_LEATHER_SHIELD }
	},

	{
		/* Weaponsmith */
		{ ItemPrimaryType::TV_SWORD, SV_DAGGER },
		{ ItemPrimaryType::TV_SWORD, SV_MAIN_GAUCHE },
		{ ItemPrimaryType::TV_SWORD, SV_RAPIER },
		{ ItemPrimaryType::TV_SWORD, SV_SMALL_SWORD },

		{ ItemPrimaryType::TV_SWORD, SV_SHORT_SWORD },
		{ ItemPrimaryType::TV_SWORD, SV_SABRE },
		{ ItemPrimaryType::TV_SWORD, SV_CUTLASS },
		{ ItemPrimaryType::TV_SWORD, SV_TULWAR },

		{ ItemPrimaryType::TV_SWORD, SV_BROAD_SWORD },
		{ ItemPrimaryType::TV_SWORD, SV_LONG_SWORD },
		{ ItemPrimaryType::TV_SWORD, SV_SCIMITAR },
		{ ItemPrimaryType::TV_SWORD, SV_KATANA },

		{ ItemPrimaryType::TV_SWORD, SV_BASTARD_SWORD },
		{ ItemPrimaryType::TV_POLEARM, SV_SPEAR },
		{ ItemPrimaryType::TV_POLEARM, SV_AWL_PIKE },
		{ ItemPrimaryType::TV_POLEARM, SV_TRIDENT },

		{ ItemPrimaryType::TV_POLEARM, SV_PIKE },
		{ ItemPrimaryType::TV_POLEARM, SV_BEAKED_AXE },
		{ ItemPrimaryType::TV_POLEARM, SV_BROAD_AXE },
		{ ItemPrimaryType::TV_POLEARM, SV_LANCE },

		{ ItemPrimaryType::TV_POLEARM, SV_BATTLE_AXE },
		{ ItemPrimaryType::TV_POLEARM, SV_HATCHET },
		{ ItemPrimaryType::TV_BOW, SV_SLING },
		{ ItemPrimaryType::TV_BOW, SV_SHORT_BOW },

		{ ItemPrimaryType::TV_BOW, SV_LIGHT_XBOW },
		{ ItemPrimaryType::TV_SHOT, SV_AMMO_NORMAL },
		{ ItemPrimaryType::TV_SHOT, SV_AMMO_NORMAL },
		{ ItemPrimaryType::TV_ARROW, SV_AMMO_NORMAL },

		{ ItemPrimaryType::TV_ARROW, SV_AMMO_NORMAL },
		{ ItemPrimaryType::TV_BOLT, SV_AMMO_NORMAL },
		{ ItemPrimaryType::TV_BOLT, SV_AMMO_NORMAL },
		{ ItemPrimaryType::TV_BOW, SV_LIGHT_XBOW },

		{ ItemPrimaryType::TV_ARROW, SV_AMMO_NORMAL },
		{ ItemPrimaryType::TV_BOLT, SV_AMMO_NORMAL },
		{ ItemPrimaryType::TV_BOW, SV_SHORT_BOW },
		{ ItemPrimaryType::TV_BOW, SV_LIGHT_XBOW },

		{ ItemPrimaryType::TV_SWORD, SV_DAGGER },
		{ ItemPrimaryType::TV_SWORD, SV_TANTO },
		{ ItemPrimaryType::TV_SWORD, SV_RAPIER },
		{ ItemPrimaryType::TV_SWORD, SV_SMALL_SWORD },

		{ ItemPrimaryType::TV_SWORD, SV_SHORT_SWORD },
		{ ItemPrimaryType::TV_SWORD, SV_LONG_SWORD },
		{ ItemPrimaryType::TV_SWORD, SV_SCIMITAR },
		{ ItemPrimaryType::TV_SWORD, SV_BROAD_SWORD },

		{ ItemPrimaryType::TV_HISSATSU_BOOK, 0 },
		{ ItemPrimaryType::TV_HISSATSU_BOOK, 0 },
		{ ItemPrimaryType::TV_HISSATSU_BOOK, 1 },
		{ ItemPrimaryType::TV_HISSATSU_BOOK, 1 },
	},

	{
		/* Temple */
		{ ItemPrimaryType::TV_HAFTED, SV_NUNCHAKU },
		{ ItemPrimaryType::TV_HAFTED, SV_QUARTERSTAFF },
		{ ItemPrimaryType::TV_HAFTED, SV_MACE },
		{ ItemPrimaryType::TV_HAFTED, SV_BO_STAFF },

		{ ItemPrimaryType::TV_HAFTED, SV_WAR_HAMMER },
		{ ItemPrimaryType::TV_HAFTED, SV_WAR_HAMMER },
		{ ItemPrimaryType::TV_HAFTED, SV_MORNING_STAR },
		{ ItemPrimaryType::TV_HAFTED, SV_FLAIL },

		{ ItemPrimaryType::TV_HAFTED, SV_LEAD_FILLED_MACE },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_REMOVE_CURSE },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_BLESSING },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_HOLY_CHANT },

		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
        { ItemPrimaryType::TV_POTION, SV_POTION_CURE_LIGHT },

		{ ItemPrimaryType::TV_POTION, SV_POTION_HEROISM },
		{ ItemPrimaryType::TV_POTION, SV_POTION_HEROISM },
		{ ItemPrimaryType::TV_POTION, SV_POTION_CURE_SERIOUS },
		{ ItemPrimaryType::TV_POTION, SV_POTION_CURE_SERIOUS },

		{ ItemPrimaryType::TV_POTION, SV_POTION_CURE_CRITICAL },
		{ ItemPrimaryType::TV_POTION, SV_POTION_CURE_CRITICAL },
		{ ItemPrimaryType::TV_POTION, SV_POTION_CURE_CRITICAL },
		{ ItemPrimaryType::TV_POTION, SV_POTION_CURE_CRITICAL },

		{ ItemPrimaryType::TV_POTION, SV_POTION_RESTORE_EXP },
		{ ItemPrimaryType::TV_POTION, SV_POTION_RESTORE_EXP },
		{ ItemPrimaryType::TV_POTION, SV_POTION_RESTORE_EXP },
		{ ItemPrimaryType::TV_POTION, SV_POTION_RESTORE_EXP },

		{ ItemPrimaryType::TV_LIFE_BOOK, 0 },
		{ ItemPrimaryType::TV_LIFE_BOOK, 1 },
		{ ItemPrimaryType::TV_LIFE_BOOK, 1 },
		{ ItemPrimaryType::TV_LIFE_BOOK, 1 },

		{ ItemPrimaryType::TV_CRUSADE_BOOK, 0 },
		{ ItemPrimaryType::TV_CRUSADE_BOOK, 1 },
		{ ItemPrimaryType::TV_CRUSADE_BOOK, 1 },
		{ ItemPrimaryType::TV_CRUSADE_BOOK, 1 },

		{ ItemPrimaryType::TV_HAFTED, SV_WHIP },
		{ ItemPrimaryType::TV_HAFTED, SV_MACE },
		{ ItemPrimaryType::TV_HAFTED, SV_BALL_AND_CHAIN },
		{ ItemPrimaryType::TV_HAFTED, SV_WAR_HAMMER },

		{ ItemPrimaryType::TV_POTION, SV_POTION_RESIST_HEAT },
		{ ItemPrimaryType::TV_POTION, SV_POTION_RESIST_COLD },
		{ ItemPrimaryType::TV_FIGURINE, 0 },
		{ ItemPrimaryType::TV_STATUE, SV_ANY },

		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_REMOVE_CURSE },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_REMOVE_CURSE },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_STAR_REMOVE_CURSE },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_STAR_REMOVE_CURSE },
	},

	{
		/* Alchemy shop */
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_ENCHANT_WEAPON_TO_HIT },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_ENCHANT_WEAPON_TO_DAM },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_ENCHANT_ARMOR },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_IDENTIFY },

		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_IDENTIFY },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_IDENTIFY },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_IDENTIFY },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_LIGHT },

		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_PHASE_DOOR },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_PHASE_DOOR },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_TELEPORT },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_MONSTER_CONFUSION },

		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_MAPPING },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_DETECT_GOLD },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_DETECT_ITEM },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_DETECT_TRAP },

		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_DETECT_INVIS },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_RECHARGING },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_TELEPORT },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },

		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_TELEPORT },

		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_TELEPORT },
		{ ItemPrimaryType::TV_POTION, SV_POTION_RES_STR },
		{ ItemPrimaryType::TV_POTION, SV_POTION_RES_INT },
		{ ItemPrimaryType::TV_POTION, SV_POTION_RES_WIS },

		{ ItemPrimaryType::TV_POTION, SV_POTION_RES_DEX },
		{ ItemPrimaryType::TV_POTION, SV_POTION_RES_CON },
		{ ItemPrimaryType::TV_POTION, SV_POTION_RES_CHR },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_IDENTIFY },

		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_IDENTIFY },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_STAR_IDENTIFY },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_STAR_IDENTIFY },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_LIGHT },

		{ ItemPrimaryType::TV_POTION, SV_POTION_RES_STR },
		{ ItemPrimaryType::TV_POTION, SV_POTION_RES_INT },
		{ ItemPrimaryType::TV_POTION, SV_POTION_RES_WIS },
		{ ItemPrimaryType::TV_POTION, SV_POTION_RES_DEX },

		{ ItemPrimaryType::TV_POTION, SV_POTION_RES_CON },
		{ ItemPrimaryType::TV_POTION, SV_POTION_RES_CHR },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_ENCHANT_ARMOR },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_ENCHANT_ARMOR },

		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_RECHARGING },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_PHASE_DOOR },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_ENCHANT_WEAPON_TO_HIT },
		{ ItemPrimaryType::TV_SCROLL, SV_SCROLL_ENCHANT_WEAPON_TO_DAM },

	},

	{
		/* Magic-User store */
		{ ItemPrimaryType::TV_RING, SV_RING_PROTECTION },
		{ ItemPrimaryType::TV_RING, SV_RING_LEVITATION_FALL },
		{ ItemPrimaryType::TV_RING, SV_RING_PROTECTION },
		{ ItemPrimaryType::TV_RING, SV_RING_RESIST_FIRE },

		{ ItemPrimaryType::TV_RING, SV_RING_RESIST_COLD },
		{ ItemPrimaryType::TV_AMULET, SV_AMULET_CHARISMA },
		{ ItemPrimaryType::TV_RING, SV_RING_WARNING },
		{ ItemPrimaryType::TV_AMULET, SV_AMULET_RESIST_ACID },

		{ ItemPrimaryType::TV_AMULET, SV_AMULET_SEARCHING },
		{ ItemPrimaryType::TV_WAND, SV_WAND_SLOW_MONSTER },
		{ ItemPrimaryType::TV_WAND, SV_WAND_CONFUSE_MONSTER },
		{ ItemPrimaryType::TV_WAND, SV_WAND_SLEEP_MONSTER },

		{ ItemPrimaryType::TV_WAND, SV_WAND_MAGIC_MISSILE },
		{ ItemPrimaryType::TV_WAND, SV_WAND_STINKING_CLOUD },
		{ ItemPrimaryType::TV_WAND, SV_WAND_WONDER },
		{ ItemPrimaryType::TV_WAND, SV_WAND_DISARMING },

		{ ItemPrimaryType::TV_STAFF, SV_STAFF_LITE },
		{ ItemPrimaryType::TV_STAFF, SV_STAFF_MAPPING },
		{ ItemPrimaryType::TV_STAFF, SV_STAFF_DETECT_TRAP },
		{ ItemPrimaryType::TV_STAFF, SV_STAFF_DETECT_DOOR },

		{ ItemPrimaryType::TV_STAFF, SV_STAFF_DETECT_GOLD },
		{ ItemPrimaryType::TV_STAFF, SV_STAFF_DETECT_ITEM },
		{ ItemPrimaryType::TV_STAFF, SV_STAFF_DETECT_INVIS },
		{ ItemPrimaryType::TV_STAFF, SV_STAFF_DETECT_EVIL },

		{ ItemPrimaryType::TV_STAFF, SV_STAFF_TELEPORTATION },
		{ ItemPrimaryType::TV_STAFF, SV_STAFF_TELEPORTATION },
		{ ItemPrimaryType::TV_STAFF, SV_STAFF_TELEPORTATION },
		{ ItemPrimaryType::TV_STAFF, SV_STAFF_TELEPORTATION },

		{ ItemPrimaryType::TV_STAFF, SV_STAFF_IDENTIFY },
		{ ItemPrimaryType::TV_STAFF, SV_STAFF_IDENTIFY },
		{ ItemPrimaryType::TV_STAFF, SV_STAFF_IDENTIFY },

		{ ItemPrimaryType::TV_STAFF, SV_STAFF_IDENTIFY },
		{ ItemPrimaryType::TV_STAFF, SV_STAFF_REMOVE_CURSE },
		{ ItemPrimaryType::TV_STAFF, SV_STAFF_CURE_LIGHT },
		{ ItemPrimaryType::TV_STAFF, SV_STAFF_PROBING },

		{ ItemPrimaryType::TV_FIGURINE, 0 },

		{ ItemPrimaryType::TV_SORCERY_BOOK, 0 },
		{ ItemPrimaryType::TV_SORCERY_BOOK, 0 },
		{ ItemPrimaryType::TV_SORCERY_BOOK, 1 },
		{ ItemPrimaryType::TV_SORCERY_BOOK, 1 },

		{ ItemPrimaryType::TV_ARCANE_BOOK, 0 },
		{ ItemPrimaryType::TV_ARCANE_BOOK, 0 },
		{ ItemPrimaryType::TV_ARCANE_BOOK, 1 },
		{ ItemPrimaryType::TV_ARCANE_BOOK, 1 },

		{ ItemPrimaryType::TV_ARCANE_BOOK, 2 },
		{ ItemPrimaryType::TV_ARCANE_BOOK, 2 },
		{ ItemPrimaryType::TV_ARCANE_BOOK, 3 },
		{ ItemPrimaryType::TV_ARCANE_BOOK, 3 },
	},

	{
		/* Black Market (unused) */
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 }
	},

	{
		/* Home (unused) */
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 }
	},

	{
		/* Bookstore */
		{ ItemPrimaryType::TV_SORCERY_BOOK, 0 },
		{ ItemPrimaryType::TV_SORCERY_BOOK, 0 },
		{ ItemPrimaryType::TV_SORCERY_BOOK, 1 },
		{ ItemPrimaryType::TV_SORCERY_BOOK, 1 },

		{ ItemPrimaryType::TV_NATURE_BOOK, 0 },
		{ ItemPrimaryType::TV_NATURE_BOOK, 0 },
		{ ItemPrimaryType::TV_NATURE_BOOK, 1 },
		{ ItemPrimaryType::TV_NATURE_BOOK, 1 },

		{ ItemPrimaryType::TV_CHAOS_BOOK, 0 },
		{ ItemPrimaryType::TV_CHAOS_BOOK, 0 },
		{ ItemPrimaryType::TV_CHAOS_BOOK, 1 },
		{ ItemPrimaryType::TV_CHAOS_BOOK, 1 },

		{ ItemPrimaryType::TV_DEATH_BOOK, 0 },
		{ ItemPrimaryType::TV_DEATH_BOOK, 0 },
		{ ItemPrimaryType::TV_DEATH_BOOK, 1 },
		{ ItemPrimaryType::TV_DEATH_BOOK, 1 },

		{ ItemPrimaryType::TV_TRUMP_BOOK, 0 },
		{ ItemPrimaryType::TV_TRUMP_BOOK, 0 },
		{ ItemPrimaryType::TV_TRUMP_BOOK, 1 },
		{ ItemPrimaryType::TV_TRUMP_BOOK, 1 },

		{ ItemPrimaryType::TV_ARCANE_BOOK, 0 },
		{ ItemPrimaryType::TV_ARCANE_BOOK, 1 },
		{ ItemPrimaryType::TV_ARCANE_BOOK, 2 },
		{ ItemPrimaryType::TV_ARCANE_BOOK, 3 },

		{ ItemPrimaryType::TV_CRAFT_BOOK, 0 },
		{ ItemPrimaryType::TV_CRAFT_BOOK, 0 },
		{ ItemPrimaryType::TV_CRAFT_BOOK, 1 },
		{ ItemPrimaryType::TV_CRAFT_BOOK, 1 },

		{ ItemPrimaryType::TV_DEMON_BOOK, 0 },
		{ ItemPrimaryType::TV_DEMON_BOOK, 0 },
		{ ItemPrimaryType::TV_DEMON_BOOK, 1 },
		{ ItemPrimaryType::TV_DEMON_BOOK, 1 },

		{ ItemPrimaryType::TV_MUSIC_BOOK, 0 },
		{ ItemPrimaryType::TV_MUSIC_BOOK, 0 },
		{ ItemPrimaryType::TV_MUSIC_BOOK, 1 },
		{ ItemPrimaryType::TV_MUSIC_BOOK, 1 },

		{ ItemPrimaryType::TV_HEX_BOOK, 0 },
		{ ItemPrimaryType::TV_HEX_BOOK, 0 },
		{ ItemPrimaryType::TV_HEX_BOOK, 1 },
		{ ItemPrimaryType::TV_HEX_BOOK, 1 },
	},

	{
		/* Museum (unused) */
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 },
		{ ItemPrimaryType::TV_NONE, 0 }
	}
};
