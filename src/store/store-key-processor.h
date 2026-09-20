#pragma once

extern bool leave_store;

class PlayerType;
class Store;
void store_process_command(PlayerType *player_ptr, Store &store);
