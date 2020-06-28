#include "player/player-realm.h"
#include "object/tval-types.h"

REALM_IDX get_realm1_book(player_type* player_ptr) { return player_ptr->realm1 + TV_LIFE_BOOK - 1; }

REALM_IDX get_realm2_book(player_type *player_ptr) { return player_ptr->realm2 + TV_LIFE_BOOK - 1; }
