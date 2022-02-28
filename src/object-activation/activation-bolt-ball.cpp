#include "object-activation/activation-bolt-ball.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "floor/floor-util.h"
#include "grid/feature-flag-types.h"
#include "hpmp/hp-mp-processor.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-specific-bolt.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool activate_missile_1(PlayerType *player_ptr)
{
    DIRECTION dir;
    msg_print(_("それは眩しいくらいに明るく輝いている...", "It glows extremely brightly..."));
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_bolt(player_ptr, AttributeType::MISSILE, dir, damroll(2, 6));
    return true;
}

bool activate_missile_2(PlayerType *player_ptr)
{
    DIRECTION dir;
    msg_print(_("魔法のトゲが現れた...", "It grows magical spikes..."));
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_bolt(player_ptr, AttributeType::MISSILE, dir, 150);
    return true;
}

bool activate_missile_3(PlayerType *player_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    msg_print(_("あなたはエレメントのブレスを吐いた。", "You breathe the elements."));
    fire_breath(player_ptr, AttributeType::MISSILE, dir, 300, 4);
    return true;
}

bool activate_bolt_acid_1(PlayerType *player_ptr)
{
    DIRECTION dir;
    msg_print(_("それは酸に覆われた...", "It is covered in acid..."));
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_bolt(player_ptr, AttributeType::ACID, dir, damroll(5, 8));
    return true;
}

bool activate_bolt_elec_1(PlayerType *player_ptr)
{
    DIRECTION dir;
    msg_print(_("それは火花に覆われた...", "It is covered in sparks..."));
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_bolt(player_ptr, AttributeType::ELEC, dir, damroll(4, 8));
    return true;
}

bool activate_bolt_fire_1(PlayerType *player_ptr)
{
    DIRECTION dir;
    msg_print(_("それは炎に覆われた...", "It is covered in fire..."));
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_bolt(player_ptr, AttributeType::FIRE, dir, damroll(9, 8));
    return true;
}

bool activate_bolt_cold_1(PlayerType *player_ptr)
{
    DIRECTION dir;
    msg_print(_("それは霜に覆われた...", "It is covered in frost..."));
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_bolt(player_ptr, AttributeType::COLD, dir, damroll(6, 8));
    return true;
}

bool activate_bolt_hypodynamia_1(PlayerType *player_ptr, concptr name)
{
    DIRECTION dir;
    msg_format(_("あなたは%sに敵を締め殺すよう命じた。", "You order the %s to strangle your opponent."), name);
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    hypodynamic_bolt(player_ptr, dir, 100);
    return true;
}

bool activate_bolt_hypodynamia_2(PlayerType *player_ptr)
{
    DIRECTION dir;
    msg_print(_("黒く輝いている...", "It glows black..."));
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    hypodynamic_bolt(player_ptr, dir, 120);
    return true;
}

bool activate_bolt_drain_1(PlayerType *player_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    for (int dummy = 0; dummy < 3; dummy++) {
        if (hypodynamic_bolt(player_ptr, dir, 50)) {
            hp_player(player_ptr, 50);
        }
    }

    return true;
}

bool activate_bolt_drain_2(PlayerType *player_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    for (int dummy = 0; dummy < 3; dummy++) {
        if (hypodynamic_bolt(player_ptr, dir, 100)) {
            hp_player(player_ptr, 100);
        }
    }

    return true;
}

bool activate_bolt_mana(PlayerType *player_ptr, concptr name)
{
    DIRECTION dir;
    msg_format(_("%sに魔法のトゲが現れた...", "The %s grows magical spikes..."), name);
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_bolt(player_ptr, AttributeType::MISSILE, dir, 150);
    return true;
}

bool activate_ball_pois_1(PlayerType *player_ptr)
{
    DIRECTION dir;
    msg_print(_("それは濃緑色に脈動している...", "It throbs deep green..."));
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_ball(player_ptr, AttributeType::POIS, dir, 12, 3);
    return true;
}

bool activate_ball_cold_1(PlayerType *player_ptr)
{
    DIRECTION dir;
    msg_print(_("それは霜に覆われた...", "It is covered in frost..."));
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_ball(player_ptr, AttributeType::COLD, dir, 48, 2);
    return true;
}

bool activate_ball_cold_2(PlayerType *player_ptr)
{
    DIRECTION dir;
    msg_print(_("それは青く激しく輝いた...", "It glows an intense blue..."));
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_ball(player_ptr, AttributeType::COLD, dir, 100, 2);
    return true;
}

