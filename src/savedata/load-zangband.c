#include "savedata/load-zangband.h"
#include "dungeon/dungeon.h"
#include "game-option/option-flags.h"
#include "market/bounty.h"
#include "monster-race/monster-race.h"
#include "player/avatar.h"
#include "player/patron.h"
#include "player/player-skill.h"
#include "realm/realm-types.h"
#include "savedata/load-util.h"
#include "world/world.h"

void load_zangband_options(void)
{
    if (option_flag[5] & (0x00000001 << 4))
        option_flag[5] &= ~(0x00000001 << 4);
    else
        option_flag[5] |= (0x00000001 << 4);

    if (option_flag[2] & (0x00000001 << 5))
        option_flag[2] &= ~(0x00000001 << 5);
    else
        option_flag[2] |= (0x00000001 << 5);

    if (option_flag[4] & (0x00000001 << 5))
        option_flag[4] &= ~(0x00000001 << 5);
    else
        option_flag[4] |= (0x00000001 << 5);

    if (option_flag[5] & (0x00000001 << 0))
        option_flag[5] &= ~(0x00000001 << 0);
    else
        option_flag[5] |= (0x00000001 << 0);

    if (option_flag[5] & (0x00000001 << 12))
        option_flag[5] &= ~(0x00000001 << 12);
    else
        option_flag[5] |= (0x00000001 << 12);

    if (option_flag[1] & (0x00000001 << 0))
        option_flag[1] &= ~(0x00000001 << 0);
    else
        option_flag[1] |= (0x00000001 << 0);

    if (option_flag[1] & (0x00000001 << 18))
        option_flag[1] &= ~(0x00000001 << 18);
    else
        option_flag[1] |= (0x00000001 << 18);

    if (option_flag[1] & (0x00000001 << 19))
        option_flag[1] &= ~(0x00000001 << 19);
    else
        option_flag[1] |= (0x00000001 << 19);

    if (option_flag[5] & (0x00000001 << 3))
        option_flag[1] &= ~(0x00000001 << 3);
    else
        option_flag[5] |= (0x00000001 << 3);
}

void set_zangband_realm(player_type* creature_ptr)
{
    if (creature_ptr->realm1 == 9)
        creature_ptr->realm1 = REALM_MUSIC;

    if (creature_ptr->realm2 == 9)
        creature_ptr->realm2 = REALM_MUSIC;

    if (creature_ptr->realm1 == 10)
        creature_ptr->realm1 = REALM_HISSATSU;

    if (creature_ptr->realm2 == 10)
        creature_ptr->realm2 = REALM_HISSATSU;
}

void set_zangband_skill(player_type *creature_ptr)
{
    if (creature_ptr->pclass != CLASS_BEASTMASTER)
        creature_ptr->skill_exp[GINOU_RIDING] /= 2;

    creature_ptr->skill_exp[GINOU_RIDING] = MIN(creature_ptr->skill_exp[GINOU_RIDING], s_info[creature_ptr->pclass].s_max[GINOU_RIDING]);
}

void set_zangband_spells(player_type* creature_ptr)
{
    for (int i = 0; i < MAX_SPELLS; i++)
        creature_ptr->magic_num1[i] = 0;

    for (int i = 0; i < MAX_SPELLS; i++)
        creature_ptr->magic_num2[i] = 0;
}

void set_zangband_race(player_type *creature_ptr)
{
    creature_ptr->start_race = creature_ptr->prace;
    creature_ptr->old_race1 = 0L;
    creature_ptr->old_race2 = 0L;
    creature_ptr->old_realm = 0;
}

void set_zangband_bounty_uniques(player_type *creature_ptr)
{
    determine_bounty_uniques(creature_ptr);
    for (int i = 0; i < MAX_BOUNTY; i++) {
        /* Is this bounty unique already dead? */
        if (!r_info[current_world_ptr->bounty_r_idx[i]].max_num)
            current_world_ptr->bounty_r_idx[i] += 10000;
    }
}

/*!
 * @brief ZAngband v2.0.6に存在しない時限効果を0で初期化する / Old savefiles do not have the following fields...
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details 厳密にv2.0.6しか見ていないため、ZAngband v2.0.5 以前のセーブデータは非対応
 */
void set_zangband_timed_effects(player_type *creature_ptr)
{
    creature_ptr->tim_esp = 0;
    creature_ptr->wraith_form = 0;
    creature_ptr->resist_magic = 0;
    creature_ptr->tim_regen = 0;
    creature_ptr->tim_pass_wall = 0;
    creature_ptr->tim_stealth = 0;
    creature_ptr->tim_levitation = 0;
    creature_ptr->tim_sh_touki = 0;
    creature_ptr->lightspeed = 0;
    creature_ptr->tsubureru = 0;
    creature_ptr->tim_res_nether = 0;
    creature_ptr->tim_res_time = 0;
    creature_ptr->mimic_form = 0;
    creature_ptr->tim_mimic = 0;
    creature_ptr->tim_sh_fire = 0;
    creature_ptr->tim_reflect = 0;
    creature_ptr->multishadow = 0;
    creature_ptr->dustrobe = 0;
    creature_ptr->chaos_patron = ((creature_ptr->age + creature_ptr->sc) % MAX_PATRON);
    creature_ptr->muta1 = 0;
    creature_ptr->muta2 = 0;
    creature_ptr->muta3 = 0;
    get_virtues(creature_ptr);
}

void set_zangband_mimic(player_type *creature_ptr)
{
    creature_ptr->tim_res_time = 0;
    creature_ptr->mimic_form = 0;
    creature_ptr->tim_mimic = 0;
    creature_ptr->tim_sh_fire = 0;
}

void set_zangband_holy_aura(player_type *creature_ptr)
{
    creature_ptr->tim_sh_holy = 0;
    creature_ptr->tim_eyeeye = 0;
}

void set_zangband_reflection(player_type *creature_ptr)
{
    creature_ptr->tim_reflect = 0;
    creature_ptr->multishadow = 0;
    creature_ptr->dustrobe = 0;
}

void rd_zangband_dungeon()
{
    s16b tmp16s;
    rd_s16b(&tmp16s);
    max_dlv[DUNGEON_ANGBAND] = tmp16s;
}

void set_zangband_game_turns(player_type *creature_ptr)
{
    creature_ptr->current_floor_ptr->generated_turn /= 2;
    creature_ptr->feeling_turn /= 2;
    current_world_ptr->game_turn /= 2;
    current_world_ptr->dungeon_turn /= 2;
}
