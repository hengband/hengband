#pragma once

#include "system/angband.h"

// @todo TIME_EFFECT型の引数はplayer_typeの時限ステータスをTimedEffectsクラスへ入れる時にshortへ差し替えること.
struct player_type;
class BadStatusSetter {
public:
    BadStatusSetter(player_type *player_ptr);
    virtual ~BadStatusSetter() = default;
    
    bool blindness(TIME_EFFECT v);

private:
    player_type *player_ptr;
};

bool set_confused(player_type *player_ptr, TIME_EFFECT v);
bool set_poisoned(player_type *player_ptr, TIME_EFFECT v);
bool set_afraid(player_type *player_ptr, TIME_EFFECT v);
bool set_paralyzed(player_type *player_ptr, TIME_EFFECT v);
bool set_image(player_type *player_ptr, TIME_EFFECT v);
bool set_slow(player_type *player_ptr, TIME_EFFECT v, bool do_dec);
bool set_stun(player_type *player_ptr, TIME_EFFECT v);
bool set_cut(player_type *player_ptr, TIME_EFFECT v);
