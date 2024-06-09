#include "market/arena-entry.h"
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
#include "system/baseitem-info.h"
#include "system/monster-race-info.h"

namespace {
/*!
 * @brief 闘技場のモンスターエントリー構造体
 */
class ArenaMonsterEntry {
public:
    ArenaMonsterEntry(MonsterRaceId r_idx, const BaseitemKey &key)
        : monrace_id(r_idx)
        , key(key)
    {
    }

    MonsterRaceId monrace_id; /*!< 闘技場のモンスター種族ID(0ならば表彰式) / Monster (0 means victory prizing) */
    BaseitemKey key;
};

/*!
 * @brief 闘技場のモンスターID及び報酬アイテムテーブル
 */
const std::vector<ArenaMonsterEntry> ARENA_ENTRIES = {
    { MonsterRaceId::NOBORTA, { ItemKindType::AMULET, SV_AMULET_ADORNMENT } },
    { MonsterRaceId::MORI_TROLL, { ItemKindType::FOOD, SV_FOOD_PINT_OF_WINE } },
    { MonsterRaceId::IMP, { ItemKindType::POTION, SV_POTION_SPEED } },
    { MonsterRaceId::LION_HEART, { ItemKindType::ROD, SV_ROD_DETECT_TRAP } },
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
    { MonraceList::empty_id(), { ItemKindType::NONE, 0 } }, /* Victory prizing */
    { MonsterRaceId::HAGURE, { ItemKindType::SCROLL, SV_SCROLL_ARTIFACT } },
};
}

ArenaEntryList ArenaEntryList::instance{};

ArenaEntryList &ArenaEntryList::get_instance()
{
    return instance;
}

/*
 * @brief 表彰式までの最大数を返す.
 */
int ArenaEntryList::get_max_entries() const
{
    return std::ssize(ARENA_ENTRIES) - 2;
}

int ArenaEntryList::get_current_entry() const
{
    return this->current_entry;
}

std::optional<int> ArenaEntryList::get_defeated_entry() const
{
    return this->defeated_entry;
}

bool ArenaEntryList::is_player_victor() const
{
    return this->current_entry == this->get_max_entries();
}

bool ArenaEntryList::is_player_true_victor() const
{
    return this->current_entry > this->get_max_entries();
}

const BaseitemKey &ArenaEntryList::get_bi_key() const
{
    return ARENA_ENTRIES.at(this->current_entry).key;
}

MonsterRaceInfo &ArenaEntryList::get_monrace()
{
    return MonraceList::get_instance().get_monrace(ARENA_ENTRIES.at(this->current_entry).monrace_id);
}

const MonsterRaceInfo &ArenaEntryList::get_monrace() const
{
    return MonraceList::get_instance().get_monrace(ARENA_ENTRIES.at(this->current_entry).monrace_id);
}

void ArenaEntryList::increment_entry()
{
    this->current_entry++;
}

void ArenaEntryList::reset_entry()
{
    this->current_entry = 0;
    this->defeated_entry = std::nullopt;
}

void ArenaEntryList::set_defeated_entry()
{
    this->defeated_entry = this->current_entry;
}

void ArenaEntryList::load_current_entry(int entry)
{
    this->current_entry = entry;
}

void ArenaEntryList::load_defeated_entry(int entry)
{
    if (entry < 0) {
        this->defeated_entry = std::nullopt;
        return;
    }

    this->defeated_entry = entry;
}
