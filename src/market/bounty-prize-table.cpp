#include "market/bounty-prize-table.h"
#include "object/tval-types.h"
#include "sv-definition/sv-potion-types.h"
#include "sv-definition/sv-scroll-types.h"

bounty_prize_type prize_list[MAX_BOUNTY] = {
    { ItemPrimaryType::TV_POTION, SV_POTION_CURING },
    { ItemPrimaryType::TV_POTION, SV_POTION_SPEED },
    { ItemPrimaryType::TV_POTION, SV_POTION_SPEED },
    { ItemPrimaryType::TV_POTION, SV_POTION_RESISTANCE },
    { ItemPrimaryType::TV_POTION, SV_POTION_ENLIGHTENMENT },

    { ItemPrimaryType::TV_POTION, SV_POTION_HEALING },
    { ItemPrimaryType::TV_POTION, SV_POTION_RESTORE_MANA },
    { ItemPrimaryType::TV_SCROLL, SV_SCROLL_STAR_DESTRUCTION },
    { ItemPrimaryType::TV_POTION, SV_POTION_STAR_ENLIGHTENMENT },
    { ItemPrimaryType::TV_SCROLL, SV_SCROLL_SUMMON_PET },

    { ItemPrimaryType::TV_SCROLL, SV_SCROLL_GENOCIDE },
    { ItemPrimaryType::TV_POTION, SV_POTION_STAR_HEALING },
    { ItemPrimaryType::TV_POTION, SV_POTION_STAR_HEALING },
    { ItemPrimaryType::TV_POTION, SV_POTION_NEW_LIFE },
    { ItemPrimaryType::TV_SCROLL, SV_SCROLL_MASS_GENOCIDE },

    { ItemPrimaryType::TV_POTION, SV_POTION_LIFE },
    { ItemPrimaryType::TV_POTION, SV_POTION_LIFE },
    { ItemPrimaryType::TV_POTION, SV_POTION_AUGMENTATION },
    { ItemPrimaryType::TV_POTION, SV_POTION_INVULNERABILITY },
    { ItemPrimaryType::TV_SCROLL, SV_SCROLL_ARTIFACT },
};
