#include "object-activation/activation-bolt-ball.h"
#include "core/hp-mp-processor.h"
#include "effect/effect-characteristics.h"
#include "floor/cave.h"
#include "floor/floor.h"
#include "grid/feature-flag-types.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-specific-bolt.h"
#include "spell/process-effect.h"
#include "spell/spell-types.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool activate_missile_1(player_type *user_ptr)
{
    DIRECTION dir;
    msg_print(_("‚»‚ê‚Íá¿‚µ‚¢‚­‚ç‚¢‚É–¾‚é‚­‹P‚¢‚Ä‚¢‚é...", "It glows extremely brightly..."));
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_bolt(user_ptr, GF_MISSILE, dir, damroll(2, 6));
    return TRUE;
}

bool activate_missile_2(player_type *user_ptr)
{
    DIRECTION dir;
    msg_print(_("–‚–@‚ÌƒgƒQ‚ªŒ»‚ê‚½...", "It grows magical spikes..."));
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_bolt(user_ptr, GF_ARROW, dir, 150);
    return TRUE;
}

bool activate_missile_3(player_type *user_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    msg_print(_("‚ ‚È‚½‚ÍƒGƒŒƒƒ“ƒg‚ÌƒuƒŒƒX‚ğ“f‚¢‚½B", "You breathe the elements."));
    fire_breath(user_ptr, GF_MISSILE, dir, 300, 4);
    return TRUE;
}

bool activate_bolt_acid_1(player_type *user_ptr)
{
    DIRECTION dir;
    msg_print(_("‚»‚ê‚Í_‚É•¢‚í‚ê‚½...", "It is covered in acid..."));
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_bolt(user_ptr, GF_ACID, dir, damroll(5, 8));
    return TRUE;
}

bool activate_bolt_elec_1(player_type *user_ptr)
{
    DIRECTION dir;
    msg_print(_("‚»‚ê‚Í‰Î‰Ô‚É•¢‚í‚ê‚½...", "It is covered in sparks..."));
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_bolt(user_ptr, GF_ELEC, dir, damroll(4, 8));
    return TRUE;
}

bool activate_bolt_fire_1(player_type *user_ptr)
{
    DIRECTION dir;
    msg_print(_("‚»‚ê‚Í‰Š‚É•¢‚í‚ê‚½...", "It is covered in fire..."));
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_bolt(user_ptr, GF_FIRE, dir, damroll(9, 8));
    return TRUE;
}

bool activate_bolt_cold_1(player_type *user_ptr)
{
    DIRECTION dir;
    msg_print(_("‚»‚ê‚Í‘š‚É•¢‚í‚ê‚½...", "It is covered in frost..."));
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_bolt(user_ptr, GF_COLD, dir, damroll(6, 8));
    return TRUE;
}

bool activate_bolt_hypodynamia_1(player_type *user_ptr, concptr name)
{
    DIRECTION dir;
    msg_format(_("‚ ‚È‚½‚Í%s‚É“G‚ğ’÷‚ßE‚·‚æ‚¤–½‚¶‚½B", "You order the %s to strangle your opponent."), name);
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    hypodynamic_bolt(user_ptr, dir, 100);
    return TRUE;
}

bool activate_bolt_hypodynamia_2(player_type *user_ptr)
{
    DIRECTION dir;
    msg_print(_("•‚­‹P‚¢‚Ä‚¢‚é...", "It glows black..."));
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    hypodynamic_bolt(user_ptr, dir, 120);
    return TRUE;
}

bool activate_bolt_drain_1(player_type *user_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    for (int dummy = 0; dummy < 3; dummy++)
        if (hypodynamic_bolt(user_ptr, dir, 50))
            hp_player(user_ptr, 50);

    return TRUE;
}

bool activate_bolt_drain_2(player_type *user_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    for (int dummy = 0; dummy < 3; dummy++)
        if (hypodynamic_bolt(user_ptr, dir, 100))
            hp_player(user_ptr, 100);

    return TRUE;
}

bool activate_bolt_mana(player_type *user_ptr, concptr name)
{
    DIRECTION dir;
    msg_format(_("%s‚É–‚–@‚ÌƒgƒQ‚ªŒ»‚ê‚½...", "The %s grows magical spikes..."), name);
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_bolt(user_ptr, GF_ARROW, dir, 150);
    return TRUE;
}

bool activate_ball_pois_1(player_type *user_ptr)
{
    DIRECTION dir;
    msg_print(_("‚»‚ê‚Í”Z—ÎF‚É–¬“®‚µ‚Ä‚¢‚é...", "It throbs deep green..."));
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_POIS, dir, 12, 3);
    return TRUE;
}

bool activate_ball_cold_1(player_type *user_ptr)
{
    DIRECTION dir;
    msg_print(_("‚»‚ê‚Í‘š‚É•¢‚í‚ê‚½...", "It is covered in frost..."));
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_COLD, dir, 48, 2);
    return TRUE;
}

