#include "market/arena-info-table.h"
#include "monster-race/race-indice-types.h"
#include "object/tval-types.h"
#include "sv-definition/sv-amulet-types.h"
#include "sv-definition/sv-food-types.h"
#include "sv-definition/sv-potion-types.h"
#include "sv-definition/sv-ring-types.h"
#include "sv-definition/sv-rod-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "sv-definition/sv-staff-types.h"
#include "sv-definition/sv-wand-types.h"
#include "system/building-type-definition.h"

/*!
 * @brief 闘技場のモンスターID及び報酬アイテムテーブル
 */
const std::vector<arena_type> arena_info = {
    { MON_NOBORTA, ItemPrimaryType::TV_AMULET, SV_AMULET_ADORNMENT },
    { MON_MORI_TROLL, ItemPrimaryType::TV_FOOD, SV_FOOD_PINT_OF_WINE },
    { MON_IMP, ItemPrimaryType::TV_POTION, SV_POTION_SPEED },
    { MON_LION_HEART, ItemPrimaryType::TV_NONE, 0 },
    { MON_MASTER_YEEK, ItemPrimaryType::TV_POTION, SV_POTION_CURING },
    { MON_SABRE_TIGER, ItemPrimaryType::TV_WAND, SV_WAND_STONE_TO_MUD },
    { MON_LIZARD_KING, ItemPrimaryType::TV_WAND, SV_WAND_TELEPORT_AWAY },
    { MON_WYVERN, ItemPrimaryType::TV_POTION, SV_POTION_HEALING },
    { MON_ARCH_VILE, ItemPrimaryType::TV_POTION, SV_POTION_RESISTANCE },
    { MON_ELF_LORD, ItemPrimaryType::TV_POTION, SV_POTION_ENLIGHTENMENT },
    { MON_GHOUL_KING, ItemPrimaryType::TV_FOOD, SV_FOOD_RESTORING },
    { MON_COLBRAN, ItemPrimaryType::TV_RING, SV_RING_ELEC },
    { MON_BICLOPS, ItemPrimaryType::TV_WAND, SV_WAND_ACID_BALL },
    { MON_M_MINDCRAFTER, ItemPrimaryType::TV_POTION, SV_POTION_SELF_KNOWLEDGE },
    { MON_GROO, ItemPrimaryType::TV_SCROLL, SV_SCROLL_ACQUIREMENT },
    { MON_RAAL, ItemPrimaryType::TV_SCROLL, SV_SCROLL_STAR_DESTRUCTION },
    { MON_DREADMASTER, ItemPrimaryType::TV_WAND, SV_WAND_HYPODYNAMIA },
    { MON_ULTRA_PALADIN, ItemPrimaryType::TV_STAFF, SV_STAFF_DISPEL_EVIL },
    { MON_BARNEY, ItemPrimaryType::TV_RING, SV_RING_RES_CHAOS },
    { MON_TROLL_KING, ItemPrimaryType::TV_SCROLL, SV_SCROLL_MASS_GENOCIDE },
    { MON_BARON_HELL, ItemPrimaryType::TV_SCROLL, SV_SCROLL_RUNE_OF_PROTECTION },
    { MON_FALLEN_ANGEL, ItemPrimaryType::TV_POTION, SV_POTION_AUGMENTATION },
    { MON_ANCIENT_CRISTAL_DRAGON, ItemPrimaryType::TV_WAND, SV_WAND_DRAGON_FIRE },
    { MON_BRONZE_LICH, ItemPrimaryType::TV_STAFF, SV_STAFF_DESTRUCTION },
    { MON_DROLEM, ItemPrimaryType::TV_POTION, SV_POTION_STAR_HEALING },
    { MON_G_TITAN, ItemPrimaryType::TV_WAND, SV_WAND_GENOCIDE },
    { MON_G_BALROG, ItemPrimaryType::TV_POTION, SV_POTION_EXPERIENCE },
    { MON_ELDER_VAMPIRE, ItemPrimaryType::TV_RING, SV_RING_SUSTAIN },
    { MON_NIGHTWALKER, ItemPrimaryType::TV_WAND, SV_WAND_STRIKING },
    { MON_S_TYRANNO, ItemPrimaryType::TV_SCROLL, SV_SCROLL_STAR_ACQUIREMENT },
    { MON_MASTER_MYSTIC, ItemPrimaryType::TV_ROD, SV_ROD_IDENTIFY },
    { MON_LORD_CHAOS, ItemPrimaryType::TV_POTION, SV_POTION_LIFE },
    { MON_SHADOWLORD, ItemPrimaryType::TV_POTION, SV_POTION_STAR_ENLIGHTENMENT },
    { MON_ULT_BEHOLDER, ItemPrimaryType::TV_AMULET, SV_AMULET_REFLECTION },
    { MON_JABBERWOCK, ItemPrimaryType::TV_ROD, SV_ROD_HEALING },
    { MON_LOCKE_CLONE, ItemPrimaryType::TV_WAND, SV_WAND_DISINTEGRATE },
    { MON_WYRM_SPACE, ItemPrimaryType::TV_ROD, SV_ROD_RESTORATION },
    { MON_SHAMBLER, ItemPrimaryType::TV_SCROLL, SV_SCROLL_STAR_ACQUIREMENT },
    { MON_BLACK_REAVER, ItemPrimaryType::TV_RING, SV_RING_LORDLY },
    { MON_FENGHUANG, ItemPrimaryType::TV_STAFF, SV_STAFF_THE_MAGI },
    { MON_WYRM_POWER, ItemPrimaryType::TV_SCROLL, SV_SCROLL_ARTIFACT },
    { 0, ItemPrimaryType::TV_NONE, 0 }, /* Victory prizing */
    { MON_HAGURE, ItemPrimaryType::TV_SCROLL, SV_SCROLL_ARTIFACT },
};

const int MAX_ARENA_MONS = arena_info.size() - 2;
