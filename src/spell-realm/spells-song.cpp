#include "spell-realm/spells-song.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/disturbance-options.h"
#include "player-base/player-class.h"
#include "player-info/bard-data-type.h"
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
    if (player_ptr->pclass != PlayerClassType::BARD)
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

    PlayerSkill(player_ptr).gain_continuous_spell_skill_exp(REALM_MUSIC, spell);

    exe_spell(player_ptr, REALM_MUSIC, spell, SpellProcessType::CONTNUATION);
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
    if (player_ptr->pclass != PlayerClassType::BARD)
        return;

    if (get_interrupting_song_effect(player_ptr) != 0) {
        set_interrupting_song_effect(player_ptr, MUSIC_NONE);
        return;
    }

    if (get_singing_song_effect(player_ptr) == 0)
        return;

    if (player_ptr->action == ACTION_SING)
        set_action(player_ptr, ACTION_NONE);

    (void)exe_spell(player_ptr, REALM_MUSIC, get_singing_song_id(player_ptr), SpellProcessType::STOP);
    set_singing_song_effect(player_ptr, MUSIC_NONE);
    set_singing_song_id(player_ptr, 0);
    set_bits(player_ptr->update, PU_BONUS);
    set_bits(player_ptr->redraw, PR_STATUS);
}

bool music_singing(player_type *player_ptr, int music_songs)
{
    auto bird_data = PlayerClass(player_ptr).get_specific_data<bard_data_type>();
    return bird_data && (bird_data->singing_song == music_songs);
}

bool music_singing_any(player_type *player_ptr)
{
    auto bird_data = PlayerClass(player_ptr).get_specific_data<bard_data_type>();
    return bird_data && (bird_data->singing_song != MUSIC_NONE);
}

int32_t get_singing_song_effect(player_type *player_ptr)
{
    auto bird_data = PlayerClass(player_ptr).get_specific_data<bard_data_type>();
    if (!bird_data) {
        return 0;
    }

    return bird_data->singing_song;
}

void set_singing_song_effect(player_type *player_ptr, const int32_t magic_num)
{
    auto bird_data = PlayerClass(player_ptr).get_specific_data<bard_data_type>();
    if (!bird_data) {
        return;
    }

    bird_data->singing_song = i2enum<realm_song_type>(magic_num);
}

int32_t get_interrupting_song_effect(player_type *player_ptr)
{
    auto bird_data = PlayerClass(player_ptr).get_specific_data<bard_data_type>();
    if (!bird_data) {
        return 0;
    }

    return bird_data->interrputing_song;
}

void set_interrupting_song_effect(player_type *player_ptr, const int32_t magic_num)
{
    auto bird_data = PlayerClass(player_ptr).get_specific_data<bard_data_type>();
    if (!bird_data) {
        return;
    }

    bird_data->interrputing_song = i2enum<realm_song_type>(magic_num);
}

int32_t get_singing_count(player_type *player_ptr)
{
    auto bird_data = PlayerClass(player_ptr).get_specific_data<bard_data_type>();
    if (!bird_data) {
        return 0;
    }

    return bird_data->singing_duration;
}

void set_singing_count(player_type *player_ptr, const int32_t magic_num)
{
    auto bird_data = PlayerClass(player_ptr).get_specific_data<bard_data_type>();
    if (!bird_data) {
        return;
    }

    bird_data->singing_duration = magic_num;
}

byte get_singing_song_id(player_type *player_ptr)
{
    auto bird_data = PlayerClass(player_ptr).get_specific_data<bard_data_type>();
    if (!bird_data) {
        return 0;
    }

    return bird_data->singing_song_spell_idx;
}

void set_singing_song_id(player_type *player_ptr, const byte magic_num)
{
    auto bird_data = PlayerClass(player_ptr).get_specific_data<bard_data_type>();
    if (!bird_data) {
        return;
    }

    bird_data->singing_song_spell_idx = magic_num;
}
