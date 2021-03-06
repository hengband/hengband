#include "player/player-race.h"
#include "core/player-redraw-types.h"
#include "inventory/inventory-slot-types.h"
#include "player/player-race-types.h"
#include "system/object-type-definition.h"
#include "mimic-info-table.h"
#include "player/race-info-table.h"

const player_race *rp_ptr;

SYMBOL_CODE get_summon_symbol_from_player(player_type *creature_ptr)
{
    SYMBOL_CODE symbol = 'N';
    switch (creature_ptr->mimic_form) {
    case MIMIC_NONE:
        switch (creature_ptr->prace) {
        case RACE_HUMAN:
        case RACE_AMBERITE:
        case RACE_BARBARIAN:
        case RACE_BEASTMAN:
        case RACE_DUNADAN:
            symbol = 'p';
            break;
        case RACE_HALF_ELF:
        case RACE_ELF:
        case RACE_HOBBIT:
        case RACE_GNOME:
        case RACE_DWARF:
        case RACE_HIGH_ELF:
        case RACE_NIBELUNG:
        case RACE_DARK_ELF:
        case RACE_MIND_FLAYER:
        case RACE_KUTAR:
        case RACE_S_FAIRY:
            symbol = 'h';
            break;
        case RACE_HALF_ORC:
            symbol = 'o';
            break;
        case RACE_HALF_TROLL:
            symbol = 'T';
            break;
        case RACE_HALF_OGRE:
            symbol = 'O';
            break;
        case RACE_HALF_GIANT:
        case RACE_HALF_TITAN:
        case RACE_CYCLOPS:
            symbol = 'P';
            break;
        case RACE_YEEK:
            symbol = 'y';
            break;
        case RACE_KLACKON:
            symbol = 'K';
            break;
        case RACE_KOBOLD:
            symbol = 'k';
            break;
        case RACE_IMP:
            if (one_in_(13))
                symbol = 'U';
            else
                symbol = 'u';
            break;
        case RACE_DRACONIAN:
            symbol = 'd';
            break;
        case RACE_GOLEM:
        case RACE_ANDROID:
            symbol = 'g';
            break;
        case RACE_SKELETON:
            if (one_in_(13))
                symbol = 'L';
            else
                symbol = 's';
            break;
        case RACE_ZOMBIE:
            symbol = 'z';
            break;
        case RACE_VAMPIRE:
            symbol = 'V';
            break;
        case RACE_SPECTRE:
            symbol = 'G';
            break;
        case RACE_SPRITE:
            symbol = 'I';
            break;
        case RACE_ENT:
            symbol = '#';
            break;
        case RACE_ARCHON:
            symbol = 'A';
            break;
        case RACE_BALROG:
            symbol = 'U';
            break;
        default:
            symbol = 'p';
            break;
        }
        break;
    case MIMIC_DEMON:
        if (one_in_(13))
            symbol = 'U';
        else
            symbol = 'u';
        break;
    case MIMIC_DEMON_LORD:
        symbol = 'U';
        break;
    case MIMIC_VAMPIRE:
        symbol = 'V';
        break;
    }
    return symbol;
}

bool is_specific_player_race(player_type *creature_ptr, player_race_type prace) { return (!creature_ptr->mimic_form && (creature_ptr->prace == prace)); }
