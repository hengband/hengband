#pragma once

#include "system/angband.h"

struct autopick_type;
class ObjectType;
class PlayerType;
bool autopick_new_entry(autopick_type *entry, concptr str, bool allow_default);
void autopick_entry_from_object(PlayerType *player_ptr, autopick_type *entry, ObjectType *o_ptr);
concptr autopick_line_from_entry(autopick_type *entry);
concptr autopick_line_from_entry_kill(autopick_type *entry);
bool entry_from_choosed_object(PlayerType *player_ptr, autopick_type *entry);
