#pragma once

#include "system/angband.h"

/*!
 * @details 仮引数がtmpなのは、全て範囲外の値を範囲内に収める処理が含まれるため
 * @todo TIME_EFFECT型の引数はplayer_typeの時限ステータスをTimedEffectsクラスへ入れる時にshortへ差し替えること.
 */
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
};
