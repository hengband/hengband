#pragma once

#include "system/angband.h"

// @todo TIME_EFFECT型の引数はplayer_typeの時限ステータスをTimedEffectsクラスへ入れる時にshortへ差し替えること.
struct player_type;
class BadStatusSetter {
public:
    BadStatusSetter(player_type *player_ptr);
    virtual ~BadStatusSetter() = default;
    
    bool blindness(TIME_EFFECT v);
    bool confusion(TIME_EFFECT v);
    bool poison(TIME_EFFECT v);
    bool afraidness(TIME_EFFECT v);
    bool paralysis(TIME_EFFECT v);
    bool hallucination(TIME_EFFECT v);
    bool slowness(TIME_EFFECT v, bool do_dec);
    bool stun(TIME_EFFECT v);

private:
    player_type *player_ptr;
};

bool set_cut(player_type *player_ptr, TIME_EFFECT v);
