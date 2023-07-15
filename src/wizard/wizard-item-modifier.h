#pragma once

#include "system/angband.h"

enum class WishResultType { FAIL = -1,
    NOTHING = 0,
    NORMAL = 1,
    EGO = 2,
    ARTIFACT = 3,
    MAX };

class PlayerType;
void wizard_item_modifier(PlayerType *player_ptr);
void wiz_modify_item(PlayerType *player_ptr);
WishResultType do_cmd_wishing(PlayerType *player_ptr, int prob, bool art_ok, bool ego_ok, bool confirm);
void wiz_identify_full_inventory(PlayerType *player_ptr);