bool activate_ball_cold_3(PlayerType *player_ptr)
{
    DIRECTION dir;
    msg_print(_("明るく白色に輝いている...", "It glows bright white..."));
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_ball(player_ptr, AttributeType::COLD, dir, 400, 3);
    return true;
}

bool activate_ball_fire_1(PlayerType *player_ptr)
{
    DIRECTION dir;
    msg_print(_("それは赤く激しく輝いた...", "It glows an intense red..."));
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_ball(player_ptr, AttributeType::FIRE, dir, 72, 2);
    return true;
}

bool activate_ball_fire_2(PlayerType *player_ptr, concptr name)
{
    DIRECTION dir;
    msg_format(_("%sから炎が吹き出した...", "The %s rages in fire..."), name);
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_ball(player_ptr, AttributeType::FIRE, dir, 120, 3);
    return true;
}

bool activate_ball_fire_3(PlayerType *player_ptr)
{
    DIRECTION dir;
    msg_print(_("深赤色に輝いている...", "It glows deep red..."));
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_ball(player_ptr, AttributeType::FIRE, dir, 300, 3);
    return true;
}

bool activate_ball_fire_4(PlayerType *player_ptr)
{
    DIRECTION dir;
    msg_print(_("それは赤く激しく輝いた...", "It glows an intense red..."));
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_ball(player_ptr, AttributeType::FIRE, dir, 100, 2);
    return true;
}

bool activate_ball_elec_2(PlayerType *player_ptr)
{
    DIRECTION dir;
    msg_print(_("電気がパチパチ音を立てた...", "It crackles with electricity..."));
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_ball(player_ptr, AttributeType::ELEC, dir, 100, 3);
    return true;
}

bool activate_ball_elec_3(PlayerType *player_ptr)
{
    DIRECTION dir;
    msg_print(_("深青色に輝いている...", "It glows deep blue..."));
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_ball(player_ptr, AttributeType::ELEC, dir, 500, 3);
    return true;
}

bool activate_ball_acid_1(PlayerType *player_ptr)
{
    DIRECTION dir;
    msg_print(_("それは黒く激しく輝いた...", "It glows an intense black..."));
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_ball(player_ptr, AttributeType::ACID, dir, 100, 2);
    return true;
}

bool activate_ball_nuke_1(PlayerType *player_ptr)
{
    DIRECTION dir;
    msg_print(_("それは緑に激しく輝いた...", "It glows an intense green..."));
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_ball(player_ptr, AttributeType::NUKE, dir, 100, 2);
    return true;
}

bool activate_rocket(PlayerType *player_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    msg_print(_("ロケットを発射した！", "You launch a rocket!"));
    (void)fire_ball(player_ptr, AttributeType::ROCKET, dir, 250 + player_ptr->lev * 3, 2);
    return true;
}

bool activate_ball_water(PlayerType *player_ptr, concptr name)
{
    DIRECTION dir;
    msg_format(_("%sが深い青色に鼓動している...", "The %s throbs deep blue..."), name);
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_ball(player_ptr, AttributeType::WATER, dir, 200, 3);
    return true;
}

bool activate_ball_lite(PlayerType *player_ptr, concptr name)
{
    int num = damroll(5, 3);
    POSITION y = 0, x = 0;
    msg_format(_("%sが稲妻で覆われた...", "The %s is surrounded by lightning..."), name);
    for (int k = 0; k < num; k++) {
        int attempts = 1000;
        while (attempts--) {
            scatter(player_ptr, &y, &x, player_ptr->y, player_ptr->x, 4, PROJECT_NONE);
            if (!cave_has_flag_bold(player_ptr->current_floor_ptr, y, x, FloorFeatureType::PROJECT)) {
                continue;
            }

            if (!player_bold(player_ptr, y, x)) {
                break;
            }
        }

        project(player_ptr, 0, 3, y, x, 150, AttributeType::ELEC, PROJECT_THRU | PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
    }

    return true;
}

bool activate_ball_dark(PlayerType *player_ptr, concptr name)
{
    DIRECTION dir;
    msg_format(_("%sが深い闇に覆われた...", "The %s is covered in pitch-darkness..."), name);
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_ball(player_ptr, AttributeType::DARK, dir, 250, 4);
    return true;
}

bool activate_ball_mana(PlayerType *player_ptr, concptr name)
{
    DIRECTION dir;
    msg_format(_("%sが青白く光った．．．", "The %s becomes pale..."), name);
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    (void)fire_ball(player_ptr, AttributeType::MANA, dir, 250, 4);
    return true;
}
