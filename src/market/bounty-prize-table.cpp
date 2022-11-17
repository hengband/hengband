#include "market/bounty-prize-table.h"
#include "object/tval-types.h"
#include "sv-definition/sv-potion-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "system/baseitem-info.h"

const std::vector<BaseitemKey> prize_list = {
    { ItemKindType::POTION, SV_POTION_CURING },
    { ItemKindType::POTION, SV_POTION_SPEED },
    { ItemKindType::POTION, SV_POTION_SPEED },
    { ItemKindType::POTION, SV_POTION_RESISTANCE },
    { ItemKindType::POTION, SV_POTION_ENLIGHTENMENT },

    { ItemKindType::POTION, SV_POTION_HEALING },
    { ItemKindType::POTION, SV_POTION_RESTORE_MANA },
    { ItemKindType::SCROLL, SV_SCROLL_STAR_DESTRUCTION },
    { ItemKindType::POTION, SV_POTION_STAR_ENLIGHTENMENT },
    { ItemKindType::SCROLL, SV_SCROLL_SUMMON_PET },

    { ItemKindType::SCROLL, SV_SCROLL_GENOCIDE },
    { ItemKindType::POTION, SV_POTION_STAR_HEALING },
    { ItemKindType::POTION, SV_POTION_STAR_HEALING },
    { ItemKindType::POTION, SV_POTION_NEW_LIFE },
    { ItemKindType::SCROLL, SV_SCROLL_MASS_GENOCIDE },

    { ItemKindType::POTION, SV_POTION_LIFE },
    { ItemKindType::POTION, SV_POTION_LIFE },
    { ItemKindType::POTION, SV_POTION_AUGMENTATION },
    { ItemKindType::POTION, SV_POTION_INVULNERABILITY },
    { ItemKindType::SCROLL, SV_SCROLL_ARTIFACT },
};
