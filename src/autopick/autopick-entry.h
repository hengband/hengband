#pragma once

bool autopick_new_entry(autopick_type *entry, concptr str, bool allow_default);
void autopick_entry_from_object(player_type *player_ptr, autopick_type *entry, object_type *o_ptr);
