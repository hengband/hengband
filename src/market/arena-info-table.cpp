#include "market/arena-info-table.h"
#include "monster-race/monster-race.h"
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
const std::vector<ArenaMonsterEntry> arena_info = {
    { MonsterRaceId::NOBORTA, { ItemKindType::AMULET, SV_AMULET_ADORNMENT } },
    { MonsterRaceId::MORI_TROLL, { ItemKindType::FOOD, SV_FOOD_PINT_OF_WINE } },
    { MonsterRaceId::IMP, { ItemKindType::POTION, SV_POTION_SPEED } },
    { MonsterRaceId::LION_HEART, { ItemKindType::NONE, 0 } },
    { MonsterRaceId::MASTER_YEEK, { ItemKindType::POTION, SV_POTION_CURING } },
    { MonsterRaceId::SABRE_TIGER, { ItemKindType::WAND, SV_WAND_STONE_TO_MUD } },
    { MonsterRaceId::LIZARD_KING, { ItemKindType::WAND, SV_WAND_TELEPORT_AWAY } },
    { MonsterRaceId::WYVERN, { ItemKindType::POTION, SV_POTION_HEALING } },
    { MonsterRaceId::ARCH_VILE, { ItemKindType::POTION, SV_POTION_RESISTANCE } },
    { MonsterRaceId::ELF_LORD, { ItemKindType::POTION, SV_POTION_ENLIGHTENMENT } },
    { MonsterRaceId::GHOUL_KING, { ItemKindType::FOOD, SV_FOOD_RESTORING } },
    { MonsterRaceId::COLBRAN, { ItemKindType::RING, SV_RING_ELEC } },
    { MonsterRaceId::BICLOPS, { ItemKindType::WAND, SV_WAND_ACID_BALL } },
    { MonsterRaceId::M_MINDCRAFTER, { ItemKindType::POTION, SV_POTION_SELF_KNOWLEDGE } },
    { MonsterRaceId::GROO, { ItemKindType::SCROLL, SV_SCROLL_ACQUIREMENT } },
    { MonsterRaceId::RAAL, { ItemKindType::SCROLL, SV_SCROLL_STAR_DESTRUCTION } },
    { MonsterRaceId::DREADMASTER, { ItemKindType::WAND, SV_WAND_HYPODYNAMIA } },
    { MonsterRaceId::ULTRA_PALADIN, { ItemKindType::STAFF, SV_STAFF_DISPEL_EVIL } },
    { MonsterRaceId::BARNEY, { ItemKindType::RING, SV_RING_RES_CHAOS } },
    { MonsterRaceId::TROLL_KING, { ItemKindType::SCROLL, SV_SCROLL_MASS_GENOCIDE } },
    { MonsterRaceId::BARON_HELL, { ItemKindType::SCROLL, SV_SCROLL_RUNE_OF_PROTECTION } },
    { MonsterRaceId::FALLEN_ANGEL, { ItemKindType::POTION, SV_POTION_AUGMENTATION } },
    { MonsterRaceId::ANCIENT_CRISTAL_DRAGON, { ItemKindType::WAND, SV_WAND_DRAGON_FIRE } },
    { MonsterRaceId::BRONZE_LICH, { ItemKindType::STAFF, SV_STAFF_DESTRUCTION } },
    { MonsterRaceId::DROLEM, { ItemKindType::POTION, SV_POTION_STAR_HEALING } },
    { MonsterRaceId::G_TITAN, { ItemKindType::WAND, SV_WAND_GENOCIDE } },
    { MonsterRaceId::G_BALROG, { ItemKindType::POTION, SV_POTION_EXPERIENCE } },
    { MonsterRaceId::ELDER_VAMPIRE, { ItemKindType::RING, SV_RING_SUSTAIN } },
    { MonsterRaceId::NIGHTWALKER, { ItemKindType::WAND, SV_WAND_STRIKING } },
    { MonsterRaceId::S_TYRANNO, { ItemKindType::SCROLL, SV_SCROLL_STAR_ACQUIREMENT } },
    { MonsterRaceId::MASTER_MYSTIC, { ItemKindType::ROD, SV_ROD_IDENTIFY } },
    { MonsterRaceId::LORD_CHAOS, { ItemKindType::POTION, SV_POTION_LIFE } },
    { MonsterRaceId::SHADOWLORD, { ItemKindType::POTION, SV_POTION_STAR_ENLIGHTENMENT } },
    { MonsterRaceId::ULT_BEHOLDER, { ItemKindType::AMULET, SV_AMULET_REFLECTION } },
    { MonsterRaceId::JABBERWOCK, { ItemKindType::ROD, SV_ROD_HEALING } },
    { MonsterRaceId::LOCKE_CLONE, { ItemKindType::WAND, SV_WAND_DISINTEGRATE } },
    { MonsterRaceId::WYRM_SPACE, { ItemKindType::ROD, SV_ROD_RESTORATION } },
    { MonsterRaceId::SHAMBLER, { ItemKindType::SCROLL, SV_SCROLL_STAR_ACQUIREMENT } },
    { MonsterRaceId::BLACK_REAVER, { ItemKindType::RING, SV_RING_LORDLY } },
    { MonsterRaceId::FENGHUANG, { ItemKindType::STAFF, SV_STAFF_THE_MAGI } },
    { MonsterRaceId::WYRM_POWER, { ItemKindType::SCROLL, SV_SCROLL_ARTIFACT } },
    { MonsterRace::empty_id(), { ItemKindType::NONE, 0 } }, /* Victory prizing */
    { MonsterRaceId::HAGURE, { ItemKindType::SCROLL, SV_SCROLL_ARTIFACT } },
};

const int MAX_ARENA_MONS = arena_info.size() - 2;
