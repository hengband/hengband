/*!
 * @brief 発動処理その他 (肥大化しがちなので適宜まとまりを別ファイルへ分割すること)
 * @date 2020/08/19
 * @author Hourier
 */

#include "object-activation/activation-others.h"
#include "artifact/fixed-art-types.h"
#include "cmd-io/cmd-save.h"
#include "core/asking-player.h"
#include "core/hp-mp-processor.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "game-option/special-options.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-status.h"
#include "player-attack/player-attack.h"
#include "player-info/avatar.h"
#include "player/player-damage.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/magic-item-recharger.h"
#include "spell-kind/spells-beam.h"
#include "spell-kind/spells-curse-removal.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-fetcher.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-grid.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-random.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-hex.h"
#include "spell/spell-types.h"
#include "spell/spells-status.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "target/target-getter.h"
#include "util/quarks.h"
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

        if (m_ptr->r_idx == MON_DIO)
            msg_print(_("きさま！　見ているなッ！", "You bastard!, You're watching me, well watch this!"));
    }

    return TRUE;
}

bool activate_dispel_curse(player_type *user_ptr, concptr name)
{
    msg_format(_("%sが真実を照らし出す...", "The %s exhibits the truth..."), name);
    (void)remove_all_curse(user_ptr);
    (void)probing(user_ptr);
    return TRUE;
}

bool activate_cure_lw(player_type *user_ptr)
{
    (void)set_afraid(user_ptr, 0);
    (void)hp_player(user_ptr, 30);
    return TRUE;
}

bool activate_grand_cross(player_type *user_ptr)
{
    msg_print(_("「闇に還れ！」", "You say, 'Return to darkness!'"));
    (void)project(user_ptr, 0, 8, user_ptr->y, user_ptr->x, (randint1(100) + 200) * 2, GF_HOLY_FIRE, PROJECT_KILL | PROJECT_ITEM | PROJECT_GRID, -1);
    return TRUE;
}

bool activate_call_chaos(player_type *user_ptr)
{
    msg_print(_("様々な色の火花を発している...", "It glows in scintillating colours..."));
    call_chaos(user_ptr);
    return TRUE;
}

bool activate_dispel_evil(player_type *user_ptr)
{
    msg_print(_("神聖な雰囲気が充満した...", "It floods the area with goodness..."));
    dispel_evil(user_ptr, user_ptr->lev * 5);
    return TRUE;
}

bool activate_dispel_good(player_type *user_ptr)
{
    msg_print(_("邪悪な雰囲気が充満した...", "It floods the area with evil..."));
    dispel_good(user_ptr, user_ptr->lev * 5);
    return TRUE;
}

bool activate_all_monsters_detection(player_type *user_ptr)
{
    (void)detect_monsters_invis(user_ptr, 255);
    (void)detect_monsters_normal(user_ptr, 255);
    return TRUE;
}

bool activate_all_detection(player_type *user_ptr)
{
    msg_print(_("白く明るく輝いている...", "It glows bright white..."));
    msg_print(_("心にイメージが浮かんできた...", "An image forms in your mind..."));
    detect_all(user_ptr, DETECT_RAD_DEFAULT);
    return TRUE;
}

bool activate_extra_detection(player_type *user_ptr)
{
    msg_print(_("明るく輝いている...", "It glows brightly..."));
    detect_all(user_ptr, DETECT_RAD_DEFAULT);
    probing(user_ptr);
    identify_fully(user_ptr, FALSE, 0);
    return TRUE;
}

bool activate_fully_identification(player_type *user_ptr)
{
    msg_print(_("黄色く輝いている...", "It glows yellow..."));
    identify_fully(user_ptr, FALSE, 0);
    return TRUE;
}

/*!
 * @brief switch_activation() から個々のスペルへの依存性をなくすためのシンタックスシュガー
 * @param user_ptr プレーヤーへの参照ポインタ
 * @return 発動に成功したらTRUE
 */
bool activate_identification(player_type *user_ptr) { return ident_spell(user_ptr, FALSE, 0); }

bool activate_pesticide(player_type *user_ptr)
{
    msg_print(_("あなたは害虫を一掃した。", "You exterminate small life."));
    (void)dispel_monsters(user_ptr, 4);
    return TRUE;
}

