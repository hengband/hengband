#include "cmd-item/cmd-zaprod.h"
#include "action/action-limited.h"
#include "avatar/avatar.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "floor/floor-object.h"
#include "game-option/disturbance-options.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-enchant/special-object-flags.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "player-info/class-info.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "spell-kind/spells-beam.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-random.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-specific-bolt.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell/spell-types.h"
#include "spell/spells-status.h"
#include "status/action-setter.h"
#include "status/buff-setter.h"
#include "status/experience.h"
#include "status/shape-changer.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-rod-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief ロッドの効果を発動する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param sval オブジェクトのsval
 * @param dir 発動目標の方向ID
 * @param use_charge チャージを消費したかどうかを返す参照ポインタ
 * @param powerful 強力発動上の処理ならばTRUE
 * @param magic 魔道具術上の処理ならばTRUE
 * @return 発動により効果内容が確定したならばTRUEを返す
 */
int rod_effect(player_type *player_ptr, OBJECT_SUBTYPE_VALUE sval, DIRECTION dir, bool *use_charge, bool powerful, bool magic)
{
    int ident = false;
    PLAYER_LEVEL lev = powerful ? player_ptr->lev * 2 : player_ptr->lev;
    POSITION detect_rad = powerful ? DETECT_RAD_DEFAULT * 3 / 2 : DETECT_RAD_DEFAULT;
    POSITION rad = powerful ? 3 : 2;

    /* Unused */
    (void)magic;

    /* Analyze the rod */
    switch (sval) {
    case SV_ROD_DETECT_TRAP: {
        if (detect_traps(player_ptr, detect_rad, dir == 0))
            ident = true;
        break;
    }

    case SV_ROD_DETECT_DOOR: {
        if (detect_doors(player_ptr, detect_rad))
            ident = true;
        if (detect_stairs(player_ptr, detect_rad))
            ident = true;
        break;
    }

    case SV_ROD_IDENTIFY: {
        if (powerful) {
            if (!identify_fully(player_ptr, false))
                *use_charge = false;
        } else {
            if (!ident_spell(player_ptr, false))
                *use_charge = false;
        }
        ident = true;
        break;
    }

    case SV_ROD_RECALL: {
        if (!recall_player(player_ptr, randint0(21) + 15))
            *use_charge = false;
        ident = true;
        break;
    }

    case SV_ROD_ILLUMINATION: {
        if (lite_area(player_ptr, damroll(2, 8), (powerful ? 4 : 2)))
            ident = true;
        break;
    }

    case SV_ROD_MAPPING: {
        map_area(player_ptr, powerful ? DETECT_RAD_MAP * 3 / 2 : DETECT_RAD_MAP);
        ident = true;
        break;
    }

    case SV_ROD_DETECTION: {
        detect_all(player_ptr, detect_rad);
        ident = true;
        break;
    }

    case SV_ROD_PROBING: {
        probing(player_ptr);
        ident = true;
        break;
    }

    case SV_ROD_CURING: {
        if (true_healing(player_ptr, 0))
            ident = true;
        if (set_shero(player_ptr, 0, true))
            ident = true;
        break;
    }

    case SV_ROD_HEALING: {
        if (cure_critical_wounds(player_ptr, powerful ? 750 : 500))
            ident = true;
        break;
    }

    case SV_ROD_RESTORATION: {
        if (restore_level(player_ptr))
            ident = true;
        if (restore_all_status(player_ptr))
            ident = true;
        break;
    }

    case SV_ROD_SPEED: {
        if (set_fast(player_ptr, randint1(30) + (powerful ? 30 : 15), false))
            ident = true;
        break;
    }

    case SV_ROD_PESTICIDE: {
        if (dispel_monsters(player_ptr, powerful ? 8 : 4))
            ident = true;
        break;
    }

    case SV_ROD_TELEPORT_AWAY: {
        int distance = MAX_SIGHT * (powerful ? 8 : 5);
        if (teleport_monster(player_ptr, dir, distance))
            ident = true;
        break;
    }

    case SV_ROD_DISARMING: {
        if (disarm_trap(player_ptr, dir))
            ident = true;
        if (powerful && disarm_traps_touch(player_ptr))
            ident = true;
        break;
    }

    case SV_ROD_LITE: {
        HIT_POINT dam = damroll((powerful ? 12 : 6), 8);
        msg_print(_("青く輝く光線が放たれた。", "A line of blue shimmering light appears."));
        (void)lite_line(player_ptr, dir, dam);
        ident = true;
        break;
    }

    case SV_ROD_SLEEP_MONSTER: {
        if (sleep_monster(player_ptr, dir, lev))
            ident = true;
        break;
    }

    case SV_ROD_SLOW_MONSTER: {
        if (slow_monster(player_ptr, dir, lev))
            ident = true;
        break;
    }

    case SV_ROD_HYPODYNAMIA: {
        if (hypodynamic_bolt(player_ptr, dir, 70 + 3 * lev / 2))
            ident = true;
        break;
    }

    case SV_ROD_POLYMORPH: {
        if (poly_monster(player_ptr, dir, lev))
            ident = true;
        break;
    }

    case SV_ROD_ACID_BOLT: {
        fire_bolt_or_beam(player_ptr, 10, GF_ACID, dir, damroll(6 + lev / 7, 8));
        ident = true;
        break;
    }

    case SV_ROD_ELEC_BOLT: {
        fire_bolt_or_beam(player_ptr, 10, GF_ELEC, dir, damroll(4 + lev / 9, 8));
        ident = true;
        break;
    }

    case SV_ROD_FIRE_BOLT: {
        fire_bolt_or_beam(player_ptr, 10, GF_FIRE, dir, damroll(7 + lev / 6, 8));
        ident = true;
        break;
    }

    case SV_ROD_COLD_BOLT: {
        fire_bolt_or_beam(player_ptr, 10, GF_COLD, dir, damroll(5 + lev / 8, 8));
        ident = true;
        break;
    }

    case SV_ROD_ACID_BALL: {
        fire_ball(player_ptr, GF_ACID, dir, 60 + lev, rad);
        ident = true;
        break;
    }

    case SV_ROD_ELEC_BALL: {
        fire_ball(player_ptr, GF_ELEC, dir, 40 + lev, rad);
        ident = true;
        break;
    }

    case SV_ROD_FIRE_BALL: {
        fire_ball(player_ptr, GF_FIRE, dir, 70 + lev, rad);
        ident = true;
        break;
    }

    case SV_ROD_COLD_BALL: {
        fire_ball(player_ptr, GF_COLD, dir, 50 + lev, rad);
        ident = true;
        break;
    }

    case SV_ROD_HAVOC: {
        call_chaos(player_ptr);
        ident = true;
        break;
    }

    case SV_ROD_STONE_TO_MUD: {
        HIT_POINT dam = powerful ? 40 + randint1(60) : 20 + randint1(30);
        if (wall_to_mud(player_ptr, dir, dam))
            ident = true;
        break;
    }

    case SV_ROD_AGGRAVATE: {
        aggravate_monsters(player_ptr, 0);
        ident = true;
        break;
    }
    }
    return ident;
}

