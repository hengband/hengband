#include "birth/initial-equipments-table.h"
#include "object/tval-types.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-bow-types.h"
#include "sv-definition/sv-food-types.h"
#include "sv-definition/sv-potion-types.h"
#include "sv-definition/sv-ring-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "sv-definition/sv-wand-types.h"
#include "sv-definition/sv-weapon-types.h"

/*!
 * プレイヤーの職業毎の初期装備テーブル。/\n
 * Each player starts out with a few items, given as tval/sval pairs.\n
 * In addition, he always has some food and a few torches.\n
 */
std::vector<std::vector<std::tuple<ItemPrimaryType, byte>>> player_init = {
    {
        /* Warrior */
        std::make_tuple(ItemPrimaryType::TV_RING, SV_RING_RES_FEAR),
        std::make_tuple(ItemPrimaryType::TV_HARD_ARMOR, SV_CHAIN_MAIL),
        std::make_tuple(ItemPrimaryType::TV_SWORD, SV_BROAD_SWORD),
    },

    {
        /* Mage */
        std::make_tuple(ItemPrimaryType::TV_SORCERY_BOOK, 0),
        std::make_tuple(ItemPrimaryType::TV_DEATH_BOOK, 0),
        std::make_tuple(ItemPrimaryType::TV_SWORD, SV_DAGGER),
    },

    {
        /* Priest */
        std::make_tuple(ItemPrimaryType::TV_SORCERY_BOOK, 0),
        std::make_tuple(ItemPrimaryType::TV_DEATH_BOOK, 0),
        std::make_tuple(ItemPrimaryType::TV_HAFTED, SV_MACE),
    },

    {
        /* Rogue */
        std::make_tuple(ItemPrimaryType::TV_SORCERY_BOOK, 0),
        std::make_tuple(ItemPrimaryType::TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR),
        std::make_tuple(ItemPrimaryType::TV_SWORD, SV_DAGGER),
    },

    {
        /* Ranger */
        std::make_tuple(ItemPrimaryType::TV_NATURE_BOOK, 0),
        std::make_tuple(ItemPrimaryType::TV_DEATH_BOOK, 0),
        std::make_tuple(ItemPrimaryType::TV_SWORD, SV_DAGGER),
    },

    {
        /* Paladin */
        std::make_tuple(ItemPrimaryType::TV_SORCERY_BOOK, 0),
        std::make_tuple(ItemPrimaryType::TV_SCROLL, SV_SCROLL_PROTECTION_FROM_EVIL),
        std::make_tuple(ItemPrimaryType::TV_SWORD, SV_BROAD_SWORD),
    },

    {
        /* Warrior-Mage */
        std::make_tuple(ItemPrimaryType::TV_SORCERY_BOOK, 0),
        std::make_tuple(ItemPrimaryType::TV_DEATH_BOOK, 0),
        std::make_tuple(ItemPrimaryType::TV_SWORD, SV_SHORT_SWORD),
    },

    {
        /* Chaos Warrior */
        std::make_tuple(ItemPrimaryType::TV_SORCERY_BOOK, 0),
        std::make_tuple(ItemPrimaryType::TV_HARD_ARMOR, SV_METAL_SCALE_MAIL),
        std::make_tuple(ItemPrimaryType::TV_SWORD, SV_BROAD_SWORD),
    },

    {
        /* Monk */
        std::make_tuple(ItemPrimaryType::TV_SORCERY_BOOK, 0),
        std::make_tuple(ItemPrimaryType::TV_POTION, SV_POTION_SPEED),
        std::make_tuple(ItemPrimaryType::TV_POTION, SV_POTION_HEROISM),
    },

    {
        /* Mindcrafter */
        std::make_tuple(ItemPrimaryType::TV_POTION, SV_POTION_SPEED),
        std::make_tuple(ItemPrimaryType::TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR),
        std::make_tuple(ItemPrimaryType::TV_SWORD, SV_SMALL_SWORD),
    },

    {
        /* High Mage */
        std::make_tuple(ItemPrimaryType::TV_SORCERY_BOOK, 0),
        std::make_tuple(ItemPrimaryType::TV_RING, SV_RING_SUSTAIN_INT),
        std::make_tuple(ItemPrimaryType::TV_SWORD, SV_DAGGER),
    },

    {
        /* Tourist */
        std::make_tuple(ItemPrimaryType::TV_FOOD, SV_FOOD_JERKY),
        std::make_tuple(ItemPrimaryType::TV_SCROLL, SV_SCROLL_MAPPING),
        std::make_tuple(ItemPrimaryType::TV_BOW, SV_SLING),
    },

    {
        /* Imitator */
        std::make_tuple(ItemPrimaryType::TV_POTION, SV_POTION_SPEED),
        std::make_tuple(ItemPrimaryType::TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR),
        std::make_tuple(ItemPrimaryType::TV_SWORD, SV_SHORT_SWORD),
    },

    {
        /* Beastmaster */
        std::make_tuple(ItemPrimaryType::TV_TRUMP_BOOK, 0),
        std::make_tuple(ItemPrimaryType::TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR),
        std::make_tuple(ItemPrimaryType::TV_POLEARM, SV_SPEAR),
    },

    {
        /* Sorcerer */
        std::make_tuple(ItemPrimaryType::TV_HAFTED, SV_WIZSTAFF),
        std::make_tuple(ItemPrimaryType::TV_RING, SV_RING_SUSTAIN_INT),
        std::make_tuple(ItemPrimaryType::TV_WAND, SV_WAND_MAGIC_MISSILE),
    },

    {
        /* Archer */
        std::make_tuple(ItemPrimaryType::TV_BOW, SV_SHORT_BOW),
        std::make_tuple(ItemPrimaryType::TV_SOFT_ARMOR, SV_LEATHER_SCALE_MAIL),
        std::make_tuple(ItemPrimaryType::TV_SWORD, SV_SHORT_SWORD),
    },

    {
        /* Magic eater */
        std::make_tuple(ItemPrimaryType::TV_WAND, SV_WAND_MAGIC_MISSILE),
        std::make_tuple(ItemPrimaryType::TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR),
        std::make_tuple(ItemPrimaryType::TV_SWORD, SV_SHORT_SWORD),
    },

    {
        /* Bard */
        std::make_tuple(ItemPrimaryType::TV_MUSIC_BOOK, 0),
        std::make_tuple(ItemPrimaryType::TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR),
        std::make_tuple(ItemPrimaryType::TV_SWORD, SV_SHORT_SWORD),
    },

    {
        /* Red Mage */
        std::make_tuple(ItemPrimaryType::TV_ARCANE_BOOK, 0),
        std::make_tuple(ItemPrimaryType::TV_SOFT_ARMOR, SV_HARD_LEATHER_ARMOR),
        std::make_tuple(ItemPrimaryType::TV_SWORD, SV_SHORT_SWORD),
    },

    {
        /* Samurai */
        std::make_tuple(ItemPrimaryType::TV_HISSATSU_BOOK, 0),
        std::make_tuple(ItemPrimaryType::TV_HARD_ARMOR, SV_CHAIN_MAIL),
        std::make_tuple(ItemPrimaryType::TV_SWORD, SV_BROAD_SWORD),
    },

    {
        /* ForceTrainer */
        std::make_tuple(ItemPrimaryType::TV_SORCERY_BOOK, 0),
        std::make_tuple(ItemPrimaryType::TV_POTION, SV_POTION_SPEED),
        std::make_tuple(ItemPrimaryType::TV_POTION, SV_POTION_RESTORE_MANA),
    },

    {
        /* Blue Mage */
        std::make_tuple(ItemPrimaryType::TV_SOFT_ARMOR, SV_ROBE),
        std::make_tuple(ItemPrimaryType::TV_WAND, SV_WAND_MAGIC_MISSILE),
        std::make_tuple(ItemPrimaryType::TV_SWORD, SV_DAGGER),
    },

    {
        /* Cavalry */
        std::make_tuple(ItemPrimaryType::TV_BOW, SV_SHORT_BOW),
        std::make_tuple(ItemPrimaryType::TV_SOFT_ARMOR, SV_LEATHER_SCALE_MAIL),
        std::make_tuple(ItemPrimaryType::TV_POLEARM, SV_BROAD_SPEAR),
    },

    {
        /* Berserker */
        std::make_tuple(ItemPrimaryType::TV_POTION, SV_POTION_HEALING),
        std::make_tuple(ItemPrimaryType::TV_HARD_ARMOR, SV_AUGMENTED_CHAIN_MAIL),
        std::make_tuple(ItemPrimaryType::TV_POLEARM, SV_BROAD_AXE),
    },

    {
        /* Weaponsmith */
        std::make_tuple(ItemPrimaryType::TV_RING, SV_RING_RES_FEAR),
        std::make_tuple(ItemPrimaryType::TV_HARD_ARMOR, SV_CHAIN_MAIL),
        std::make_tuple(ItemPrimaryType::TV_POLEARM, SV_BROAD_AXE),
    },

    {
        /* Mirror-Master */
        std::make_tuple(ItemPrimaryType::TV_POTION, SV_POTION_SPEED),
        std::make_tuple(ItemPrimaryType::TV_RING, SV_RING_SUSTAIN_INT),
        std::make_tuple(ItemPrimaryType::TV_SWORD, SV_DAGGER),
    },

    {
        /* Ninja */
        std::make_tuple(ItemPrimaryType::TV_POTION, SV_POTION_SPEED),
        std::make_tuple(ItemPrimaryType::TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR),
        std::make_tuple(ItemPrimaryType::TV_SWORD, SV_DAGGER),
    },

    {
        /* Sniper */
        std::make_tuple(ItemPrimaryType::TV_BOW, SV_LIGHT_XBOW),
        std::make_tuple(ItemPrimaryType::TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR),
        std::make_tuple(ItemPrimaryType::TV_SWORD, SV_DAGGER),
    },

    {
        /* Elementalist */
        std::make_tuple(ItemPrimaryType::TV_POTION, SV_POTION_SPEED),
        std::make_tuple(ItemPrimaryType::TV_RING, SV_RING_SUSTAIN_WIS),
        std::make_tuple(ItemPrimaryType::TV_SWORD, SV_DAGGER),
    },
};
