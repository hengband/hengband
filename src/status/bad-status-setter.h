#pragma once

#include "system/angband.h"
#include <memory>

/*!
 * @details 仮引数がtmpなのは、全て範囲外の値を範囲内に収める処理が含まれるため
 * @todo TIME_EFFECT型の引数はPlayerTypeの時限ステータスをTimedEffectsクラスへ入れる時にshortへ差し替えること.
 */
enum class PlayerCutRank;
enum class PlayerStunRank;
class PlayerConfusion;
class PlayerType;
class BadStatusSetter {
public:
    BadStatusSetter(PlayerType *player_ptr);
    virtual ~BadStatusSetter() = default;

    bool set_blindness(const TIME_EFFECT tmp_v);
    bool mod_blindness(const TIME_EFFECT tmp_v);
    bool set_confusion(const TIME_EFFECT tmp_v);
    bool mod_confusion(const TIME_EFFECT tmp_v);
    bool set_poison(const TIME_EFFECT tmp_v);
    bool mod_poison(const TIME_EFFECT tmp_v);
    bool set_fear(const TIME_EFFECT tmp_v);
    bool mod_fear(const TIME_EFFECT tmp_v);
    bool set_paralysis(const TIME_EFFECT tmp_v);
    bool mod_paralysis(const TIME_EFFECT tmp_v);
    bool hallucination(const TIME_EFFECT tmp_v);
    bool mod_hallucination(const TIME_EFFECT tmp_v);
    bool set_deceleration(const TIME_EFFECT tmp_v, bool do_dec);
    bool mod_deceleration(const TIME_EFFECT tmp_v, bool do_dec);
    bool set_stun(const TIME_EFFECT tmp_v);
    bool mod_stun(const TIME_EFFECT tmp_v);
    bool set_cut(const TIME_EFFECT tmp_v);
    bool mod_cut(const TIME_EFFECT tmp_v);

private:
    PlayerType *player_ptr;
    std::shared_ptr<PlayerConfusion> player_confusion;

    bool process_stun_effect(const short v);
    void process_stun_status(const PlayerStunRank new_rank, const short v);
    void clear_head();
    void decrease_int_wis(const short v);
    bool process_cut_effect(const short v);
    void decrease_charisma(const PlayerCutRank new_rank, const short v);
    void stop_blooding(const PlayerCutRank new_rank);
};
