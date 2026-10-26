#pragma once

#include "system/angband.h"

constexpr auto MPE_STAYING = 0x00000001;
constexpr auto MPE_FORGET_FLOW = 0x00000002;
constexpr auto MPE_HANDLE_STUFF = 0x00000004;
constexpr auto MPE_ENERGY_USE = 0x00000008;
constexpr auto MPE_DONT_PICKUP = 0x00000010;
constexpr auto MPE_DO_PICKUP = 0x00000020;
constexpr auto MPE_BREAK_TRAP = 0x00000040;
constexpr auto MPE_DONT_SWAP_MON = 0x00000080;

class PlayerType;
bool move_player_effect(PlayerType *player_ptr, POSITION ny, POSITION nx, BIT_FLAGS mpe_mode);
bool trap_can_be_ignored(PlayerType *player_ptr, FEAT_IDX feat);
void search(PlayerType *player_ptr);
