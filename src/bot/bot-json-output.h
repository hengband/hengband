#pragma once

class PlayerType;
enum class StoreSaleType;

void output_bot_json_snapshot(PlayerType *player_ptr);
void output_bot_json_store_snapshot(PlayerType *player_ptr, StoreSaleType store_num);
