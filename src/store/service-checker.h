#pragma once

struct object_type;
class PlayerType;
bool store_will_buy(PlayerType *player_ptr, const object_type *o_ptr);
void mass_produce(PlayerType *player_ptr, object_type *o_ptr);
