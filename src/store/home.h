#pragma once

struct object_type;
class PlayerType;
enum class StoreSaleType;
int home_carry(PlayerType *player_ptr, object_type *o_ptr);
bool combine_and_reorder_home(PlayerType *player_ptr, const StoreSaleType store_num);
