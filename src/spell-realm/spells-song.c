#include "spell-realm/spells-song.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/disturbance-options.h"
#include "player/attack-defense-types.h"
#include "player/player-skill.h"
#include "realm/realm-song-numbers.h"
#include "spell/spell-info.h"
#include "spell/spells-execution.h"
#include "spell/technic-info-table.h"
#include "system/floor-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief プレイヤーの歌に関する継続処理
 * @return なし
 */
void check_music(player_type *caster_ptr)
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
    const magic_type *s_ptr;
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
        if (one_in_(5) && ((caster_ptr->current_floor_ptr->dun_level + 5) > caster_ptr->lev)
            && ((caster_ptr->current_floor_ptr->dun_level + 5) > s_ptr->slevel))
            caster_ptr->spell_exp[spell] += 1;
    } else if (caster_ptr->spell_exp[spell] < SPELL_EXP_MASTER) {
        if (one_in_(5) && ((caster_ptr->current_floor_ptr->dun_level + 5) > caster_ptr->lev) && (caster_ptr->current_floor_ptr->dun_level > s_ptr->slevel))
            caster_ptr->spell_exp[spell] += 1;
    }

    exe_spell(caster_ptr, REALM_MUSIC, spell, SPELL_CONT);
}

/*!
 * @brief 隠遁の歌の継続時間をセットする / Set "tim_stealth", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_stealth(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->tim_stealth && !do_dec) {
            if (creature_ptr->tim_stealth > v)
                return FALSE;
        } else if (!is_time_limit_stealth(creature_ptr)) {
            msg_print(_("足音が小さくなった！", "You begin to walk silently!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->tim_stealth && !music_singing(creature_ptr, MUSIC_STEALTH)) {
            msg_print(_("足音が大きくなった。", "You no longer walk silently."));
            notice = TRUE;
        }
    }

    creature_ptr->tim_stealth = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
    return TRUE;
}
