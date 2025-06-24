#include "mind/mind-chaos-warrior.h"
#include "floor/floor-object.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-ego.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include <span>

void acquire_chaos_weapon(PlayerType *player_ptr)
{
    constexpr static auto weapons = {
        SV_DAGGER,
        SV_DAGGER,
        SV_MAIN_GAUCHE,
        SV_MAIN_GAUCHE,
        SV_TANTO,
        SV_RAPIER,
        SV_RAPIER,
        SV_SMALL_SWORD,
        SV_SMALL_SWORD,
        SV_BASILLARD, // LV10
        SV_BASILLARD,
        SV_SHORT_SWORD,
        SV_SHORT_SWORD,
        SV_SHORT_SWORD,
        SV_SABRE,
        SV_SABRE,
        SV_CUTLASS,
        SV_CUTLASS,
        SV_WAKIZASHI,
        SV_KHOPESH, // LV20
        SV_TULWAR,
        SV_BROAD_SWORD,
        SV_LONG_SWORD,
        SV_LONG_SWORD,
        SV_SCIMITAR,
        SV_SCIMITAR,
        SV_NINJATO,
        SV_KATANA,
        SV_BASTARD_SWORD,
        SV_BASTARD_SWORD, // LV30
        SV_GREAT_SCIMITAR,
        SV_CLAYMORE,
        SV_ESPADON,
        SV_TWO_HANDED_SWORD,
        SV_FLAMBERGE,
        SV_NO_DACHI,
        SV_EXECUTIONERS_SWORD,
        SV_ZWEIHANDER,
        SV_HAYABUSA,
        SV_BLADE_OF_CHAOS, // LV40
        SV_BLADE_OF_CHAOS,
        SV_BLADE_OF_CHAOS,
        SV_BLADE_OF_CHAOS,
        SV_BLADE_OF_CHAOS,
        SV_BLADE_OF_CHAOS,
        SV_BLADE_OF_CHAOS,
        SV_BLADE_OF_CHAOS,
        SV_BLADE_OF_CHAOS,
        SV_BLADE_OF_CHAOS,
        SV_BLADE_OF_CHAOS, // LV50
    };

    const auto candidates = std::span(weapons).first(player_ptr->lev);
    const auto sval = rand_choice(candidates);

    ItemEntity item({ ItemKindType::SWORD, sval });
    item.to_h = 3 + randint1(player_ptr->current_floor_ptr->dun_level) % 10;
    item.to_d = 3 + randint1(player_ptr->current_floor_ptr->dun_level) % 10;
    one_resistance(&item);
    item.ego_idx = EgoType::CHAOTIC;
    (void)drop_near(player_ptr, item, player_ptr->get_position());
}
