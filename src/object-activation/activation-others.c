#include "object-activation/activation-others.h"
#include "art-definition/art-weapon-types.h"
#include "cmd-io/cmd-save.h"
#include "core/asking-player.h"
#include "core/hp-mp-processor.h"
#include "game-option/special-options.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-status.h"
#include "player/avatar.h"
#include "player/player-damage.h"
#include "spell/spell-types.h"
#include "spell-kind/spells-beam.h"
#include "spell-kind/spells-curse-removal.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-fetcher.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-grid.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-hex.h"
#include "spell/spells-status.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool activate_sunlight(player_type *user_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    msg_print(_("太陽光線が放たれた。", "A line of sunlight appears."));
    (void)lite_line(user_ptr, dir, damroll(6, 8));
    return TRUE;
}

bool activate_confusion(player_type *user_ptr)
{
    DIRECTION dir;
    msg_print(_("様々な色の火花を発している...", "It glows in scintillating colours..."));
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    confuse_monster(user_ptr, dir, 20);
    return TRUE;
}

bool activate_teleport_away(player_type *user_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    (void)fire_beam(user_ptr, GF_AWAY_ALL, dir, user_ptr->lev);
    return TRUE;
}

bool activate_banish_evil(player_type *user_ptr)
{
    if (banish_evil(user_ptr, 100))
        msg_print(_("アーティファクトの力が邪悪を打ち払った！", "The power of the artifact banishes evil!"));

    return TRUE;
}

bool activate_scare(player_type *user_ptr)
{
    if (music_singing_any(user_ptr))
        stop_singing(user_ptr);

    if (hex_spelling_any(user_ptr))
        stop_hex_spell_all(user_ptr);

    msg_print(_("あなたは力強い突風を吹き鳴らした。周囲の敵が震え上っている!", "You wind a mighty blast; your enemies tremble!"));
    (void)turn_monsters(user_ptr, (3 * user_ptr->lev / 2) + 10);
    return TRUE;
}

bool activate_aggravation(player_type *user_ptr, object_type *o_ptr, concptr name)
{
    if (o_ptr->name1 == ART_HYOUSIGI)
        msg_print(_("拍子木を打った。", "You beat your wooden clappers."));
    else
        msg_format(_("%sは不快な物音を立てた。", "The %s sounds an unpleasant noise."), name);

    aggravate_monsters(user_ptr, 0);
    return TRUE;
}

bool activate_stone_mud(player_type *user_ptr)
{
    DIRECTION dir;
    msg_print(_("鼓動している...", "It pulsates..."));
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    wall_to_mud(user_ptr, dir, 20 + randint1(30));
    return TRUE;
}

bool activate_judgement(player_type *user_ptr, concptr name)
{
    msg_format(_("%sは赤く明るく光った！", "The %s flashes bright red!"), name);
    chg_virtue(user_ptr, V_KNOWLEDGE, 1);
    chg_virtue(user_ptr, V_ENLIGHTEN, 1);
    wiz_lite(user_ptr, FALSE);

    msg_format(_("%sはあなたの体力を奪った...", "The %s drains your vitality..."), name);
    take_hit(user_ptr, DAMAGE_LOSELIFE, damroll(3, 8), _("審判の宝石", "the Jewel of Judgement"), -1);

    (void)detect_traps(user_ptr, DETECT_RAD_DEFAULT, TRUE);
    (void)detect_doors(user_ptr, DETECT_RAD_DEFAULT);
    (void)detect_stairs(user_ptr, DETECT_RAD_DEFAULT);

    if (get_check(_("帰還の力を使いますか？", "Activate recall? ")))
        (void)recall_player(user_ptr, randint0(21) + 15);

    return TRUE;
}

bool activate_telekinesis(player_type *user_ptr, concptr name)
{
    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    msg_format(_("%sを伸ばした。", "You stretched your %s."), name);
    fetch_item(user_ptr, dir, 500, TRUE);
    return TRUE;
}

bool activate_unique_detection(player_type *user_ptr)
{
    monster_type *m_ptr;
    monster_race *r_ptr;
    msg_print(_("奇妙な場所が頭の中に浮かんだ．．．", "Some strange places show up in your mind. And you see ..."));
    for (int i = user_ptr->current_floor_ptr->m_max - 1; i >= 1; i--) {
        m_ptr = &user_ptr->current_floor_ptr->m_list[i];
        if (!monster_is_valid(m_ptr))
            continue;

        r_ptr = &r_info[m_ptr->r_idx];
        if (r_ptr->flags1 & RF1_UNIQUE)
            msg_format(_("%s． ", "%s. "), r_name + r_ptr->name);
    }

    return TRUE;
}

bool activate_escape(player_type *user_ptr)
{
    switch (randint1(13)) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        teleport_player(user_ptr, 10, TELEPORT_SPONTANEOUS);
        return TRUE;
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        teleport_player(user_ptr, 222, TELEPORT_SPONTANEOUS);
        return TRUE;
    case 11:
    case 12:
        (void)stair_creation(user_ptr);
        return TRUE;
    default:
        if (!get_check(_("この階を去りますか？", "Leave this level? ")))
            return TRUE;

        if (autosave_l)
            do_cmd_save_game(user_ptr, TRUE);

        user_ptr->leaving = TRUE;
        return TRUE;
    }
}

bool activate_teleport_level(player_type *user_ptr)
{
    if (!get_check(_("本当に他の階にテレポートしますか？", "Are you sure? (Teleport Level)")))
        return FALSE;

    teleport_level(user_ptr, 0);
    return TRUE;
}

bool activate_dimension_door(player_type *user_ptr)
{
    msg_print(_("次元の扉が開いた。目的地を選んで下さい。", "You open a dimensional gate. Choose a destination."));
    return dimension_door(user_ptr);
}

bool activate_teleport(player_type *user_ptr)
{
    msg_print(_("周りの空間が歪んでいる...", "It twists space around you..."));
    teleport_player(user_ptr, 100, TELEPORT_SPONTANEOUS);
    return TRUE;
}

bool activate_dispel_curse(player_type *user_ptr, concptr name)
{
    msg_format(_("%sが真実を照らし出す...", "The %s exhibits the truth..."), name);
    (void)remove_all_curse(user_ptr);
    (void)probing(user_ptr);
    return TRUE;
}
