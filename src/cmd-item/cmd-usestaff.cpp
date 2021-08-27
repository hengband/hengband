#include "cmd-item/cmd-usestaff.h"
#include "action/action-limited.h"
#include "avatar/avatar.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "floor/floor-object.h"
#include "game-option/disturbance-options.h"
#include "inventory/inventory-object.h"
#include "inventory/player-inventory.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "object-enchant/special-object-flags.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/player-class.h"
#include "player/player-race-types.h"
#include "player/player-race.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-curse-removal.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-genocide.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spells-staff-only.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "status/action-setter.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/buff-setter.h"
#include "status/experience.h"
#include "status/shape-changer.h"
#include "status/sight-setter.h"
#include "sv-definition/sv-staff-types.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "view/object-describer.h"

/*!
 * @brief 杖の効果を発動する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param sval オブジェクトのsval
 * @param use_charge 使用回数を消費したかどうかを返す参照ポインタ
 * @param powerful 強力発動上の処理ならばTRUE
 * @param magic 魔道具術上の処理ならばTRUE
 * @param known 判明済ならばTRUE
 * @return 発動により効果内容が確定したならばTRUEを返す
 */
int staff_effect(player_type *creature_ptr, OBJECT_SUBTYPE_VALUE sval, bool *use_charge, bool powerful, bool magic, bool known)
{
    int k;
    bool ident = false;
    PLAYER_LEVEL lev = powerful ? creature_ptr->lev * 2 : creature_ptr->lev;
    POSITION detect_rad = powerful ? DETECT_RAD_DEFAULT * 3 / 2 : DETECT_RAD_DEFAULT;

    /* Analyze the staff */
    switch (sval) {
    case SV_STAFF_DARKNESS: {
        if (!has_resist_blind(creature_ptr) && !has_resist_dark(creature_ptr)) {
            if (set_blind(creature_ptr, creature_ptr->blind + 3 + randint1(5)))
                ident = true;
        }
        if (unlite_area(creature_ptr, 10, (powerful ? 6 : 3)))
            ident = true;
        break;
    }

    case SV_STAFF_SLOWNESS: {
        if (set_slow(creature_ptr, creature_ptr->slow + randint1(30) + 15, false))
            ident = true;
        break;
    }

    case SV_STAFF_HASTE_MONSTERS: {
        if (speed_monsters(creature_ptr))
            ident = true;
        break;
    }

    case SV_STAFF_SUMMONING: {
        const int times = randint1(powerful ? 8 : 4);
        for (k = 0; k < times; k++) {
            if (summon_specific(creature_ptr, 0, creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_NONE,
                    (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET))) {
                ident = true;
            }
        }
        break;
    }

    case SV_STAFF_TELEPORTATION: {
        teleport_player(creature_ptr, (powerful ? 150 : 100), 0L);
        ident = true;
        break;
    }

    case SV_STAFF_IDENTIFY: {
        if (powerful) {
            if (!identify_fully(creature_ptr, false))
                *use_charge = false;
        } else {
            if (!ident_spell(creature_ptr, false))
                *use_charge = false;
        }
        ident = true;
        break;
    }

    case SV_STAFF_REMOVE_CURSE: {
        bool result = (powerful ? remove_all_curse(creature_ptr) : remove_curse(creature_ptr)) != 0;
        if (result) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_STARLITE:
        ident = starlight(creature_ptr, magic);
        break;

    case SV_STAFF_LITE: {
        if (lite_area(creature_ptr, damroll(2, 8), (powerful ? 4 : 2)))
            ident = true;
        break;
    }

    case SV_STAFF_MAPPING: {
        map_area(creature_ptr, powerful ? DETECT_RAD_MAP * 3 / 2 : DETECT_RAD_MAP);
        ident = true;
        break;
    }

    case SV_STAFF_DETECT_GOLD: {
        if (detect_treasure(creature_ptr, detect_rad))
            ident = true;
        if (detect_objects_gold(creature_ptr, detect_rad))
            ident = true;
        break;
    }

    case SV_STAFF_DETECT_ITEM: {
        if (detect_objects_normal(creature_ptr, detect_rad))
            ident = true;
        break;
    }

    case SV_STAFF_DETECT_TRAP: {
        if (detect_traps(creature_ptr, detect_rad, known))
            ident = true;
        break;
    }

    case SV_STAFF_DETECT_DOOR: {
        if (detect_doors(creature_ptr, detect_rad))
            ident = true;
        if (detect_stairs(creature_ptr, detect_rad))
            ident = true;
        break;
    }

    case SV_STAFF_DETECT_INVIS: {
        if (set_tim_invis(creature_ptr, creature_ptr->tim_invis + 12 + randint1(12), false))
            ident = true;
        break;
    }

    case SV_STAFF_DETECT_EVIL: {
        if (detect_monsters_evil(creature_ptr, detect_rad))
            ident = true;
        break;
    }

    case SV_STAFF_CURE_LIGHT: {
        ident = cure_light_wounds(creature_ptr, (powerful ? 4 : 2), 8);
        break;
    }

    case SV_STAFF_CURING: {
        ident = true_healing(creature_ptr, 0);
        if (set_shero(creature_ptr, 0, true))
            ident = true;
        break;
    }

    case SV_STAFF_HEALING: {
        if (cure_critical_wounds(creature_ptr, powerful ? 500 : 300))
            ident = true;
        break;
    }

    case SV_STAFF_THE_MAGI: {
        if (do_res_stat(creature_ptr, A_INT))
            ident = true;
        ident |= restore_mana(creature_ptr, false);
        if (set_shero(creature_ptr, 0, true))
            ident = true;
        break;
    }

    case SV_STAFF_SLEEP_MONSTERS: {
        if (sleep_monsters(creature_ptr, lev))
            ident = true;
        break;
    }

    case SV_STAFF_SLOW_MONSTERS: {
        if (slow_monsters(creature_ptr, lev))
            ident = true;
        break;
    }

    case SV_STAFF_SPEED: {
        if (set_fast(creature_ptr, randint1(30) + (powerful ? 30 : 15), false))
            ident = true;
        break;
    }

    case SV_STAFF_PROBING: {
        ident = probing(creature_ptr);
        break;
    }

    case SV_STAFF_DISPEL_EVIL: {
        ident = dispel_evil(creature_ptr, powerful ? 120 : 80);
        break;
    }

    case SV_STAFF_POWER: {
        ident = dispel_monsters(creature_ptr, powerful ? 225 : 150);
        break;
    }

    case SV_STAFF_HOLINESS: {
        ident = cleansing_nova(creature_ptr, magic, powerful);
        break;
    }

    case SV_STAFF_GENOCIDE: {
        ident = symbol_genocide(creature_ptr, (magic ? lev + 50 : 200), true);
        break;
    }

    case SV_STAFF_EARTHQUAKES: {
        if (earthquake(creature_ptr, creature_ptr->y, creature_ptr->x, (powerful ? 15 : 10), 0))
            ident = true;
        else
            msg_print(_("ダンジョンが揺れた。", "The dungeon trembles."));

        break;
    }

    case SV_STAFF_DESTRUCTION: {
        ident = destroy_area(creature_ptr, creature_ptr->y, creature_ptr->x, (powerful ? 18 : 13) + randint0(5), false);
        break;
    }

    case SV_STAFF_ANIMATE_DEAD: {
        ident = animate_dead(creature_ptr, 0, creature_ptr->y, creature_ptr->x);
        break;
    }

    case SV_STAFF_MSTORM: {
        ident = unleash_mana_storm(creature_ptr, powerful);
        break;
    }

    case SV_STAFF_NOTHING: {
        msg_print(_("何も起らなかった。", "Nothing happens."));
        if (player_race_food(creature_ptr) == PlayerRaceFood::MANA)
            msg_print(_("もったいない事をしたような気がする。食べ物は大切にしなくては。", "What a waste.  It's your food!"));
        break;
    }
    }
    return ident;
}

/*!
 * @brief 杖を使うコマンドのサブルーチン /
 * Use a staff.			-RAK-
 * @param item 使うオブジェクトの所持品ID
 * @details
 * One charge of one staff disappears.
 * Hack -- staffs of identify can be "cancelled".
 */
void exe_use_staff(player_type *creature_ptr, INVENTORY_IDX item)
{
    int ident, chance, lev;
    object_type *o_ptr;

    /* Hack -- let staffs of identify get aborted */
    bool use_charge = true;

    o_ptr = ref_item(creature_ptr, item);

    /* Mega-Hack -- refuse to use a pile from the ground */
    if ((item < 0) && (o_ptr->number > 1)) {
        msg_print(_("まずは杖を拾わなければ。", "You must first pick up the staffs."));
        return;
    }

    PlayerEnergy(creature_ptr).set_player_turn_energy(100);

    lev = k_info[o_ptr->k_idx].level;
    if (lev > 50)
        lev = 50 + (lev - 50) / 2;

    /* Base chance of success */
    chance = creature_ptr->skill_dev;

    /* Confusion hurts skill */
    if (creature_ptr->confused)
        chance = chance / 2;

    /* Hight level objects are harder */
    chance = chance - lev;

    /* Give everyone a (slight) chance */
    if ((chance < USE_DEVICE) && one_in_(USE_DEVICE - chance + 1)) {
        chance = USE_DEVICE;
    }

    if (cmd_limit_time_walk(creature_ptr))
        return;

    /* Roll for usage */
    if ((chance < USE_DEVICE) || (randint1(chance) < USE_DEVICE) || (creature_ptr->pclass == CLASS_BERSERKER)) {
        if (flush_failure)
            flush();
        msg_print(_("杖をうまく使えなかった。", "You failed to use the staff properly."));
        sound(SOUND_FAIL);
        return;
    }

    /* Notice empty staffs */
    if (o_ptr->pval <= 0) {
        if (flush_failure)
            flush();
        msg_print(_("この杖にはもう魔力が残っていない。", "The staff has no charges left."));
        o_ptr->ident |= (IDENT_EMPTY);
        creature_ptr->update |= (PU_COMBINE | PU_REORDER);
        creature_ptr->window_flags |= (PW_INVEN);

        return;
    }

    sound(SOUND_ZAP);

    ident = staff_effect(creature_ptr, o_ptr->sval, &use_charge, false, false, object_is_aware(o_ptr));

    if (!(object_is_aware(o_ptr))) {
        chg_virtue(creature_ptr, V_PATIENCE, -1);
        chg_virtue(creature_ptr, V_CHANCE, 1);
        chg_virtue(creature_ptr, V_KNOWLEDGE, -1);
    }

    /*
     * Temporarily remove the flags for updating the inventory so
     * gain_exp() does not reorder the inventory before the charge
     * is deducted from the staff.
     */
    BIT_FLAGS inventory_flags = (PU_COMBINE | PU_REORDER | (creature_ptr->update & PU_AUTODESTROY));
    reset_bits(creature_ptr->update, PU_COMBINE | PU_REORDER | PU_AUTODESTROY);

    /* Tried the item */
    object_tried(o_ptr);

    /* An identification was made */
    if (ident && !object_is_aware(o_ptr)) {
        object_aware(creature_ptr, o_ptr);
        gain_exp(creature_ptr, (lev + (creature_ptr->lev >> 1)) / creature_ptr->lev);
    }

    set_bits(creature_ptr->window_flags, PW_INVEN | PW_EQUIP | PW_PLAYER | PW_FLOOR_ITEM_LIST);
    set_bits(creature_ptr->update, inventory_flags);

    /* Hack -- some uses are "free" */
    if (!use_charge)
        return;

    /* Use a single charge */
    o_ptr->pval--;

    /* XXX Hack -- unstack if necessary */
    if ((item >= 0) && (o_ptr->number > 1)) {
        object_type forge;
        object_type *q_ptr;
        q_ptr = &forge;
        q_ptr->copy_from(o_ptr);

        /* Modify quantity */
        q_ptr->number = 1;

        /* Restore the charges */
        o_ptr->pval++;

        /* Unstack the used item */
        o_ptr->number--;
        item = store_item_to_inventory(creature_ptr, q_ptr);

        msg_print(_("杖をまとめなおした。", "You unstack your staff."));
    }

    /* Describe charges in the pack */
    if (item >= 0) {
        inven_item_charges(creature_ptr, item);
    }

    /* Describe charges on the floor */
    else {
        floor_item_charges(creature_ptr->current_floor_ptr, 0 - item);
    }
}

/*!
 * @brief 杖を使うコマンドのメインルーチン /
 */
void do_cmd_use_staff(player_type *creature_ptr)
{
    OBJECT_IDX item;
    concptr q, s;

    if (creature_ptr->wild_mode) {
        return;
    }

    if (cmd_limit_arena(creature_ptr))
        return;

    if (creature_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN)) {
        set_action(creature_ptr, ACTION_NONE);
    }

    q = _("どの杖を使いますか? ", "Use which staff? ");
    s = _("使える杖がない。", "You have no staff to use.");
    if (!choose_object(creature_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), TvalItemTester(TV_STAFF)))
        return;

    exe_use_staff(creature_ptr, item);
}
