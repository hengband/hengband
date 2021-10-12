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

// リテラルはintと解釈されるので警告対処のため定数を定義しておく
constexpr byte FIRST_BOOK = 0;

/*!
 * プレイヤーの職業毎の初期装備テーブル。/\n
 * Each player starts out with a few items, given as tval/sval pairs.\n
 * In addition, he always has some food and a few torches.\n
 */
std::vector<std::vector<std::tuple<ItemKindType, byte>>> player_init = {
    {
        /* Warrior */
        std::make_tuple(ItemKindType::RING, static_cast<byte>(SV_RING_RES_FEAR)),
        std::make_tuple(ItemKindType::HARD_ARMOR, static_cast<byte>(SV_CHAIN_MAIL)),
        std::make_tuple(ItemKindType::SWORD, static_cast<byte>(SV_BROAD_SWORD)),
    },

    {
        /* Mage */
        std::make_tuple(ItemKindType::SORCERY_BOOK, FIRST_BOOK),
        std::make_tuple(ItemKindType::DEATH_BOOK, FIRST_BOOK),
        std::make_tuple(ItemKindType::SWORD, static_cast<byte>(SV_DAGGER)),
    },

    {
        /* Priest */
        std::make_tuple(ItemKindType::SORCERY_BOOK, FIRST_BOOK),
        std::make_tuple(ItemKindType::DEATH_BOOK, FIRST_BOOK),
        std::make_tuple(ItemKindType::HAFTED, static_cast<byte>(SV_MACE)),
    },

    {
        /* Rogue */
        std::make_tuple(ItemKindType::SORCERY_BOOK, FIRST_BOOK),
        std::make_tuple(ItemKindType::SOFT_ARMOR, static_cast<byte>(SV_SOFT_LEATHER_ARMOR)),
        std::make_tuple(ItemKindType::SWORD, static_cast<byte>(SV_DAGGER)),
    },

    {
        /* Ranger */
        std::make_tuple(ItemKindType::NATURE_BOOK, FIRST_BOOK),
        std::make_tuple(ItemKindType::DEATH_BOOK, FIRST_BOOK),
        std::make_tuple(ItemKindType::SWORD, static_cast<byte>(SV_DAGGER)),
    },

    {
        /* Paladin */
        std::make_tuple(ItemKindType::SORCERY_BOOK, FIRST_BOOK),
        std::make_tuple(ItemKindType::SCROLL, static_cast<byte>(SV_SCROLL_PROTECTION_FROM_EVIL)),
        std::make_tuple(ItemKindType::SWORD, static_cast<byte>(SV_BROAD_SWORD)),
    },

    {
        /* Warrior-Mage */
        std::make_tuple(ItemKindType::SORCERY_BOOK, FIRST_BOOK),
        std::make_tuple(ItemKindType::DEATH_BOOK, FIRST_BOOK),
        std::make_tuple(ItemKindType::SWORD, static_cast<byte>(SV_SHORT_SWORD)),
    },

    {
        /* Chaos Warrior */
        std::make_tuple(ItemKindType::SORCERY_BOOK, FIRST_BOOK),
        std::make_tuple(ItemKindType::HARD_ARMOR, static_cast<byte>(SV_METAL_SCALE_MAIL)),
        std::make_tuple(ItemKindType::SWORD, static_cast<byte>(SV_BROAD_SWORD)),
    },

    {
        /* Monk */
        std::make_tuple(ItemKindType::SORCERY_BOOK, FIRST_BOOK),
        std::make_tuple(ItemKindType::POTION, static_cast<byte>(SV_POTION_SPEED)),
        std::make_tuple(ItemKindType::POTION, static_cast<byte>(SV_POTION_HEROISM)),
    },

    {
        /* Mindcrafter */
        std::make_tuple(ItemKindType::POTION, static_cast<byte>(SV_POTION_SPEED)),
        std::make_tuple(ItemKindType::SOFT_ARMOR, static_cast<byte>(SV_SOFT_LEATHER_ARMOR)),
        std::make_tuple(ItemKindType::SWORD, static_cast<byte>(SV_SMALL_SWORD)),
    },

    {
        /* High Mage */
        std::make_tuple(ItemKindType::SORCERY_BOOK, FIRST_BOOK),
        std::make_tuple(ItemKindType::RING, static_cast<byte>(SV_RING_SUSTAIN_INT)),
        std::make_tuple(ItemKindType::SWORD, static_cast<byte>(SV_DAGGER)),
    },

    {
        /* Tourist */
        std::make_tuple(ItemKindType::FOOD, static_cast<byte>(SV_FOOD_JERKY)),
        std::make_tuple(ItemKindType::SCROLL, static_cast<byte>(SV_SCROLL_MAPPING)),
        std::make_tuple(ItemKindType::BOW, static_cast<byte>(SV_SLING)),
    },

    {
        /* Imitator */
        std::make_tuple(ItemKindType::POTION, static_cast<byte>(SV_POTION_SPEED)),
        std::make_tuple(ItemKindType::SOFT_ARMOR, static_cast<byte>(SV_SOFT_LEATHER_ARMOR)),
        std::make_tuple(ItemKindType::SWORD, static_cast<byte>(SV_SHORT_SWORD)),
    },

    {
        /* Beastmaster */
        std::make_tuple(ItemKindType::TRUMP_BOOK, FIRST_BOOK),
        std::make_tuple(ItemKindType::SOFT_ARMOR, static_cast<byte>(SV_SOFT_LEATHER_ARMOR)),
        std::make_tuple(ItemKindType::POLEARM, static_cast<byte>(SV_SPEAR)),
    },

    {
        /* Sorcerer */
        std::make_tuple(ItemKindType::HAFTED, static_cast<byte>(SV_WIZSTAFF)),
        std::make_tuple(ItemKindType::RING, static_cast<byte>(SV_RING_SUSTAIN_INT)),
        std::make_tuple(ItemKindType::WAND, static_cast<byte>(SV_WAND_MAGIC_MISSILE)),
    },

    {
        /* Archer */
        std::make_tuple(ItemKindType::BOW, static_cast<byte>(SV_SHORT_BOW)),
        std::make_tuple(ItemKindType::SOFT_ARMOR, static_cast<byte>(SV_LEATHER_SCALE_MAIL)),
        std::make_tuple(ItemKindType::SWORD, static_cast<byte>(SV_SHORT_SWORD)),
    },

    {
        /* Magic eater */
        std::make_tuple(ItemKindType::WAND, static_cast<byte>(SV_WAND_MAGIC_MISSILE)),
        std::make_tuple(ItemKindType::SOFT_ARMOR, static_cast<byte>(SV_SOFT_LEATHER_ARMOR)),
        std::make_tuple(ItemKindType::SWORD, static_cast<byte>(SV_SHORT_SWORD)),
    },

    {
        /* Bard */
        std::make_tuple(ItemKindType::MUSIC_BOOK, FIRST_BOOK),
        std::make_tuple(ItemKindType::SOFT_ARMOR, static_cast<byte>(SV_SOFT_LEATHER_ARMOR)),
        std::make_tuple(ItemKindType::SWORD, static_cast<byte>(SV_SHORT_SWORD)),
    },

    {
        /* Red Mage */
        std::make_tuple(ItemKindType::ARCANE_BOOK, FIRST_BOOK),
        std::make_tuple(ItemKindType::SOFT_ARMOR, static_cast<byte>(SV_HARD_LEATHER_ARMOR)),
        std::make_tuple(ItemKindType::SWORD, static_cast<byte>(SV_SHORT_SWORD)),
    },

    {
        /* Samurai */
        std::make_tuple(ItemKindType::HISSATSU_BOOK, FIRST_BOOK),
        std::make_tuple(ItemKindType::HARD_ARMOR, static_cast<byte>(SV_CHAIN_MAIL)),
        std::make_tuple(ItemKindType::SWORD, static_cast<byte>(SV_BROAD_SWORD)),
    },

    {
        /* ForceTrainer */
        std::make_tuple(ItemKindType::SORCERY_BOOK, FIRST_BOOK),
        std::make_tuple(ItemKindType::POTION, static_cast<byte>(SV_POTION_SPEED)),
        std::make_tuple(ItemKindType::POTION, static_cast<byte>(SV_POTION_RESTORE_MANA)),
    },

    {
        /* Blue Mage */
        std::make_tuple(ItemKindType::SOFT_ARMOR, static_cast<byte>(SV_ROBE)),
        std::make_tuple(ItemKindType::WAND, static_cast<byte>(SV_WAND_MAGIC_MISSILE)),
        std::make_tuple(ItemKindType::SWORD, static_cast<byte>(SV_DAGGER)),
    },

    {
        /* Cavalry */
        std::make_tuple(ItemKindType::BOW, static_cast<byte>(SV_SHORT_BOW)),
        std::make_tuple(ItemKindType::SOFT_ARMOR, static_cast<byte>(SV_LEATHER_SCALE_MAIL)),
        std::make_tuple(ItemKindType::POLEARM, static_cast<byte>(SV_BROAD_SPEAR)),
    },

    {
        /* Berserker */
        std::make_tuple(ItemKindType::POTION, static_cast<byte>(SV_POTION_HEALING)),
        std::make_tuple(ItemKindType::HARD_ARMOR, static_cast<byte>(SV_AUGMENTED_CHAIN_MAIL)),
        std::make_tuple(ItemKindType::POLEARM, static_cast<byte>(SV_BROAD_AXE)),
    },

    {
        /* Weaponsmith */
        std::make_tuple(ItemKindType::RING, static_cast<byte>(SV_RING_RES_FEAR)),
        std::make_tuple(ItemKindType::HARD_ARMOR, static_cast<byte>(SV_CHAIN_MAIL)),
        std::make_tuple(ItemKindType::POLEARM, static_cast<byte>(SV_BROAD_AXE)),
    },

    {
        /* Mirror-Master */
        std::make_tuple(ItemKindType::POTION, static_cast<byte>(SV_POTION_SPEED)),
        std::make_tuple(ItemKindType::RING, static_cast<byte>(SV_RING_SUSTAIN_INT)),
        std::make_tuple(ItemKindType::SWORD, static_cast<byte>(SV_DAGGER)),
    },

    {
        /* Ninja */
        std::make_tuple(ItemKindType::POTION, static_cast<byte>(SV_POTION_SPEED)),
        std::make_tuple(ItemKindType::SOFT_ARMOR, static_cast<byte>(SV_SOFT_LEATHER_ARMOR)),
        std::make_tuple(ItemKindType::SWORD, static_cast<byte>(SV_DAGGER)),
    },

    {
        /* Sniper */
        std::make_tuple(ItemKindType::BOW, static_cast<byte>(SV_LIGHT_XBOW)),
        std::make_tuple(ItemKindType::SOFT_ARMOR, static_cast<byte>(SV_SOFT_LEATHER_ARMOR)),
        std::make_tuple(ItemKindType::SWORD, static_cast<byte>(SV_DAGGER)),
    },

    {
        /* Elementalist */
        std::make_tuple(ItemKindType::POTION, static_cast<byte>(SV_POTION_SPEED)),
        std::make_tuple(ItemKindType::RING, static_cast<byte>(SV_RING_SUSTAIN_WIS)),
        std::make_tuple(ItemKindType::SWORD, static_cast<byte>(SV_DAGGER)),
    },
};
