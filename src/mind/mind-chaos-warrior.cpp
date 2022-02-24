#include "mind/mind-chaos-warrior.h"
#include "floor/floor-object.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-ego.h"
#include "object/object-kind-hook.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"

void acquire_chaos_weapon(PlayerType *player_ptr)
{
    ObjectType forge;
    auto *q_ptr = &forge;
    auto dummy = ItemKindType::SWORD;
    OBJECT_SUBTYPE_VALUE dummy2;
    switch (randint1(player_ptr->lev)) {
    case 0:
    case 1:
        dummy2 = SV_DAGGER;
        break;
    case 2:
    case 3:
        dummy2 = SV_MAIN_GAUCHE;
        break;
    case 4:
        dummy2 = SV_TANTO;
        break;
    case 5:
    case 6:
        dummy2 = SV_RAPIER;
        break;
    case 7:
    case 8:
        dummy2 = SV_SMALL_SWORD;
        break;
    case 9:
    case 10:
        dummy2 = SV_BASILLARD;
        break;
    case 11:
    case 12:
    case 13:
        dummy2 = SV_SHORT_SWORD;
        break;
    case 14:
    case 15:
        dummy2 = SV_SABRE;
        break;
    case 16:
    case 17:
        dummy2 = SV_CUTLASS;
        break;
    case 18:
        dummy2 = SV_WAKIZASHI;
        break;
    case 19:
        dummy2 = SV_KHOPESH;
        break;
    case 20:
        dummy2 = SV_TULWAR;
        break;
    case 21:
        dummy2 = SV_BROAD_SWORD;
        break;
    case 22:
    case 23:
        dummy2 = SV_LONG_SWORD;
        break;
    case 24:
    case 25:
        dummy2 = SV_SCIMITAR;
        break;
    case 26:
        dummy2 = SV_NINJATO;
        break;
    case 27:
        dummy2 = SV_KATANA;
        break;
    case 28:
    case 29:
        dummy2 = SV_BASTARD_SWORD;
        break;
    case 30:
        dummy2 = SV_GREAT_SCIMITAR;
        break;
    case 31:
        dummy2 = SV_CLAYMORE;
        break;
    case 32:
        dummy2 = SV_ESPADON;
        break;
    case 33:
        dummy2 = SV_TWO_HANDED_SWORD;
        break;
    case 34:
        dummy2 = SV_FLAMBERGE;
        break;
    case 35:
        dummy2 = SV_NO_DACHI;
        break;
    case 36:
        dummy2 = SV_EXECUTIONERS_SWORD;
        break;
    case 37:
        dummy2 = SV_ZWEIHANDER;
        break;
    case 38:
        dummy2 = SV_HAYABUSA;
        break;
    default:
        dummy2 = SV_BLADE_OF_CHAOS;
    }

    q_ptr->prep(lookup_kind(dummy, dummy2));
    q_ptr->to_h = 3 + randint1(player_ptr->current_floor_ptr->dun_level) % 10;
    q_ptr->to_d = 3 + randint1(player_ptr->current_floor_ptr->dun_level) % 10;
    one_resistance(q_ptr);
    q_ptr->ego_idx = EgoType::CHAOTIC;
    (void)drop_near(player_ptr, q_ptr, -1, player_ptr->y, player_ptr->x);
}
