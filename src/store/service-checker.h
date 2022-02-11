#pragma once

class ObjectType;
class PlayerType;
bool store_will_buy(PlayerType *player_ptr, const ObjectType *o_ptr);
void mass_produce(PlayerType *player_ptr, ObjectType *o_ptr);
