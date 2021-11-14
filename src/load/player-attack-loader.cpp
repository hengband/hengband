#include "load/player-attack-loader.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "load/load-zangband.h"
#include "player-base/player-class.h"
#include "player-info/monk-data-type.h"
#include "player-info/samurai-data-type.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "system/player-type-definition.h"

void rd_special_attack(PlayerType *player_ptr)
{
    if (h_older_than(0, 0, 9)) {
        set_zangband_special_attack(player_ptr);
        return;
    }

    player_ptr->ele_attack = rd_s16b();
    player_ptr->special_attack = rd_u32b();
}

void rd_special_action(PlayerType *player_ptr)
{
    if (!PlayerClass(player_ptr).monk_stance_is(MonkStanceType::NONE)) {
        player_ptr->action = ACTION_MONK_STANCE;
        return;
    }

    if (!PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::NONE)) {
        player_ptr->action = ACTION_SAMURAI_STANCE;
    }
}

void rd_special_defense(PlayerType *player_ptr)
{
    if (h_older_than(0, 0, 12)) {
        set_zangband_special_defense(player_ptr);
        return;
    }

    player_ptr->ele_immune = rd_s16b();
    player_ptr->special_defense = rd_u32b();
}

void rd_action(PlayerType *player_ptr)
{
    strip_bytes(1);
    player_ptr->action = rd_byte();
    if (!h_older_than(0, 4, 3)) {
        set_zangband_action(player_ptr);
    }
}
