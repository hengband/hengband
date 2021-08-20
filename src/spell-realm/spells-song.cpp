#include "spell-realm/spells-song.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/disturbance-options.h"
#include "player/attack-defense-types.h"
#include "player/player-skill.h"
#include "player/player-status.h"
#include "realm/realm-song-numbers.h"
#include "spell/spell-info.h"
#include "spell/spells-execution.h"
#include "spell/technic-info-table.h"
#include "status/action-setter.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief プレイヤーの歌に関する継続処理
 */
void check_music(player_type *caster_ptr)
{
    if (caster_ptr->pclass != CLASS_BARD)
        return;

    auto interupting_song_effect = get_interrupting_song_effect(caster_ptr);
    if ((get_singing_song_effect(caster_ptr) == 0) && (interupting_song_effect == 0))
        return;

    if (caster_ptr->anti_magic) {
        stop_singing(caster_ptr);
        return;
    }

    int spell = get_singing_song_id(caster_ptr);
    const magic_type *s_ptr;
    s_ptr = &technic_info[REALM_MUSIC - MIN_TECHNIC][spell];

    MANA_POINT need_mana = mod_need_mana(caster_ptr, s_ptr->smana, spell, REALM_MUSIC);
    uint32_t need_mana_frac = 0;

    s64b_rshift(&need_mana, &need_mana_frac, 1);
    if (s64b_cmp(caster_ptr->csp, caster_ptr->csp_frac, need_mana, need_mana_frac) < 0) {
        stop_singing(caster_ptr);
        return;
    } else {
        s64b_sub(&(caster_ptr->csp), &(caster_ptr->csp_frac), need_mana, need_mana_frac);

        caster_ptr->redraw |= PR_MANA;
        if (interupting_song_effect != 0) {
            set_singing_song_effect(caster_ptr, interupting_song_effect);
            set_interrupting_song_effect(caster_ptr, MUSIC_NONE);
            msg_print(_("歌を再開した。", "You resume singing."));
            caster_ptr->action = ACTION_SING;
            caster_ptr->update |= (PU_BONUS | PU_HP | PU_MONSTERS);
            caster_ptr->redraw |= (PR_MAP | PR_STATUS | PR_STATE);
            caster_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);
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
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return false;

    if (v) {
        if (creature_ptr->tim_stealth && !do_dec) {
            if (creature_ptr->tim_stealth > v)
                return false;
        } else if (!is_time_limit_stealth(creature_ptr)) {
            msg_print(_("足音が小さくなった！", "You begin to walk silently!"));
            notice = true;
        }
    } else {
        if (creature_ptr->tim_stealth && !music_singing(creature_ptr, MUSIC_STEALTH)) {
            msg_print(_("足音が大きくなった。", "You no longer walk silently."));
            notice = true;
        }
    }

    creature_ptr->tim_stealth = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return false;

    if (disturb_state)
        disturb(creature_ptr, false, false);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
    return true;
}

/*!
 * @brief 歌の停止を処理する / Stop singing if the player is a Bard
 */
void stop_singing(player_type *creature_ptr)
{
    if (creature_ptr->pclass != CLASS_BARD)
        return;

    if (get_interrupting_song_effect(creature_ptr) != 0) {
        set_interrupting_song_effect(creature_ptr, MUSIC_NONE);
        return;
    }

    if (get_singing_song_effect(creature_ptr) == 0)
        return;

    if (creature_ptr->action == ACTION_SING)
        set_action(creature_ptr, ACTION_NONE);

    (void)exe_spell(creature_ptr, REALM_MUSIC, get_singing_song_id(creature_ptr), SPELL_STOP);
    set_singing_song_effect(creature_ptr, MUSIC_NONE);
    set_singing_song_id(creature_ptr, 0);
    set_bits(creature_ptr->update, PU_BONUS);
    set_bits(creature_ptr->redraw, PR_STATUS);
}

bool music_singing(player_type *caster_ptr, int music_songs)
{
    return (caster_ptr->pclass == CLASS_BARD) && (caster_ptr->magic_num1[0] == music_songs);
}

bool music_singing_any(player_type *creature_ptr)
{
    return (creature_ptr->pclass == CLASS_BARD) && (creature_ptr->magic_num1[0] != 0);
}

int32_t get_singing_song_effect(const player_type *creature_ptr)
{
    return creature_ptr->magic_num1[0];
}

void set_singing_song_effect(player_type *creature_ptr, const int32_t magic_num)
{
    creature_ptr->magic_num1[0] = magic_num;
}

int32_t get_interrupting_song_effect(const player_type *creature_ptr)
{
    return creature_ptr->magic_num1[1];
}

void set_interrupting_song_effect(player_type *creature_ptr, const int32_t magic_num)
{
    creature_ptr->magic_num1[1] = magic_num;
}

int32_t get_singing_count(const player_type *creature_ptr)
{
    return creature_ptr->magic_num1[2];
}

void set_singing_count(player_type *creature_ptr, const int32_t magic_num)
{
    creature_ptr->magic_num1[2] = magic_num;
}

byte get_singing_song_id(const player_type *creature_ptr)
{
    return creature_ptr->magic_num2[0];
}

void set_singing_song_id(player_type *creature_ptr, const byte magic_num)
{
    creature_ptr->magic_num2[0] = magic_num;
}
