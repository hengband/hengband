#pragma once

class ObjectType;
class PlayerType;
enum class StoreSaleType;
int home_carry(PlayerType *player_ptr, ObjectType *o_ptr, StoreSaleType store_num);
bool combine_and_reorder_home(PlayerType *player_ptr, const StoreSaleType store_num);
