#include "specific-object/blade-turner.h"
#include "core/hp-mp-processor.h"
#include "spell-kind/spells-launcher.h"
#include "spell/spell-types.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool activate_bladeturner(player_type *user_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    msg_print(_("あなたはエレメントのブレスを吐いた。", "You breathe the elements."));
    fire_breath(user_ptr, GF_MISSILE, dir, 300, 4);
    msg_print(_("鎧が様々な色に輝いた...", "Your armor glows many colours..."));
    (void)set_afraid(user_ptr, 0);
    (void)set_hero(user_ptr, randint1(50) + 50, FALSE);
    (void)hp_player(user_ptr, 10);
    (void)set_blessed(user_ptr, randint1(50) + 50, FALSE);
    (void)set_oppose_acid(user_ptr, randint1(50) + 50, FALSE);
    (void)set_oppose_elec(user_ptr, randint1(50) + 50, FALSE);
    (void)set_oppose_fire(user_ptr, randint1(50) + 50, FALSE);
    (void)set_oppose_cold(user_ptr, randint1(50) + 50, FALSE);
    (void)set_oppose_pois(user_ptr, randint1(50) + 50, FALSE);
    return TRUE;
}
