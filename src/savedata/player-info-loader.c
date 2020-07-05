#include "savedata/player-info-loader.h"
#include "player/player-skill.h"
#include "savedata/angband-version-comparer.h"
#include "savedata/birth-loader.h"
#include "savedata/load-util.h"
#include "savedata/load-v1-3-0.h"
#include "savedata/load-v1-7-0.h"
#include "savedata/load-zangband.h"

void rd_base_info(player_type *creature_ptr)
{
    rd_string(creature_ptr->name, sizeof(creature_ptr->name));
    rd_string(creature_ptr->died_from, sizeof(creature_ptr->died_from));
    if (!h_older_than(1, 7, 0, 1)) {
        char buf[1024];
        rd_string(buf, sizeof buf);
        if (buf[0])
            creature_ptr->last_message = string_make(buf);
    }

    load_quick_start();
    const int max_history_lines = 4;
    for (int i = 0; i < max_history_lines; i++)
        rd_string(creature_ptr->history[i], sizeof(creature_ptr->history[i]));

    byte tmp8u;
    rd_byte(&tmp8u);
    creature_ptr->prace = (player_race_type)tmp8u;

    rd_byte(&tmp8u);
    creature_ptr->pclass = (player_class_type)tmp8u;

    rd_byte(&tmp8u);
    creature_ptr->pseikaku = (player_personality_type)tmp8u;

    rd_byte(&creature_ptr->psex);
    rd_byte(&tmp8u);
    creature_ptr->realm1 = (REALM_IDX)tmp8u;

    rd_byte(&tmp8u);
    creature_ptr->realm2 = (REALM_IDX)tmp8u;

    rd_byte(&tmp8u);
    if (z_older_than(10, 4, 4))
        set_zangband_realm(creature_ptr);

    rd_byte(&tmp8u);
    creature_ptr->hitdie = (DICE_SID)tmp8u;
    rd_u16b(&creature_ptr->expfact);

    rd_s16b(&creature_ptr->age);
    rd_s16b(&creature_ptr->ht);
    rd_s16b(&creature_ptr->wt);
}

void rd_experience(player_type *creature_ptr)
{
    rd_s32b(&creature_ptr->max_exp);
    if (h_older_than(1, 5, 4, 1))
        creature_ptr->max_max_exp = creature_ptr->max_exp;
    else
        rd_s32b(&creature_ptr->max_max_exp);

    rd_s32b(&creature_ptr->exp);
    if (h_older_than(1, 7, 0, 3))
        set_exp_frac_old(creature_ptr);
    else
        rd_u32b(&creature_ptr->exp_frac);

    rd_s16b(&creature_ptr->lev);
    for (int i = 0; i < 64; i++)
        rd_s16b(&creature_ptr->spell_exp[i]);

    if ((creature_ptr->pclass == CLASS_SORCERER) && z_older_than(10, 4, 2))
        for (int i = 0; i < 64; i++)
            creature_ptr->spell_exp[i] = SPELL_EXP_MASTER;

    const int max_weapon_exp_size = z_older_than(10, 3, 6) ? 60 : 64;
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < max_weapon_exp_size; j++)
            rd_s16b(&creature_ptr->weapon_exp[i][j]);

    for (int i = 0; i < GINOU_MAX; i++)
        rd_s16b(&creature_ptr->skill_exp[i]);
}

static void set_spells(player_type *creature_ptr)
{
    for (int i = 0; i < MAX_SPELLS; i++)
        rd_s32b(&creature_ptr->magic_num1[i]);

    for (int i = 0; i < MAX_SPELLS; i++)
        rd_byte(&creature_ptr->magic_num2[i]);

    if (h_older_than(1, 3, 0, 1))
        set_spells_old(creature_ptr);
}

void rd_skills(player_type *creature_ptr)
{
    if (z_older_than(10, 4, 1))
        set_zangband_skill(creature_ptr);

    if (z_older_than(10, 3, 14))
        set_zangband_spells(creature_ptr);
    else
        set_spells(creature_ptr);

    if (music_singing_any(creature_ptr))
        creature_ptr->action = ACTION_SING;
}

static void set_race(player_type *creature_ptr)
{
    byte tmp8u;
    rd_byte(&tmp8u);
    creature_ptr->start_race = (player_race_type)tmp8u;
    s32b tmp32s;
    rd_s32b(&tmp32s);
    creature_ptr->old_race1 = (BIT_FLAGS)tmp32s;
    rd_s32b(&tmp32s);
    creature_ptr->old_race2 = (BIT_FLAGS)tmp32s;
    rd_s16b(&creature_ptr->old_realm);
}

void rd_race(player_type *creature_ptr)
{
    if (z_older_than(11, 0, 7)) {
        set_zangband_race(creature_ptr);
        return;
    }
    
    set_race(creature_ptr);
}