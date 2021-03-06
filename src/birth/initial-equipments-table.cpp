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
byte player_init[MAX_CLASS][3][2] = {
    { /* Warrior */
        { TV_RING, SV_RING_RES_FEAR },
        { TV_HARD_ARMOR, SV_CHAIN_MAIL },
        { TV_SWORD, SV_BROAD_SWORD } },

    { /* Mage */
        { TV_SORCERY_BOOK, 0 },
        { TV_DEATH_BOOK, 0 },
        { TV_SWORD, SV_DAGGER } },

    { /* Priest */
        { TV_SORCERY_BOOK, 0 },
        { TV_DEATH_BOOK, 0 },
        { TV_HAFTED, SV_MACE } },

    { /* Rogue */
        { TV_SORCERY_BOOK, 0 },
        { TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
        { TV_SWORD, SV_DAGGER } },

    { /* Ranger */
        { TV_NATURE_BOOK, 0 },
        { TV_DEATH_BOOK, 0 },
        { TV_SWORD, SV_DAGGER } },

    { /* Paladin */
        { TV_SORCERY_BOOK, 0 },
        { TV_SCROLL, SV_SCROLL_PROTECTION_FROM_EVIL },
        { TV_SWORD, SV_BROAD_SWORD } },

    { /* Warrior-Mage */
        { TV_SORCERY_BOOK, 0 },
        { TV_DEATH_BOOK, 0 },
        { TV_SWORD, SV_SHORT_SWORD } },

    { /* Chaos Warrior */
        { TV_SORCERY_BOOK, 0 },
        { TV_HARD_ARMOR, SV_METAL_SCALE_MAIL },
        { TV_SWORD, SV_BROAD_SWORD } },

    { /* Monk */
        { TV_SORCERY_BOOK, 0 },
        { TV_POTION, SV_POTION_SPEED },
        { TV_POTION, SV_POTION_HEROISM } },

    { /* Mindcrafter */
        { TV_POTION, SV_POTION_SPEED },
        { TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
        { TV_SWORD, SV_SMALL_SWORD } },

    { /* High Mage */
        { TV_SORCERY_BOOK, 0 },
        { TV_RING, SV_RING_SUSTAIN_INT },
        { TV_SWORD, SV_DAGGER } },

    { /* Tourist */
        { TV_FOOD, SV_FOOD_JERKY },
        { TV_SCROLL, SV_SCROLL_MAPPING },
        { TV_BOW, SV_SLING } },

    { /* Imitator */
        { TV_POTION, SV_POTION_SPEED },
        { TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
        { TV_SWORD, SV_SHORT_SWORD } },

    { /* Beastmaster */
        { TV_TRUMP_BOOK, 0 },
        { TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
        { TV_POLEARM, SV_SPEAR } },

    { /* Sorcerer */
        { TV_HAFTED, SV_WIZSTAFF },
        { TV_RING, SV_RING_SUSTAIN_INT },
        { TV_WAND, SV_WAND_MAGIC_MISSILE } },

    {
        /* Archer */
        { TV_BOW, SV_SHORT_BOW },
        { TV_SOFT_ARMOR, SV_LEATHER_SCALE_MAIL },
        { TV_SWORD, SV_SHORT_SWORD },
    },

    {
        /* Magic eater */
        { TV_WAND, SV_WAND_MAGIC_MISSILE },
        { TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
        { TV_SWORD, SV_SHORT_SWORD },
    },

    {
        /* Bard */
        { TV_MUSIC_BOOK, 0 },
        { TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
        { TV_SWORD, SV_SHORT_SWORD },
    },

    {
        /* Red Mage */
        { TV_ARCANE_BOOK, 0 },
        { TV_SOFT_ARMOR, SV_HARD_LEATHER_ARMOR },
        { TV_SWORD, SV_SHORT_SWORD },
    },

    { /* Samurai */
        { TV_HISSATSU_BOOK, 0 },
        { TV_HARD_ARMOR, SV_CHAIN_MAIL },
        { TV_SWORD, SV_BROAD_SWORD }
    },

    { /* ForceTrainer */
        { TV_SORCERY_BOOK, 0 },
        { TV_POTION, SV_POTION_SPEED },
        { TV_POTION, SV_POTION_RESTORE_MANA }
    },

    { /* Blue Mage */
        { TV_SOFT_ARMOR, SV_ROBE },
        { TV_WAND, SV_WAND_MAGIC_MISSILE },
        { TV_SWORD, SV_DAGGER }
    },

    { /* Cavalry */
        { TV_BOW, SV_SHORT_BOW },
        { TV_SOFT_ARMOR, SV_LEATHER_SCALE_MAIL },
        { TV_POLEARM, SV_BROAD_SPEAR }
    },

    { /* Berserker */
        { TV_POTION, SV_POTION_HEALING },
        { TV_HARD_ARMOR, SV_AUGMENTED_CHAIN_MAIL },
        { TV_POLEARM, SV_BROAD_AXE }
    },

    { /* Weaponsmith */
        { TV_RING, SV_RING_RES_FEAR },
        { TV_HARD_ARMOR, SV_CHAIN_MAIL },
        { TV_POLEARM, SV_BROAD_AXE }
    },

    { /* Mirror-Master */
        { TV_POTION, SV_POTION_SPEED },
        { TV_RING, SV_RING_SUSTAIN_INT },
        { TV_SWORD, SV_DAGGER }
    },

    { /* Ninja */
        { TV_POTION, SV_POTION_SPEED },
        { TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
        { TV_SWORD, SV_DAGGER }
    },

    { /* Sniper */
        { TV_BOW, SV_LIGHT_XBOW },
        { TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
        { TV_SWORD, SV_DAGGER }
    },
};
