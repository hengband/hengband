#pragma once

#include "system/angband.h"

/*!
 * @details 仮引数がtmpなのは、全て範囲外の値を範囲内に収める処理が含まれるため
 * @todo TIME_EFFECT型の引数はplayer_typeの時限ステータスをTimedEffectsクラスへ入れる時にshortへ差し替えること.
 */
enum class PlayerCutRank;
enum class PlayerStunRank;
struct player_type;
class BadStatusSetter {
public:
    BadStatusSetter(player_type *player_ptr);
    virtual ~BadStatusSetter() = default;
    
    bool blindness(const TIME_EFFECT tmp_v);
    bool confusion(const TIME_EFFECT tmp_v);
    bool poison(const TIME_EFFECT tmp_v);
    bool afraidness(const TIME_EFFECT tmp_v);
    bool paralysis(const TIME_EFFECT tmp_v);
    bool hallucination(const TIME_EFFECT tmp_v);
    bool slowness(const TIME_EFFECT tmp_v, bool do_dec);
    bool stun(const TIME_EFFECT tmp_v);
    bool cut(const TIME_EFFECT tmp_v);

private:
    player_type *player_ptr;

    bool process_stun_effect(const short v);
    void process_stun_status(const PlayerStunRank new_rank, const short v);
    void clear_head(const PlayerStunRank new_rank);
    void decrease_int_wis(const short v);
    bool process_cut_effect(const short v);
    void decrease_charisma(const PlayerCutRank new_rank, const short v);
    void stop_blooding(const PlayerCutRank new_rank);
};
