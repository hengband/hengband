#pragma once

#include <span>
#include <tl/optional.hpp>

class PlayerType;
void do_cmd_hissatsu(PlayerType *player_ptr);
void do_cmd_gain_hissatsu(PlayerType *player_ptr);
tl::optional<int> hissatsu_spell_id_of_choice(std::span<const int> selections, bool is_menu, int choice);
