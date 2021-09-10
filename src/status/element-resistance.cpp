#include "status/element-resistance.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/stuff-handler.h"
#include "game-option/disturbance-options.h"
#include "player-base/player-race.h"
#include "player-info/race-info.h"
#include "player/special-defense-types.h"
#include "realm/realm-song-numbers.h"
#include "spell-realm/spells-song.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 一時的酸耐性の継続時間をセットする / Set "oppose_acid", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_oppose_acid(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;
    if (creature_ptr->is_dead)
        return false;

    if (v) {
        if (creature_ptr->oppose_acid && !do_dec) {
            if (creature_ptr->oppose_acid > v)
                return false;
        } else if (!is_oppose_acid(creature_ptr)) {
            msg_print(_("酸への耐性がついた気がする！", "You feel resistant to acid!"));
            notice = true;
        }
    } else {
        if (creature_ptr->oppose_acid && !music_singing(creature_ptr, MUSIC_RESIST) && !(creature_ptr->special_defense & KATA_MUSOU)) {
            msg_print(_("酸への耐性が薄れた気がする。", "You feel less resistant to acid."));
            notice = true;
        }
    }

    creature_ptr->oppose_acid = v;

    if (!notice)
        return false;
    creature_ptr->redraw |= (PR_STATUS);

    if (disturb_state)
        disturb(creature_ptr, false, false);
    handle_stuff(creature_ptr);
    return true;
}

/*!
 * @brief 一時的電撃耐性の継続時間をセットする / Set "oppose_elec", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_oppose_elec(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return false;

    if (v) {
        if (creature_ptr->oppose_elec && !do_dec) {
            if (creature_ptr->oppose_elec > v)
                return false;
        } else if (!is_oppose_elec(creature_ptr)) {
            msg_print(_("電撃への耐性がついた気がする！", "You feel resistant to electricity!"));
            notice = true;
        }
    } else {
        if (creature_ptr->oppose_elec && !music_singing(creature_ptr, MUSIC_RESIST) && !(creature_ptr->special_defense & KATA_MUSOU)) {
            msg_print(_("電撃への耐性が薄れた気がする。", "You feel less resistant to electricity."));
            notice = true;
        }
    }

    creature_ptr->oppose_elec = v;

    if (!notice)
        return false;
    creature_ptr->redraw |= (PR_STATUS);

    if (disturb_state)
        disturb(creature_ptr, false, false);
    handle_stuff(creature_ptr);
    return true;
}

/*!
 * @brief 一時的火炎耐性の継続時間をセットする / Set "oppose_fire", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_oppose_fire(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;
    if (creature_ptr->is_dead)
        return false;

    if ((PlayerRace(creature_ptr).equals(player_race_type::BALROG) && (creature_ptr->lev > 44)) || (creature_ptr->mimic_form == MIMIC_DEMON))
        v = 1;
    if (v) {
        if (creature_ptr->oppose_fire && !do_dec) {
            if (creature_ptr->oppose_fire > v)
                return false;
        } else if (!is_oppose_fire(creature_ptr)) {
            msg_print(_("火への耐性がついた気がする！", "You feel resistant to fire!"));
            notice = true;
        }
    } else {
        if (creature_ptr->oppose_fire && !music_singing(creature_ptr, MUSIC_RESIST) && !(creature_ptr->special_defense & KATA_MUSOU)) {
            msg_print(_("火への耐性が薄れた気がする。", "You feel less resistant to fire."));
            notice = true;
        }
    }

    creature_ptr->oppose_fire = v;

    if (!notice)
        return false;
    creature_ptr->redraw |= (PR_STATUS);

    if (disturb_state)
        disturb(creature_ptr, false, false);
    handle_stuff(creature_ptr);
    return true;
}

/*!
 * @brief 一時的冷気耐性の継続時間をセットする / Set "oppose_cold", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_oppose_cold(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;
    if (creature_ptr->is_dead)
        return false;

    if (v) {
        if (creature_ptr->oppose_cold && !do_dec) {
            if (creature_ptr->oppose_cold > v)
                return false;
        } else if (!is_oppose_cold(creature_ptr)) {
            msg_print(_("冷気への耐性がついた気がする！", "You feel resistant to cold!"));
            notice = true;
        }
    } else {
        if (creature_ptr->oppose_cold && !music_singing(creature_ptr, MUSIC_RESIST) && !(creature_ptr->special_defense & KATA_MUSOU)) {
            msg_print(_("冷気への耐性が薄れた気がする。", "You feel less resistant to cold."));
            notice = true;
        }
    }

    creature_ptr->oppose_cold = v;

    if (!notice)
        return false;
    creature_ptr->redraw |= (PR_STATUS);

    if (disturb_state)
        disturb(creature_ptr, false, false);
    handle_stuff(creature_ptr);
    return true;
}

/*!
 * @brief 一時的毒耐性の継続時間をセットする / Set "oppose_pois", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_oppose_pois(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;
    if ((creature_ptr->pclass == CLASS_NINJA) && (creature_ptr->lev > 44))
        v = 1;
    if (creature_ptr->is_dead)
        return false;

    if (v) {
        if (creature_ptr->oppose_pois && !do_dec) {
            if (creature_ptr->oppose_pois > v)
                return false;
        } else if (!is_oppose_pois(creature_ptr)) {
            msg_print(_("毒への耐性がついた気がする！", "You feel resistant to poison!"));
            notice = true;
        }
    } else {
        if (creature_ptr->oppose_pois && !music_singing(creature_ptr, MUSIC_RESIST) && !(creature_ptr->special_defense & KATA_MUSOU)) {
            msg_print(_("毒への耐性が薄れた気がする。", "You feel less resistant to poison."));
            notice = true;
        }
    }

    creature_ptr->oppose_pois = v;
    if (!notice)
        return false;
    creature_ptr->redraw |= (PR_STATUS);

    if (disturb_state)
        disturb(creature_ptr, false, false);
    handle_stuff(creature_ptr);
    return true;
}

bool is_oppose_acid(player_type *creature_ptr)
{
    return creature_ptr->oppose_acid || music_singing(creature_ptr, MUSIC_RESIST) || (creature_ptr->special_defense & KATA_MUSOU);
}

bool is_oppose_elec(player_type *creature_ptr)
{
    return creature_ptr->oppose_elec || music_singing(creature_ptr, MUSIC_RESIST) || (creature_ptr->special_defense & KATA_MUSOU);
}

bool is_oppose_fire(player_type *creature_ptr)
{
    return creature_ptr->oppose_fire || music_singing(creature_ptr, MUSIC_RESIST)
        || (creature_ptr->special_defense & KATA_MUSOU || (creature_ptr->mimic_form == MIMIC_DEMON)
            || (PlayerRace(creature_ptr).equals(player_race_type::BALROG) && creature_ptr->lev > 44));
}

bool is_oppose_cold(player_type *creature_ptr)
{
    return creature_ptr->oppose_cold || music_singing(creature_ptr, MUSIC_RESIST) || (creature_ptr->special_defense & KATA_MUSOU);
}

bool is_oppose_pois(player_type *creature_ptr)
{
    return creature_ptr->oppose_pois || music_singing(creature_ptr, MUSIC_RESIST)
        || (creature_ptr->special_defense & KATA_MUSOU || (creature_ptr->pclass == CLASS_NINJA && creature_ptr->lev > 44));
}