/*!
 * @brief ロッドを使うコマンドのサブルーチン /
 * Activate (zap) a Rod
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param item 使うオブジェクトの所持品ID
 * @details
 * <pre>
 * Unstack fully charged rods as needed.
 * Hack -- rods of perception/genocide can be "cancelled"
 * All rods can be cancelled at the "Direction?" prompt
 * pvals are defined for each rod in k_info. -LM-
 * </pre>
 */
void exe_zap_rod(player_type *player_ptr, INVENTORY_IDX item)
{
    int ident, chance, lev, fail;
    DIRECTION dir = 0;
    object_type *o_ptr;
    bool success;

    /* Hack -- let perception get aborted */
    bool use_charge = true;

    object_kind *k_ptr;

    o_ptr = ref_item(player_ptr, item);

    /* Mega-Hack -- refuse to zap a pile from the ground */
    if ((item < 0) && (o_ptr->number > 1)) {
        msg_print(_("まずはロッドを拾わなければ。", "You must first pick up the rods."));
        return;
    }

    /* Get a direction (unless KNOWN not to need it) */
    if (((o_ptr->sval >= SV_ROD_MIN_DIRECTION) && (o_ptr->sval != SV_ROD_HAVOC) && (o_ptr->sval != SV_ROD_AGGRAVATE) && (o_ptr->sval != SV_ROD_PESTICIDE))
        || !o_ptr->is_aware()) {
        /* Get a direction, allow cancel */
        if (!get_aim_dir(player_ptr, &dir))
            return;
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);

    lev = k_info[o_ptr->k_idx].level;

    /* Base chance of success */
    chance = player_ptr->skill_dev;

    /* Confusion hurts skill */
    if (player_ptr->confused)
        chance = chance / 2;

    fail = lev + 5;
    if (chance > fail)
        fail -= (chance - fail) * 2;
    else
        chance -= (fail - chance) * 2;
    if (fail < USE_DEVICE)
        fail = USE_DEVICE;
    if (chance < USE_DEVICE)
        chance = USE_DEVICE;

    if (cmd_limit_time_walk(player_ptr))
        return;

    if (player_ptr->pclass == CLASS_BERSERKER)
        success = false;
    else if (chance > fail) {
        if (randint0(chance * 2) < fail)
            success = false;
        else
            success = true;
    } else {
        if (randint0(fail * 2) < chance)
            success = true;
        else
            success = false;
    }

    /* Roll for usage */
    if (!success) {
        if (flush_failure)
            flush();
        msg_print(_("うまくロッドを使えなかった。", "You failed to use the rod properly."));
        sound(SOUND_FAIL);
        return;
    }

    k_ptr = &k_info[o_ptr->k_idx];

    /* A single rod is still charging */
    if ((o_ptr->number == 1) && (o_ptr->timeout)) {
        if (flush_failure)
            flush();
        msg_print(_("このロッドはまだ魔力を充填している最中だ。", "The rod is still charging."));
        return;
    }
    /* A stack of rods lacks enough energy. */
    else if ((o_ptr->number > 1) && (o_ptr->timeout > k_ptr->pval * (o_ptr->number - 1))) {
        if (flush_failure)
            flush();
        msg_print(_("そのロッドはまだ充填中です。", "The rods are all still charging."));
        return;
    }

    sound(SOUND_ZAP);

    ident = rod_effect(player_ptr, o_ptr->sval, dir, &use_charge, false, false);

    /* Increase the timeout by the rod kind's pval. -LM- */
    if (use_charge)
        o_ptr->timeout += k_ptr->pval;
    player_ptr->update |= (PU_COMBINE | PU_REORDER);

    if (!(o_ptr->is_aware())) {
        chg_virtue(player_ptr, V_PATIENCE, -1);
        chg_virtue(player_ptr, V_CHANCE, 1);
        chg_virtue(player_ptr, V_KNOWLEDGE, -1);
    }

    /* Tried the object */
    object_tried(o_ptr);

    /* Successfully determined the object function */
    if (ident && !o_ptr->is_aware()) {
        object_aware(player_ptr, o_ptr);
        gain_exp(player_ptr, (lev + (player_ptr->lev >> 1)) / player_ptr->lev);
    }

    set_bits(player_ptr->window_flags, PW_INVEN | PW_EQUIP | PW_PLAYER | PW_FLOOR_ITEM_LIST);
}

/*!
 * @brief ロッドを使うコマンドのメインルーチン /
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_zap_rod(player_type *player_ptr)
{
    OBJECT_IDX item;
    concptr q, s;

    if (player_ptr->wild_mode) {
        return;
    }

    if (cmd_limit_arena(player_ptr))
        return;

    if (player_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN)) {
        set_action(player_ptr, ACTION_NONE);
    }

    q = _("どのロッドを振りますか? ", "Zap which rod? ");
    s = _("使えるロッドがない。", "You have no rod to zap.");

    if (!choose_object(player_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), TvalItemTester(TV_ROD)))
        return;

    /* Zap the rod */
    exe_zap_rod(player_ptr, item);
}
