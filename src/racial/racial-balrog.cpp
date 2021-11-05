﻿#include "racial/racial-balrog.h"
#include "player/player-status.h"
#include "spell-kind/spells-launcher.h"
#include "spell/spell-types.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool demonic_breath(PlayerType *player_ptr)
{
    DIRECTION dir;
    int type = (one_in_(2) ? GF_NETHER : GF_FIRE);
    if (!get_aim_dir(player_ptr, &dir))
        return false;
    stop_mouth(player_ptr);
    msg_format(_("あなたは%sのブレスを吐いた。", "You breathe %s."), ((type == GF_NETHER) ? _("地獄", "nether") : _("火炎", "fire")));
    fire_breath(player_ptr, type, dir, player_ptr->lev * 3, (player_ptr->lev / 15) + 1);
    return true;
}
