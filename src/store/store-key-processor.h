#pragma once

extern bool leave_store;

class CapturedMonsterType;
class PlayerType;
void store_process_command(PlayerType *player_ptr, CapturedMonsterType *cap_mon_ptr);
