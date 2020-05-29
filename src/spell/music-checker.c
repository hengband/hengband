#include "system/angband.h"
#include "floor/floor.h"
#include "spell/music-checker.h"
#include "realm/realm-song.h"
#include "spell/spells3.h"
#include "player/player-skill.h"
#include "spell/spells-execution.h"

/*!
 * @brief プレイヤーの歌に関する継続処理
 * @return なし
 */
void check_music(player_type* caster_ptr)
{
    if (caster_ptr->pclass != CLASS_BARD)
        return;
    if (!SINGING_SONG_EFFECT(caster_ptr) && !INTERUPTING_SONG_EFFECT(caster_ptr))
        return;

    if (caster_ptr->anti_magic) {
        stop_singing(caster_ptr);
        return;
    }

    int spell = SINGING_SONG_ID(caster_ptr);
    const magic_type* s_ptr;
    s_ptr = &technic_info[REALM_MUSIC - MIN_TECHNIC][spell];

    MANA_POINT need_mana = mod_need_mana(caster_ptr, s_ptr->smana, spell, REALM_MUSIC);
    u32b need_mana_frac = 0;

    s64b_RSHIFT(need_mana, need_mana_frac, 1);
    if (s64b_cmp(caster_ptr->csp, caster_ptr->csp_frac, need_mana, need_mana_frac) < 0) {
        stop_singing(caster_ptr);
        return;
    } else {
        s64b_sub(&(caster_ptr->csp), &(caster_ptr->csp_frac), need_mana, need_mana_frac);

        caster_ptr->redraw |= PR_MANA;
        if (INTERUPTING_SONG_EFFECT(caster_ptr)) {
            SINGING_SONG_EFFECT(caster_ptr) = INTERUPTING_SONG_EFFECT(caster_ptr);
            INTERUPTING_SONG_EFFECT(caster_ptr) = MUSIC_NONE;
            msg_print(_("歌を再開した。", "You restart singing."));
            caster_ptr->action = ACTION_SING;
            caster_ptr->update |= (PU_BONUS | PU_HP | PU_MONSTERS);
            caster_ptr->redraw |= (PR_MAP | PR_STATUS | PR_STATE);
            caster_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
        }
    }

    if (caster_ptr->spell_exp[spell] < SPELL_EXP_BEGINNER)
        caster_ptr->spell_exp[spell] += 5;
    else if (caster_ptr->spell_exp[spell] < SPELL_EXP_SKILLED) {
        if (one_in_(2) && (caster_ptr->current_floor_ptr->dun_level > 4) && ((caster_ptr->current_floor_ptr->dun_level + 10) > caster_ptr->lev))
            caster_ptr->spell_exp[spell] += 1;
    } else if (caster_ptr->spell_exp[spell] < SPELL_EXP_EXPERT) {
        if (one_in_(5) && ((caster_ptr->current_floor_ptr->dun_level + 5) > caster_ptr->lev) && ((caster_ptr->current_floor_ptr->dun_level + 5) > s_ptr->slevel))
            caster_ptr->spell_exp[spell] += 1;
    } else if (caster_ptr->spell_exp[spell] < SPELL_EXP_MASTER) {
        if (one_in_(5) && ((caster_ptr->current_floor_ptr->dun_level + 5) > caster_ptr->lev) && (caster_ptr->current_floor_ptr->dun_level > s_ptr->slevel))
            caster_ptr->spell_exp[spell] += 1;
    }

    exe_spell(caster_ptr, REALM_MUSIC, spell, SPELL_CONT);
}
