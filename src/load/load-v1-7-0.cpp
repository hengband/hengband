#include "load/load-v1-7-0.h"
#include "dungeon/dungeon.h"
#include "game-option/birth-options.h"
#include "load/load-util.h"
#include "load/load-v1-5-0.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"

void set_hp_old(player_type *creature_ptr)
{
    int16_t tmp16s;
    rd_s16b(&tmp16s);
    creature_ptr->mhp = tmp16s;

    rd_s16b(&tmp16s);
    creature_ptr->chp = tmp16s;

    uint16_t tmp16u;
    rd_u16b(&tmp16u);
    creature_ptr->chp_frac = (uint32_t)tmp16u;
}

void set_mana_old(player_type *creature_ptr)
{
    int16_t tmp16s;
    rd_s16b(&tmp16s);
    creature_ptr->msp = tmp16s;

    rd_s16b(&tmp16s);
    creature_ptr->csp = tmp16s;

    uint16_t tmp16u;
    rd_u16b(&tmp16u);
    creature_ptr->csp_frac = (uint32_t)tmp16u;
}

void set_exp_frac_old(player_type *creature_ptr)
{
    uint16_t tmp16u;
    rd_u16b(&tmp16u);
    creature_ptr->exp_frac = (uint32_t)tmp16u;
}

void remove_water_cave(player_type* creature_ptr)
{
    if (creature_ptr->current_floor_ptr->inside_quest != OLD_QUEST_WATER_CAVE)
        return;

    creature_ptr->dungeon_idx = lite_town ? DUNGEON_ANGBAND : DUNGEON_GALGALS;
    creature_ptr->current_floor_ptr->dun_level = 1;
    creature_ptr->current_floor_ptr->inside_quest = 0;
}
