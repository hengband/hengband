#pragma once

typedef struct object_type object_type;
typedef struct player_type player_type;
bool store_will_buy(player_type *player_ptr, const object_type *o_ptr);
void mass_produce(player_type *player_ptr, object_type *o_ptr);
