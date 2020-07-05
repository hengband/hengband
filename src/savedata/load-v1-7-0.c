#include "savedata/load-v1-7-0.h"
#include "savedata/load-util.h"

void set_hp_old(player_type *creature_ptr)
{
    s16b tmp16s;
    rd_s16b(&tmp16s);
    creature_ptr->mhp = tmp16s;

    rd_s16b(&tmp16s);
    creature_ptr->chp = tmp16s;

    u16b tmp16u;
    rd_u16b(&tmp16u);
    creature_ptr->chp_frac = (u32b)tmp16u;
}

void set_mana_old(player_type *creature_ptr)
{
    s16b tmp16s;
    rd_s16b(&tmp16s);
    creature_ptr->msp = tmp16s;

    rd_s16b(&tmp16s);
    creature_ptr->csp = tmp16s;

    u16b tmp16u;
    rd_u16b(&tmp16u);
    creature_ptr->csp_frac = (u32b)tmp16u;
}

void set_exp_frac_old(player_type *creature_ptr)
{
    u16b tmp16u;
    rd_u16b(&tmp16u);
    creature_ptr->exp_frac = (u32b)tmp16u;
}
