#pragma once

struct object_type;
struct player_type;
enum class StoreSaleType;
int home_carry(player_type *player_ptr, object_type *o_ptr);
bool combine_and_reorder_home(player_type *player_ptr, const StoreSaleType store_num);
