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
    { MON_NOBORTA, ItemKindType::AMULET, SV_AMULET_ADORNMENT },
    { MON_MORI_TROLL, ItemKindType::FOOD, SV_FOOD_PINT_OF_WINE },
    { MON_IMP, ItemKindType::POTION, SV_POTION_SPEED },
    { MON_LION_HEART, ItemKindType::NONE, 0 },
    { MON_MASTER_YEEK, ItemKindType::POTION, SV_POTION_CURING },
    { MON_SABRE_TIGER, ItemKindType::WAND, SV_WAND_STONE_TO_MUD },
    { MON_LIZARD_KING, ItemKindType::WAND, SV_WAND_TELEPORT_AWAY },
    { MON_WYVERN, ItemKindType::POTION, SV_POTION_HEALING },
    { MON_ARCH_VILE, ItemKindType::POTION, SV_POTION_RESISTANCE },
    { MON_ELF_LORD, ItemKindType::POTION, SV_POTION_ENLIGHTENMENT },
    { MON_GHOUL_KING, ItemKindType::FOOD, SV_FOOD_RESTORING },
    { MON_COLBRAN, ItemKindType::RING, SV_RING_ELEC },
    { MON_BICLOPS, ItemKindType::WAND, SV_WAND_ACID_BALL },
    { MON_M_MINDCRAFTER, ItemKindType::POTION, SV_POTION_SELF_KNOWLEDGE },
    { MON_GROO, ItemKindType::SCROLL, SV_SCROLL_ACQUIREMENT },
    { MON_RAAL, ItemKindType::SCROLL, SV_SCROLL_STAR_DESTRUCTION },
    { MON_DREADMASTER, ItemKindType::WAND, SV_WAND_HYPODYNAMIA },
    { MON_ULTRA_PALADIN, ItemKindType::STAFF, SV_STAFF_DISPEL_EVIL },
    { MON_BARNEY, ItemKindType::RING, SV_RING_RES_CHAOS },
    { MON_TROLL_KING, ItemKindType::SCROLL, SV_SCROLL_MASS_GENOCIDE },
    { MON_BARON_HELL, ItemKindType::SCROLL, SV_SCROLL_RUNE_OF_PROTECTION },
    { MON_FALLEN_ANGEL, ItemKindType::POTION, SV_POTION_AUGMENTATION },
    { MON_ANCIENT_CRISTAL_DRAGON, ItemKindType::WAND, SV_WAND_DRAGON_FIRE },
    { MON_BRONZE_LICH, ItemKindType::STAFF, SV_STAFF_DESTRUCTION },
    { MON_DROLEM, ItemKindType::POTION, SV_POTION_STAR_HEALING },
    { MON_G_TITAN, ItemKindType::WAND, SV_WAND_GENOCIDE },
    { MON_G_BALROG, ItemKindType::POTION, SV_POTION_EXPERIENCE },
    { MON_ELDER_VAMPIRE, ItemKindType::RING, SV_RING_SUSTAIN },
    { MON_NIGHTWALKER, ItemKindType::WAND, SV_WAND_STRIKING },
    { MON_S_TYRANNO, ItemKindType::SCROLL, SV_SCROLL_STAR_ACQUIREMENT },
    { MON_MASTER_MYSTIC, ItemKindType::ROD, SV_ROD_IDENTIFY },
    { MON_LORD_CHAOS, ItemKindType::POTION, SV_POTION_LIFE },
    { MON_SHADOWLORD, ItemKindType::POTION, SV_POTION_STAR_ENLIGHTENMENT },
    { MON_ULT_BEHOLDER, ItemKindType::AMULET, SV_AMULET_REFLECTION },
    { MON_JABBERWOCK, ItemKindType::ROD, SV_ROD_HEALING },
    { MON_LOCKE_CLONE, ItemKindType::WAND, SV_WAND_DISINTEGRATE },
    { MON_WYRM_SPACE, ItemKindType::ROD, SV_ROD_RESTORATION },
    { MON_SHAMBLER, ItemKindType::SCROLL, SV_SCROLL_STAR_ACQUIREMENT },
    { MON_BLACK_REAVER, ItemKindType::RING, SV_RING_LORDLY },
    { MON_FENGHUANG, ItemKindType::STAFF, SV_STAFF_THE_MAGI },
    { MON_WYRM_POWER, ItemKindType::SCROLL, SV_SCROLL_ARTIFACT },
    { 0, ItemKindType::NONE, 0 }, /* Victory prizing */
    { MON_HAGURE, ItemKindType::SCROLL, SV_SCROLL_ARTIFACT },
};

const int MAX_ARENA_MONS = arena_info.size() - 2;
