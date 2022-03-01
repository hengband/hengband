#include "specific-object/blade-turner.h"
#include "effect/attribute-types.h"
#include "hpmp/hp-mp-processor.h"
#include "spell-kind/spells-launcher.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool activate_bladeturner(PlayerType *player_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    msg_print(_("あなたはエレメントのブレスを吐いた。", "You breathe the elements."));
    fire_breath(player_ptr, AttributeType::MISSILE, dir, 300, 4);
    msg_print(_("鎧が様々な色に輝いた...", "Your armor glows many colours..."));
    (void)BadStatusSetter(player_ptr).fear(0);
    (void)set_hero(player_ptr, randint1(50) + 50, false);
    (void)hp_player(player_ptr, 10);
    (void)set_blessed(player_ptr, randint1(50) + 50, false);
    (void)set_oppose_acid(player_ptr, randint1(50) + 50, false);
    (void)set_oppose_elec(player_ptr, randint1(50) + 50, false);
    (void)set_oppose_fire(player_ptr, randint1(50) + 50, false);
    (void)set_oppose_cold(player_ptr, randint1(50) + 50, false);
    (void)set_oppose_pois(player_ptr, randint1(50) + 50, false);
    return true;
}
