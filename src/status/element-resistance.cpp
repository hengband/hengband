#include "status/element-resistance.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/stuff-handler.h"
#include "game-option/disturbance-options.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/race-info.h"
#include "player-info/samurai-data-type.h"
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
bool set_oppose_acid(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;
    if (player_ptr->is_dead)
        return false;

    if (v) {
        if (player_ptr->oppose_acid && !do_dec) {
            if (player_ptr->oppose_acid > v)
                return false;
        } else if (!is_oppose_acid(player_ptr)) {
            msg_print(_("酸への耐性がついた気がする！", "You feel resistant to acid!"));
            notice = true;
        }
    } else {
        if (player_ptr->oppose_acid && !music_singing(player_ptr, MUSIC_RESIST) &&
            !PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::MUSOU)) {
            msg_print(_("酸への耐性が薄れた気がする。", "You feel less resistant to acid."));
            notice = true;
        }
    }

    player_ptr->oppose_acid = v;

    if (!notice)
        return false;
    player_ptr->redraw |= (PR_STATUS);

    if (disturb_state)
        disturb(player_ptr, false, false);
    handle_stuff(player_ptr);
    return true;
}

/*!
 * @brief 一時的電撃耐性の継続時間をセットする / Set "oppose_elec", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_oppose_elec(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (player_ptr->is_dead)
        return false;

    if (v) {
        if (player_ptr->oppose_elec && !do_dec) {
            if (player_ptr->oppose_elec > v)
                return false;
        } else if (!is_oppose_elec(player_ptr)) {
            msg_print(_("電撃への耐性がついた気がする！", "You feel resistant to electricity!"));
            notice = true;
        }
    } else {
        if (player_ptr->oppose_elec && !music_singing(player_ptr, MUSIC_RESIST) &&
            !PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::MUSOU)) {
            msg_print(_("電撃への耐性が薄れた気がする。", "You feel less resistant to electricity."));
            notice = true;
        }
    }

    player_ptr->oppose_elec = v;

    if (!notice)
        return false;
    player_ptr->redraw |= (PR_STATUS);

    if (disturb_state)
        disturb(player_ptr, false, false);
    handle_stuff(player_ptr);
    return true;
}

/*!
 * @brief 一時的火炎耐性の継続時間をセットする / Set "oppose_fire", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_oppose_fire(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;
    if (player_ptr->is_dead)
        return false;

    if ((PlayerRace(player_ptr).equals(PlayerRaceType::BALROG) && (player_ptr->lev > 44)) || (player_ptr->mimic_form == MIMIC_DEMON))
        v = 1;
    if (v) {
        if (player_ptr->oppose_fire && !do_dec) {
            if (player_ptr->oppose_fire > v)
                return false;
        } else if (!is_oppose_fire(player_ptr)) {
            msg_print(_("火への耐性がついた気がする！", "You feel resistant to fire!"));
            notice = true;
        }
    } else {
        if (player_ptr->oppose_fire && !music_singing(player_ptr, MUSIC_RESIST) &&
            !PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::MUSOU)) {
            msg_print(_("火への耐性が薄れた気がする。", "You feel less resistant to fire."));
            notice = true;
        }
    }

    player_ptr->oppose_fire = v;

    if (!notice)
        return false;
    player_ptr->redraw |= (PR_STATUS);

    if (disturb_state)
        disturb(player_ptr, false, false);
    handle_stuff(player_ptr);
    return true;
}

/*!
 * @brief 一時的冷気耐性の継続時間をセットする / Set "oppose_cold", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_oppose_cold(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;
    if (player_ptr->is_dead)
        return false;

    if (v) {
        if (player_ptr->oppose_cold && !do_dec) {
            if (player_ptr->oppose_cold > v)
                return false;
        } else if (!is_oppose_cold(player_ptr)) {
            msg_print(_("冷気への耐性がついた気がする！", "You feel resistant to cold!"));
            notice = true;
        }
    } else {
        if (player_ptr->oppose_cold && !music_singing(player_ptr, MUSIC_RESIST) &&
            !PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::MUSOU)) {
            msg_print(_("冷気への耐性が薄れた気がする。", "You feel less resistant to cold."));
            notice = true;
        }
    }

    player_ptr->oppose_cold = v;

    if (!notice)
        return false;
    player_ptr->redraw |= (PR_STATUS);

    if (disturb_state)
        disturb(player_ptr, false, false);
    handle_stuff(player_ptr);
    return true;
}

/*!
 * @brief 一時的毒耐性の継続時間をセットする / Set "oppose_pois", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_oppose_pois(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;
    if ((player_ptr->pclass == PlayerClassType::NINJA) && (player_ptr->lev > 44))
        v = 1;
    if (player_ptr->is_dead)
        return false;

    if (v) {
        if (player_ptr->oppose_pois && !do_dec) {
            if (player_ptr->oppose_pois > v)
                return false;
        } else if (!is_oppose_pois(player_ptr)) {
            msg_print(_("毒への耐性がついた気がする！", "You feel resistant to poison!"));
            notice = true;
        }
    } else {
        if (player_ptr->oppose_pois && !music_singing(player_ptr, MUSIC_RESIST) &&
            !PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::MUSOU)) {
            msg_print(_("毒への耐性が薄れた気がする。", "You feel less resistant to poison."));
            notice = true;
        }
    }

    player_ptr->oppose_pois = v;
    if (!notice)
        return false;
    player_ptr->redraw |= (PR_STATUS);

    if (disturb_state)
        disturb(player_ptr, false, false);
    handle_stuff(player_ptr);
    return true;
}

bool is_oppose_acid(PlayerType *player_ptr)
{
    return player_ptr->oppose_acid || music_singing(player_ptr, MUSIC_RESIST) || PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::MUSOU);
}

bool is_oppose_elec(PlayerType *player_ptr)
{
    return player_ptr->oppose_elec || music_singing(player_ptr, MUSIC_RESIST) || PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::MUSOU);
}

bool is_oppose_fire(PlayerType *player_ptr)
{
    return player_ptr->oppose_fire || music_singing(player_ptr, MUSIC_RESIST)
        || (PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::MUSOU) || (player_ptr->mimic_form == MIMIC_DEMON)
            || (PlayerRace(player_ptr).equals(PlayerRaceType::BALROG) && player_ptr->lev > 44));
}

bool is_oppose_cold(PlayerType *player_ptr)
{
    return player_ptr->oppose_cold || music_singing(player_ptr, MUSIC_RESIST) || PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::MUSOU);
}

bool is_oppose_pois(PlayerType *player_ptr)
{
    return player_ptr->oppose_pois || music_singing(player_ptr, MUSIC_RESIST)
        || (PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::MUSOU) || (player_ptr->pclass == PlayerClassType::NINJA && player_ptr->lev > 44));
}
