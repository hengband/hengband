#include "mind/racial-android.h"
#include "io/targeting.h"
#include "spell-kind/spells-launcher.h"
#include "spell/spell-types.h"

bool android_inside_weapon(player_type *creature_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(creature_ptr, &dir))
        return FALSE;

    if (creature_ptr->lev < 10) {
        msg_print(_("レイガンを発射した。", "You fire your ray gun."));
        fire_bolt(creature_ptr, GF_MISSILE, dir, (creature_ptr->lev + 1) / 2);
        return TRUE;
    }

    if (creature_ptr->lev < 25) {
        msg_print(_("ブラスターを発射した。", "You fire your blaster."));
        fire_bolt(creature_ptr, GF_MISSILE, dir, creature_ptr->lev);
        return TRUE;
    }

    if (creature_ptr->lev < 35) {
        msg_print(_("バズーカを発射した。", "You fire your bazooka."));
        fire_ball(creature_ptr, GF_MISSILE, dir, creature_ptr->lev * 2, 2);
        return TRUE;
    }

    if (creature_ptr->lev < 45) {
        msg_print(_("ビームキャノンを発射した。", "You fire a beam cannon."));
        fire_beam(creature_ptr, GF_MISSILE, dir, creature_ptr->lev * 2);
        return TRUE;
    }

    msg_print(_("ロケットを発射した。", "You fire a rocket."));
    fire_rocket(creature_ptr, GF_ROCKET, dir, creature_ptr->lev * 5, 2);
    return TRUE;
}
