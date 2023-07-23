#pragma once

extern bool leave_store;

enum class StoreSaleType;
class PlayerType;
void store_process_command(PlayerType *player_ptr, StoreSaleType store_num);