/*!
 * @brief switch_activation() から個々のスペルへの依存性をなくすためのシンタックスシュガー
 * @param user_ptr プレーヤーへの参照ポインタ
 * @return 発動に成功したらTRUE
 */
bool activate_whirlwind(player_type *user_ptr)
{
    massacre(user_ptr);
    return TRUE;
}

bool activate_blinding_light(player_type *user_ptr, concptr name)
{
    msg_format(_("%sが眩しい光で輝いた...", "The %s gleams with blinding light..."), name);
    (void)fire_ball(user_ptr, GF_LITE, 0, 300, 6);
    confuse_monsters(user_ptr, 3 * user_ptr->lev / 2);
    return TRUE;
}

bool activate_sleep(player_type *user_ptr)
{
    msg_print(_("深青色に輝いている...", "It glows deep blue..."));
    sleep_monsters_touch(user_ptr);
    return TRUE;
}

bool activate_door_destroy(player_type *user_ptr)
{
    msg_print(_("明るい赤色に輝いている...", "It glows bright red..."));
    destroy_doors_touch(user_ptr);
    return TRUE;
}

bool activate_earthquake(player_type *user_ptr)
{
    earthquake(user_ptr, user_ptr->y, user_ptr->x, 5, 0);
    return TRUE;
}

bool activate_recharge(player_type *user_ptr)
{
    recharge(user_ptr, 130);
    return TRUE;
}

bool activate_recharge_extra(player_type *user_ptr, concptr name)
{
    msg_format(_("%sが白く輝いた．．．", "The %s gleams with blinding light..."), name);
    return recharge(user_ptr, 1000);
}

bool activate_shikofumi(player_type *user_ptr)
{
    msg_print(_("力強く四股を踏んだ。", "You stamp. (as if you are in a ring.)"));
    (void)set_afraid(user_ptr, 0);
    (void)set_hero(user_ptr, randint1(20) + 20, FALSE);
    (void)dispel_evil(user_ptr, user_ptr->lev * 3);
    return TRUE;
}

bool activate_terror(player_type *user_ptr)
{
    turn_monsters(user_ptr, 40 + user_ptr->lev);
    return TRUE;
}

bool activate_map_light(player_type *user_ptr)
{
    msg_print(_("眩しく輝いた...", "It shines brightly..."));
    map_area(user_ptr, DETECT_RAD_MAP);
    lite_area(user_ptr, damroll(2, 15), 3);
    return TRUE;
}

bool activate_exploding_rune(player_type *user_ptr)
{
    msg_print(_("明るい赤色に輝いている...", "It glows bright red..."));
    explosive_rune(user_ptr, user_ptr->y, user_ptr->x);
    return TRUE;
}

bool activate_protection_rune(player_type *user_ptr)
{
    msg_print(_("ブルーに明るく輝いている...", "It glows light blue..."));
    warding_glyph(user_ptr);
    return TRUE;
}

bool activate_light(player_type *user_ptr, concptr name)
{
    msg_format(_("%sから澄んだ光があふれ出た...", "The %s wells with clear light..."), name);
    (void)lite_area(user_ptr, damroll(2, 15), 3);
    return TRUE;
}

bool activate_recall(player_type *user_ptr)
{
    msg_print(_("やわらかな白色に輝いている...", "It glows soft white..."));
    return recall_player(user_ptr, randint0(21) + 15);
}

bool activate_tree_creation(player_type *user_ptr, object_type *o_ptr, concptr name)
{
    msg_format(_("%s%sから明るい緑の光があふれ出た...", "The %s%s wells with clear light..."), name, quark_str(o_ptr->art_name));
    return tree_creation(user_ptr, user_ptr->y, user_ptr->x);
}

bool activate_animate_dead(player_type *user_ptr, object_type *o_ptr)
{
    msg_print(_("黄金色の光が溢れ出た...", "It emitted a golden light..."));
    if (o_ptr->name1 > 0)
        msg_print(_("ぴぴるぴるぴるぴぴるぴ～♪", "Pipiru piru piru pipiru pii"));

    return animate_dead(user_ptr, 0, user_ptr->y, user_ptr->x);
}