bool activate_ball_cold_2(player_type *user_ptr)
{
    DIRECTION dir;
    msg_print(_("‚»‚ê‚ÍÂ‚­Œƒ‚µ‚­‹P‚¢‚½...", "It glows an intense blue..."));
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_COLD, dir, 100, 2);
    return TRUE;
}

bool activate_ball_cold_3(player_type *user_ptr)
{
    DIRECTION dir;
    msg_print(_("–¾‚é‚­”’F‚É‹P‚¢‚Ä‚¢‚é...", "It glows bright white..."));
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_COLD, dir, 400, 3);
    return TRUE;
}

bool activate_ball_fire_1(player_type *user_ptr)
{
    DIRECTION dir;
    msg_print(_("‚»‚ê‚ÍÔ‚­Œƒ‚µ‚­‹P‚¢‚½...", "It glows an intense red..."));
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_FIRE, dir, 72, 2);
    return TRUE;
}

bool activate_ball_fire_2(player_type *user_ptr, concptr name)
{
    DIRECTION dir;
    msg_format(_("%s‚©‚ç‰Š‚ª‚«o‚µ‚½...", "The %s rages in fire..."), name);
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_FIRE, dir, 120, 3);
    return TRUE;
}

bool activate_ball_fire_3(player_type *user_ptr)
{
    DIRECTION dir;
    msg_print(_("[ÔF‚É‹P‚¢‚Ä‚¢‚é...", "It glows deep red..."));
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_FIRE, dir, 300, 3);
    return TRUE;
}

bool activate_ball_fire_4(player_type *user_ptr)
{
    DIRECTION dir;
    msg_print(_("‚»‚ê‚ÍÔ‚­Œƒ‚µ‚­‹P‚¢‚½...", "It glows an intense red..."));
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_FIRE, dir, 100, 2);
    return TRUE;
}

bool activate_ball_elec_2(player_type *user_ptr)
{
    DIRECTION dir;
    msg_print(_("“d‹C‚ªƒpƒ`ƒpƒ`‰¹‚ğ—§‚Ä‚½...", "It crackles with electricity..."));
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_ELEC, dir, 100, 3);
    return TRUE;
}

bool activate_ball_elec_3(player_type *user_ptr)
{
    DIRECTION dir;
    msg_print(_("[ÂF‚É‹P‚¢‚Ä‚¢‚é...", "It glows deep blue..."));
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_ELEC, dir, 500, 3);
    return TRUE;
}

bool activate_ball_acid_1(player_type *user_ptr)
{
    DIRECTION dir;
    msg_print(_("‚»‚ê‚Í•‚­Œƒ‚µ‚­‹P‚¢‚½...", "It glows an intense black..."));
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_ACID, dir, 100, 2);
    return TRUE;
}

bool activate_ball_nuke_1(player_type *user_ptr)
{
    DIRECTION dir;
    msg_print(_("‚»‚ê‚Í—Î‚ÉŒƒ‚µ‚­‹P‚¢‚½...", "It glows an intense green..."));
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_NUKE, dir, 100, 2);
    return TRUE;
}

bool activate_rocket(player_type *user_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    msg_print(_("ƒƒPƒbƒg‚ğ”­Ë‚µ‚½I", "You launch a rocket!"));
    (void)fire_ball(user_ptr, GF_ROCKET, dir, 250 + user_ptr->lev * 3, 2);
    return TRUE;
}

bool activate_ball_water(player_type *user_ptr, concptr name)
{
    DIRECTION dir;
    msg_format(_("%s‚ª[‚¢ÂF‚ÉŒÛ“®‚µ‚Ä‚¢‚é...", "The %s throbs deep blue..."), name);
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_WATER, dir, 200, 3);
    return TRUE;
}

bool activate_ball_lite(player_type *user_ptr, concptr name)
{
    HIT_POINT num = damroll(5, 3);
    POSITION y = 0, x = 0;
    msg_format(_("%s‚ªˆîÈ‚Å•¢‚í‚ê‚½...", "The %s is surrounded by lightning..."), name);
    for (int k = 0; k < num; k++) {
        int attempts = 1000;
        while (attempts--) {
            scatter(user_ptr, &y, &x, user_ptr->y, user_ptr->x, 4, 0);
            if (!cave_have_flag_bold(user_ptr->current_floor_ptr, y, x, FF_PROJECT))
                continue;

            if (!player_bold(user_ptr, y, x))
                break;
        }

        project(user_ptr, 0, 3, y, x, 150, GF_ELEC, PROJECT_THRU | PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL, -1);
    }

    return TRUE;
}

bool activate_ball_dark(player_type *user_ptr, concptr name)
{
    DIRECTION dir;
    msg_format(_("%s‚ª[‚¢ˆÅ‚É•¢‚í‚ê‚½...", "The %s is coverd in pitch-darkness..."), name);
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_DARK, dir, 250, 4);
    return TRUE;
}

bool activate_ball_mana(player_type *user_ptr, concptr name)
{
    DIRECTION dir;
    msg_format(_("%s‚ªÂ”’‚­Œõ‚Á‚½DDD", "The %s glows pale..."), name);
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_ball(user_ptr, GF_MANA, dir, 250, 4);
    return TRUE;
}
