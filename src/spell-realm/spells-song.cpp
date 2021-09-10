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
void check_music(player_type *player_ptr)
{
    if (player_ptr->pclass != CLASS_BARD)
        return;

    auto interupting_song_effect = get_interrupting_song_effect(player_ptr);
    if ((get_singing_song_effect(player_ptr) == 0) && (interupting_song_effect == 0))
        return;

    if (player_ptr->anti_magic) {
        stop_singing(player_ptr);
        return;
    }

    int spell = get_singing_song_id(player_ptr);
    const magic_type *s_ptr;
    s_ptr = &technic_info[REALM_MUSIC - MIN_TECHNIC][spell];

    MANA_POINT need_mana = mod_need_mana(player_ptr, s_ptr->smana, spell, REALM_MUSIC);
    uint32_t need_mana_frac = 0;

    s64b_rshift(&need_mana, &need_mana_frac, 1);
    if (s64b_cmp(player_ptr->csp, player_ptr->csp_frac, need_mana, need_mana_frac) < 0) {
        stop_singing(player_ptr);
        return;
    } else {
        s64b_sub(&(player_ptr->csp), &(player_ptr->csp_frac), need_mana, need_mana_frac);

        player_ptr->redraw |= PR_MANA;
        if (interupting_song_effect != 0) {
            set_singing_song_effect(player_ptr, interupting_song_effect);
            set_interrupting_song_effect(player_ptr, MUSIC_NONE);
            msg_print(_("歌を再開した。", "You resume singing."));
            player_ptr->action = ACTION_SING;
            player_ptr->update |= (PU_BONUS | PU_HP | PU_MONSTERS);
            player_ptr->redraw |= (PR_MAP | PR_STATUS | PR_STATE);
            player_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);
        }
    }

    if (player_ptr->spell_exp[spell] < SPELL_EXP_BEGINNER)
        player_ptr->spell_exp[spell] += 5;
    else if (player_ptr->spell_exp[spell] < SPELL_EXP_SKILLED) {
        if (one_in_(2) && (player_ptr->current_floor_ptr->dun_level > 4) && ((player_ptr->current_floor_ptr->dun_level + 10) > player_ptr->lev))
            player_ptr->spell_exp[spell] += 1;
    } else if (player_ptr->spell_exp[spell] < SPELL_EXP_EXPERT) {
        if (one_in_(5) && ((player_ptr->current_floor_ptr->dun_level + 5) > player_ptr->lev)
            && ((player_ptr->current_floor_ptr->dun_level + 5) > s_ptr->slevel))
            player_ptr->spell_exp[spell] += 1;
    } else if (player_ptr->spell_exp[spell] < SPELL_EXP_MASTER) {
        if (one_in_(5) && ((player_ptr->current_floor_ptr->dun_level + 5) > player_ptr->lev) && (player_ptr->current_floor_ptr->dun_level > s_ptr->slevel))
            player_ptr->spell_exp[spell] += 1;
    }

    exe_spell(player_ptr, REALM_MUSIC, spell, SPELL_CONT);
}

/*!
 * @brief 隠遁の歌の継続時間をセットする / Set "tim_stealth", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_stealth(player_type *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (player_ptr->is_dead)
        return false;

    if (v) {
        if (player_ptr->tim_stealth && !do_dec) {
            if (player_ptr->tim_stealth > v)
                return false;
        } else if (!is_time_limit_stealth(player_ptr)) {
            msg_print(_("足音が小さくなった！", "You begin to walk silently!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_stealth && !music_singing(player_ptr, MUSIC_STEALTH)) {
            msg_print(_("足音が大きくなった。", "You no longer walk silently."));
            notice = true;
        }
    }

    player_ptr->tim_stealth = v;
    player_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return false;

    if (disturb_state)
        disturb(player_ptr, false, false);
    player_ptr->update |= (PU_BONUS);
    handle_stuff(player_ptr);
    return true;
}

/*!
 * @brief 歌の停止を処理する / Stop singing if the player is a Bard
 */
void stop_singing(player_type *player_ptr)
{
    if (player_ptr->pclass != CLASS_BARD)
        return;

    if (get_interrupting_song_effect(player_ptr) != 0) {
        set_interrupting_song_effect(player_ptr, MUSIC_NONE);
        return;
    }

    if (get_singing_song_effect(player_ptr) == 0)
        return;

    if (player_ptr->action == ACTION_SING)
        set_action(player_ptr, ACTION_NONE);

    (void)exe_spell(player_ptr, REALM_MUSIC, get_singing_song_id(player_ptr), SPELL_STOP);
    set_singing_song_effect(player_ptr, MUSIC_NONE);
    set_singing_song_id(player_ptr, 0);
    set_bits(player_ptr->update, PU_BONUS);
    set_bits(player_ptr->redraw, PR_STATUS);
}

bool music_singing(player_type *player_ptr, int music_songs)
{
    return (player_ptr->pclass == CLASS_BARD) && (player_ptr->magic_num1[0] == music_songs);
}

bool music_singing_any(player_type *player_ptr)
{
    return (player_ptr->pclass == CLASS_BARD) && (player_ptr->magic_num1[0] != 0);
}

int32_t get_singing_song_effect(const player_type *player_ptr)
{
    return player_ptr->magic_num1[0];
}

void set_singing_song_effect(player_type *player_ptr, const int32_t magic_num)
{
    player_ptr->magic_num1[0] = magic_num;
}

int32_t get_interrupting_song_effect(const player_type *player_ptr)
{
    return player_ptr->magic_num1[1];
}

void set_interrupting_song_effect(player_type *player_ptr, const int32_t magic_num)
{
    player_ptr->magic_num1[1] = magic_num;
}

int32_t get_singing_count(const player_type *player_ptr)
{
    return player_ptr->magic_num1[2];
}

void set_singing_count(player_type *player_ptr, const int32_t magic_num)
{
    player_ptr->magic_num1[2] = magic_num;
}

byte get_singing_song_id(const player_type *player_ptr)
{
    return player_ptr->magic_num2[0];
}

void set_singing_song_id(player_type *player_ptr, const byte magic_num)
{
    player_ptr->magic_num2[0] = magic_num;
}
