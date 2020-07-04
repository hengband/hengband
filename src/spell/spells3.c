/*!
 * @brief 魔法効果の実装/ Spell code (part 3)
 * @date 2014/07/26
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * </pre>
 */

#include "spell/spells3.h"
#include "autopick/autopick.h"
#include "cmd-action/cmd-attack.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-building/cmd-building.h"
#include "cmd-io/cmd-dump.h"
#include "core/asking-player.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "effect/spells-effect-util.h"
#include "floor/floor-object.h"
#include "floor/floor-save.h"
#include "floor/floor-town.h"
#include "floor/wild.h"
#include "game-option/auto-destruction-options.h"
#include "game-option/birth-options.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "game-option/play-record-options.h"
#include "grid/grid.h"
#include "inventory/inventory-object.h"
#include "inventory/player-inventory.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "io/targeting.h"
#include "io/write-diary.h"
#include "lore/lore-calculator.h"
#include "market/building-util.h"
#include "mind/mind-force-trainer.h"
#include "mind/mind-sniper.h"
#include "mind/mind.h"
#include "mind/racial-android.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-describer.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-processor.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "monster/monster-util.h"
#include "object-enchant/artifact.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-enchant.h"
#include "object-hook/hook-expendable.h"
#include "object-hook/hook-magic.h"
#include "object-hook/hook-perception.h"
#include "object-hook/hook-weapon.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-flavor.h"
#include "object/object-generator.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "object/object-mark-types.h"
#include "object/object-value.h"
#include "perception/identification.h"
#include "perception/object-perception.h"
#include "player/avatar.h"
#include "player/digestion-processor.h"
#include "player/player-class.h"
#include "player/player-damage.h"
#include "player/player-move.h"
#include "player/player-personalities-types.h"
#include "player/player-skill.h"
#include "player/player-status.h"
#include "realm/realm-names-table.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell/process-effect.h"
#include "spell/spell-types.h"
#include "spell/spells-execution.h"
#include "spell/spells-summon.h"
#include "spell/technic-info-table.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/experience.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief プレイヤーの装備劣化処理 /
 * Apply disenchantment to the player's stuff
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param mode 最下位ビットが1ならば劣化処理が若干低減される
 * @return 劣化処理に関するメッセージが発せられた場合はTRUEを返す /
 * Return "TRUE" if the player notices anything
 */
bool apply_disenchant(player_type *target_ptr, BIT_FLAGS mode)
{
    int t = 0;
    switch (randint1(8)) {
    case 1:
        t = INVEN_RARM;
        break;
    case 2:
        t = INVEN_LARM;
        break;
    case 3:
        t = INVEN_BOW;
        break;
    case 4:
        t = INVEN_BODY;
        break;
    case 5:
        t = INVEN_OUTER;
        break;
    case 6:
        t = INVEN_HEAD;
        break;
    case 7:
        t = INVEN_HANDS;
        break;
    case 8:
        t = INVEN_FEET;
        break;
    }

    object_type *o_ptr;
    o_ptr = &target_ptr->inventory_list[t];
    if (!o_ptr->k_idx)
        return FALSE;

    if (!object_is_weapon_armour_ammo(target_ptr, o_ptr))
        return FALSE;

    if ((o_ptr->to_h <= 0) && (o_ptr->to_d <= 0) && (o_ptr->to_a <= 0) && (o_ptr->pval <= 1)) {
        return FALSE;
    }

    GAME_TEXT o_name[MAX_NLEN];
    object_desc(target_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    if (object_is_artifact(o_ptr) && (randint0(100) < 71)) {
#ifdef JP
        msg_format("%s(%c)は劣化を跳ね返した！", o_name, index_to_label(t));
#else
        msg_format("Your %s (%c) resist%s disenchantment!", o_name, index_to_label(t), ((o_ptr->number != 1) ? "" : "s"));
#endif
        return TRUE;
    }

    int to_h = o_ptr->to_h;
    int to_d = o_ptr->to_d;
    int to_a = o_ptr->to_a;
    int pval = o_ptr->pval;

    if (o_ptr->to_h > 0)
        o_ptr->to_h--;
    if ((o_ptr->to_h > 5) && (randint0(100) < 20))
        o_ptr->to_h--;

    if (o_ptr->to_d > 0)
        o_ptr->to_d--;
    if ((o_ptr->to_d > 5) && (randint0(100) < 20))
        o_ptr->to_d--;

    if (o_ptr->to_a > 0)
        o_ptr->to_a--;
    if ((o_ptr->to_a > 5) && (randint0(100) < 20))
        o_ptr->to_a--;

    if ((o_ptr->pval > 1) && one_in_(13) && !(mode & 0x01))
        o_ptr->pval--;

    bool is_actually_disenchanted = to_h != o_ptr->to_h;
    is_actually_disenchanted |= to_d != o_ptr->to_d;
    is_actually_disenchanted |= to_a != o_ptr->to_a;
    is_actually_disenchanted |= pval != o_ptr->pval;
    if (!is_actually_disenchanted)
        return TRUE;

#ifdef JP
    msg_format("%s(%c)は劣化してしまった！", o_name, index_to_label(t));
#else
    msg_format("Your %s (%c) %s disenchanted!", o_name, index_to_label(t), ((o_ptr->number != 1) ? "were" : "was"));
#endif
    chg_virtue(target_ptr, V_HARMONY, 1);
    chg_virtue(target_ptr, V_ENCHANT, -2);
    target_ptr->update |= (PU_BONUS);
    target_ptr->window |= (PW_EQUIP | PW_PLAYER);

    calc_android_exp(target_ptr);
    return TRUE;
}

/*!
 * @brief アイテム引き寄せ処理 /
 * Fetch an item (teleport it right underneath the caster)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 魔法の発動方向
 * @param wgt 許容重量
 * @param require_los 射線の通りを要求するならばTRUE
 * @return なし
 */
void fetch(player_type *caster_ptr, DIRECTION dir, WEIGHT wgt, bool require_los)
{
    grid_type *g_ptr;
    object_type *o_ptr;
    GAME_TEXT o_name[MAX_NLEN];

    if (caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x].o_idx) {
        msg_print(_("自分の足の下にある物は取れません。", "You can't fetch when you're already standing on something."));
        return;
    }

    POSITION ty, tx;
    if (dir == 5 && target_okay(caster_ptr)) {
        tx = target_col;
        ty = target_row;

        if (distance(caster_ptr->y, caster_ptr->x, ty, tx) > get_max_range(caster_ptr)) {
            msg_print(_("そんなに遠くにある物は取れません！", "You can't fetch something that far away!"));
            return;
        }

        g_ptr = &caster_ptr->current_floor_ptr->grid_array[ty][tx];
        if (!g_ptr->o_idx) {
            msg_print(_("そこには何もありません。", "There is no object at this place."));
            return;
        }

        if (g_ptr->info & CAVE_ICKY) {
            msg_print(_("アイテムがコントロールを外れて落ちた。", "The item slips from your control."));
            return;
        }

        if (require_los) {
            if (!player_has_los_bold(caster_ptr, ty, tx)) {
                msg_print(_("そこはあなたの視界に入っていません。", "You have no direct line of sight to that location."));
                return;
            } else if (!projectable(caster_ptr, caster_ptr->y, caster_ptr->x, ty, tx)) {
                msg_print(_("そこは壁の向こうです。", "You have no direct line of sight to that location."));
                return;
            }
        }
    } else {
        ty = caster_ptr->y;
        tx = caster_ptr->x;
        bool is_first_loop = TRUE;
        g_ptr = &caster_ptr->current_floor_ptr->grid_array[ty][tx];
        while (is_first_loop || !g_ptr->o_idx) {
            is_first_loop = FALSE;
            ty += ddy[dir];
            tx += ddx[dir];
            g_ptr = &caster_ptr->current_floor_ptr->grid_array[ty][tx];

            if ((distance(caster_ptr->y, caster_ptr->x, ty, tx) > get_max_range(caster_ptr))
                || !cave_have_flag_bold(caster_ptr->current_floor_ptr, ty, tx, FF_PROJECT))
                return;
        }
    }

    o_ptr = &caster_ptr->current_floor_ptr->o_list[g_ptr->o_idx];
    if (o_ptr->weight > wgt) {
        msg_print(_("そのアイテムは重過ぎます。", "The object is too heavy."));
        return;
    }

    OBJECT_IDX i = g_ptr->o_idx;
    g_ptr->o_idx = o_ptr->next_o_idx;
    caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x].o_idx = i; /* 'move' it */

    o_ptr->next_o_idx = 0;
    o_ptr->iy = caster_ptr->y;
    o_ptr->ix = caster_ptr->x;

    object_desc(caster_ptr, o_name, o_ptr, OD_NAME_ONLY);
    msg_format(_("%^sがあなたの足元に飛んできた。", "%^s flies through the air to your feet."), o_name);

    note_spot(caster_ptr, caster_ptr->y, caster_ptr->x);
    caster_ptr->redraw |= PR_MAP;
}

/*!
 * @brief 現実変容処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void reserve_alter_reality(player_type *caster_ptr)
{
    if (caster_ptr->current_floor_ptr->inside_arena || ironman_downward) {
        msg_print(_("何も起こらなかった。", "Nothing happens."));
        return;
    }

    if (caster_ptr->alter_reality) {
        caster_ptr->alter_reality = 0;
        msg_print(_("景色が元に戻った...", "The view around you returns to normal..."));
        caster_ptr->redraw |= PR_STATUS;
        return;
    }

    TIME_EFFECT turns = randint0(21) + 15;
    caster_ptr->alter_reality = turns;
    msg_print(_("回りの景色が変わり始めた...", "The view around you begins to change..."));
    caster_ptr->redraw |= PR_STATUS;
}

/*!
 * @brief アイテムの価値に応じた錬金術処理 /
 * Turns an object into gold, gain some of its value in a shop
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 処理が実際に行われたらTRUEを返す
 */
bool alchemy(player_type *caster_ptr)
{
    bool force = FALSE;
    if (command_arg > 0)
        force = TRUE;

    concptr q = _("どのアイテムを金に変えますか？", "Turn which item to gold? ");
    concptr s = _("金に変えられる物がありません。", "You have nothing to turn to gold.");
    OBJECT_IDX item;
    object_type *o_ptr;
    o_ptr = choose_object(caster_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), 0);
    if (!o_ptr)
        return FALSE;

    int amt = 1;
    if (o_ptr->number > 1) {
        amt = get_quantity(NULL, o_ptr->number);
        if (amt <= 0)
            return FALSE;
    }

    ITEM_NUMBER old_number = o_ptr->number;
    o_ptr->number = amt;
    GAME_TEXT o_name[MAX_NLEN];
    object_desc(caster_ptr, o_name, o_ptr, 0);
    o_ptr->number = old_number;

    if (!force) {
        if (confirm_destroy || (object_value(caster_ptr, o_ptr) > 0)) {
            char out_val[MAX_NLEN + 40];
            sprintf(out_val, _("本当に%sを金に変えますか？", "Really turn %s to gold? "), o_name);
            if (!get_check(out_val))
                return FALSE;
        }
    }

    if (!can_player_destroy_object(caster_ptr, o_ptr)) {
        msg_format(_("%sを金に変えることに失敗した。", "You fail to turn %s to gold!"), o_name);
        return FALSE;
    }

    PRICE price = object_value_real(caster_ptr, o_ptr);
    if (price <= 0) {
        msg_format(_("%sをニセの金に変えた。", "You turn %s to fool's gold."), o_name);
        vary_item(caster_ptr, item, -amt);
        return TRUE;
    }

    price /= 3;

    if (amt > 1)
        price *= amt;

    if (price > 30000)
        price = 30000;
    msg_format(_("%sを＄%d の金に変えた。", "You turn %s to %ld coins worth of gold."), o_name, price);

    caster_ptr->au += price;
    caster_ptr->redraw |= PR_GOLD;
    caster_ptr->window |= PW_PLAYER;
    vary_item(caster_ptr, item, -amt);
    return TRUE;
}

/*!
 * @brief アーティファクト生成の巻物処理 /
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 生成が実際に試みられたらTRUEを返す
 */
bool artifact_scroll(player_type *caster_ptr)
{
    item_tester_hook = item_tester_hook_nameless_weapon_armour;

    concptr q = _("どのアイテムを強化しますか? ", "Enchant which item? ");
    concptr s = _("強化できるアイテムがない。", "You have nothing to enchant.");
    object_type *o_ptr;
    OBJECT_IDX item;
    o_ptr = choose_object(caster_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);
    if (!o_ptr)
        return FALSE;

    GAME_TEXT o_name[MAX_NLEN];
    object_desc(caster_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
#ifdef JP
    msg_format("%s は眩い光を発した！", o_name);
#else
    msg_format("%s %s radiate%s a blinding light!", ((item >= 0) ? "Your" : "The"), o_name, ((o_ptr->number > 1) ? "" : "s"));
#endif

    bool okay = FALSE;
    if (object_is_artifact(o_ptr)) {
#ifdef JP
        msg_format("%sは既に伝説のアイテムです！", o_name);
#else
        msg_format("The %s %s already %s!", o_name, ((o_ptr->number > 1) ? "are" : "is"), ((o_ptr->number > 1) ? "artifacts" : "an artifact"));
#endif
        okay = FALSE;
    } else if (object_is_ego(o_ptr)) {
#ifdef JP
        msg_format("%sは既に名のあるアイテムです！", o_name);
#else
        msg_format("The %s %s already %s!", o_name, ((o_ptr->number > 1) ? "are" : "is"), ((o_ptr->number > 1) ? "ego items" : "an ego item"));
#endif
        okay = FALSE;
    } else if (o_ptr->xtra3) {
#ifdef JP
        msg_format("%sは既に強化されています！", o_name);
#else
        msg_format("The %s %s already %s!", o_name, ((o_ptr->number > 1) ? "are" : "is"), ((o_ptr->number > 1) ? "customized items" : "a customized item"));
#endif
    } else {
        if (o_ptr->number > 1) {
            msg_print(_("複数のアイテムに魔法をかけるだけのエネルギーはありません！", "Not enough energy to enchant more than one object!"));
#ifdef JP
            msg_format("%d 個の%sが壊れた！", (o_ptr->number) - 1, o_name);
#else
            msg_format("%d of your %s %s destroyed!", (o_ptr->number) - 1, o_name, (o_ptr->number > 2 ? "were" : "was"));
#endif

            if (item >= 0) {
                inven_item_increase(caster_ptr, item, 1 - (o_ptr->number));
            } else {
                floor_item_increase(caster_ptr->current_floor_ptr, 0 - item, 1 - (o_ptr->number));
            }
        }

        okay = become_random_artifact(caster_ptr, o_ptr, TRUE);
    }

    if (!okay) {
        if (flush_failure)
            flush();
        msg_print(_("強化に失敗した。", "The enchantment failed."));
        if (one_in_(3))
            chg_virtue(caster_ptr, V_ENCHANT, -1);
        calc_android_exp(caster_ptr);
        return TRUE;
    }

    if (record_rand_art) {
        object_desc(caster_ptr, o_name, o_ptr, OD_NAME_ONLY);
        exe_write_diary(caster_ptr, DIARY_ART_SCROLL, 0, o_name);
    }

    chg_virtue(caster_ptr, V_ENCHANT, 1);
    calc_android_exp(caster_ptr);
    return TRUE;
}

/*!
 * @brief アイテム凡庸化のメインルーチン処理 /
 * Identify an object in the inventory (or on the floor)
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param only_equip 装備品のみを対象とするならばTRUEを返す
 * @return 実際に凡庸化をを行ったならばTRUEを返す
 * @details
 * <pre>
 * Mundanify an object in the inventory (or on the floor)
 * This routine does *not* automatically combine objects.
 * Returns TRUE if something was mundanified, else FALSE.
 * </pre>
 */
bool mundane_spell(player_type *owner_ptr, bool only_equip)
{
    if (only_equip)
        item_tester_hook = object_is_weapon_armour_ammo;

    OBJECT_IDX item;
    object_type *o_ptr;
    concptr q = _("どれを使いますか？", "Use which item? ");
    concptr s = _("使えるものがありません。", "You have nothing you can use.");

    o_ptr = choose_object(owner_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);
    if (!o_ptr)
        return FALSE;

    msg_print(_("まばゆい閃光が走った！", "There is a bright flash of light!"));
    POSITION iy = o_ptr->iy;
    POSITION ix = o_ptr->ix;
    OBJECT_IDX next_o_idx = o_ptr->next_o_idx;
    byte marked = o_ptr->marked;
    WEIGHT weight = o_ptr->number * o_ptr->weight;
    u16b inscription = o_ptr->inscription;

    object_prep(owner_ptr, o_ptr, o_ptr->k_idx);

    o_ptr->iy = iy;
    o_ptr->ix = ix;
    o_ptr->next_o_idx = next_o_idx;
    o_ptr->marked = marked;
    o_ptr->inscription = inscription;
    if (item >= 0)
        owner_ptr->total_weight += (o_ptr->weight - weight);

    calc_android_exp(owner_ptr);
    return TRUE;
}

/*!
 * @brief 魔力充填処理 /
 * Recharge a wand/staff/rod from the pack or on the floor.
 * This function has been rewritten in Oangband and ZAngband.
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param power 充填パワー
 * @return ターン消費を要する処理まで進んだらTRUEを返す
 *
 * Sorcery/Arcane -- Recharge  --> recharge(plev * 4)
 * Chaos -- Arcane Binding     --> recharge(90)
 *
 * Scroll of recharging        --> recharge(130)
 * Artifact activation/Thingol --> recharge(130)
 *
 * It is harder to recharge high level, and highly charged wands,
 * staffs, and rods.  The more wands in a stack, the more easily and
 * strongly they recharge.  Staffs, however, each get fewer charges if
 * stacked.
 *
 * Beware of "sliding index errors".
 */
bool recharge(player_type *caster_ptr, int power)
{
    item_tester_hook = item_tester_hook_recharge;
    concptr q = _("どのアイテムに魔力を充填しますか? ", "Recharge which item? ");
    concptr s = _("魔力を充填すべきアイテムがない。", "You have nothing to recharge.");

    OBJECT_IDX item;
    object_type *o_ptr;
    o_ptr = choose_object(caster_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), 0);
    if (!o_ptr)
        return FALSE;

    object_kind *k_ptr;
    k_ptr = &k_info[o_ptr->k_idx];
    DEPTH lev = k_info[o_ptr->k_idx].level;

    TIME_EFFECT recharge_amount;
    int recharge_strength;
    bool is_recharge_successful = TRUE;
    if (o_ptr->tval == TV_ROD) {
        recharge_strength = ((power > lev / 2) ? (power - lev / 2) : 0) / 5;
        if (one_in_(recharge_strength)) {
            is_recharge_successful = FALSE;
        } else {
            recharge_amount = (power * damroll(3, 2));
            if (o_ptr->timeout > recharge_amount)
                o_ptr->timeout -= recharge_amount;
            else
                o_ptr->timeout = 0;
        }
    } else {
        if ((o_ptr->tval == TV_WAND) && (o_ptr->number > 1))
            recharge_strength = (100 + power - lev - (8 * o_ptr->pval / o_ptr->number)) / 15;
        else
            recharge_strength = (100 + power - lev - (8 * o_ptr->pval)) / 15;

        if (recharge_strength < 0)
            recharge_strength = 0;

        if (one_in_(recharge_strength)) {
            is_recharge_successful = FALSE;
        } else {
            recharge_amount = randint1(1 + k_ptr->pval / 2);
            if ((o_ptr->tval == TV_WAND) && (o_ptr->number > 1)) {
                recharge_amount += (randint1(recharge_amount * (o_ptr->number - 1))) / 2;
                if (recharge_amount < 1)
                    recharge_amount = 1;
                if (recharge_amount > 12)
                    recharge_amount = 12;
            }

            if ((o_ptr->tval == TV_STAFF) && (o_ptr->number > 1)) {
                recharge_amount /= (TIME_EFFECT)o_ptr->number;
                if (recharge_amount < 1)
                    recharge_amount = 1;
            }

            o_ptr->pval += recharge_amount;
            o_ptr->ident &= ~(IDENT_KNOWN);
            o_ptr->ident &= ~(IDENT_EMPTY);
        }
    }

    if (!is_recharge_successful) {
        return update_player(caster_ptr);
    }

    byte fail_type = 1;
    GAME_TEXT o_name[MAX_NLEN];
    if (object_is_fixed_artifact(o_ptr)) {
        object_desc(caster_ptr, o_name, o_ptr, OD_NAME_ONLY);
        msg_format(_("魔力が逆流した！%sは完全に魔力を失った。", "The recharging backfires - %s is completely drained!"), o_name);
        if ((o_ptr->tval == TV_ROD) && (o_ptr->timeout < 10000))
            o_ptr->timeout = (o_ptr->timeout + 100) * 2;
        else if ((o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_STAFF))
            o_ptr->pval = 0;
        return update_player(caster_ptr);
    }

    object_desc(caster_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

    if (IS_WIZARD_CLASS(caster_ptr) || caster_ptr->pclass == CLASS_MAGIC_EATER || caster_ptr->pclass == CLASS_BLUE_MAGE) {
        /* 10% chance to blow up one rod, otherwise draining. */
        if (o_ptr->tval == TV_ROD) {
            if (one_in_(10))
                fail_type = 2;
            else
                fail_type = 1;
        }
        /* 75% chance to blow up one wand, otherwise draining. */
        else if (o_ptr->tval == TV_WAND) {
            if (!one_in_(3))
                fail_type = 2;
            else
                fail_type = 1;
        }
        /* 50% chance to blow up one staff, otherwise no effect. */
        else if (o_ptr->tval == TV_STAFF) {
            if (one_in_(2))
                fail_type = 2;
            else
                fail_type = 0;
        }
    } else {
        /* 33% chance to blow up one rod, otherwise draining. */
        if (o_ptr->tval == TV_ROD) {
            if (one_in_(3))
                fail_type = 2;
            else
                fail_type = 1;
        }
        /* 20% chance of the entire stack, else destroy one wand. */
        else if (o_ptr->tval == TV_WAND) {
            if (one_in_(5))
                fail_type = 3;
            else
                fail_type = 2;
        }
        /* Blow up one staff. */
        else if (o_ptr->tval == TV_STAFF) {
            fail_type = 2;
        }
    }

    if (fail_type == 1) {
        if (o_ptr->tval == TV_ROD) {
            msg_print(_("魔力が逆噴射して、ロッドからさらに魔力を吸い取ってしまった！", "The recharge backfires, draining the rod further!"));

            if (o_ptr->timeout < 10000)
                o_ptr->timeout = (o_ptr->timeout + 100) * 2;
        } else if (o_ptr->tval == TV_WAND) {
            msg_format(_("%sは破損を免れたが、魔力が全て失われた。", "You save your %s from destruction, but all charges are lost."), o_name);
            o_ptr->pval = 0;
        }
    }

    if (fail_type == 2) {
        if (o_ptr->number > 1)
            msg_format(_("乱暴な魔法のために%sが一本壊れた！", "Wild magic consumes one of your %s!"), o_name);
        else
            msg_format(_("乱暴な魔法のために%sが壊れた！", "Wild magic consumes your %s!"), o_name);

        if (o_ptr->tval == TV_ROD)
            o_ptr->timeout = (o_ptr->number - 1) * k_ptr->pval;
        if (o_ptr->tval == TV_WAND)
            o_ptr->pval = 0;

        vary_item(caster_ptr, item, -1);
    }

    if (fail_type == 3) {
        if (o_ptr->number > 1)
            msg_format(_("乱暴な魔法のために%sが全て壊れた！", "Wild magic consumes all your %s!"), o_name);
        else
            msg_format(_("乱暴な魔法のために%sが壊れた！", "Wild magic consumes your %s!"), o_name);

        vary_item(caster_ptr, item, -999);
    }

    return update_player(caster_ptr);
}

/*!
 * @brief クリーチャー全既知呪文を表示する /
 * Hack -- Display all known spells in a window
 * @param caster_ptr 術者の参照ポインタ
 * return なし
 * @details
 * Need to analyze size of the window.
 * Need more color coding.
 */
void display_spell_list(player_type *caster_ptr)
{
    TERM_LEN y, x;
    int m[9];
    const magic_type *s_ptr;
    GAME_TEXT name[MAX_NLEN];
    char out_val[160];

    clear_from(0);

    if (caster_ptr->pclass == CLASS_SORCERER)
        return;
    if (caster_ptr->pclass == CLASS_RED_MAGE)
        return;
    if (caster_ptr->pclass == CLASS_SNIPER) {
        display_snipe_list(caster_ptr);
        return;
    }

    if ((caster_ptr->pclass == CLASS_MINDCRAFTER) || (caster_ptr->pclass == CLASS_BERSERKER) || (caster_ptr->pclass == CLASS_NINJA)
        || (caster_ptr->pclass == CLASS_MIRROR_MASTER) || (caster_ptr->pclass == CLASS_FORCETRAINER)) {
        PERCENTAGE minfail = 0;
        PLAYER_LEVEL plev = caster_ptr->lev;
        PERCENTAGE chance = 0;
        mind_type spell;
        char comment[80];
        char psi_desc[80];
        int use_mind;
        bool use_hp = FALSE;

        y = 1;
        x = 1;

        prt("", y, x);
        put_str(_("名前", "Name"), y, x + 5);
        put_str(_("Lv   MP 失率 効果", "Lv Mana Fail Info"), y, x + 35);

        switch (caster_ptr->pclass) {
        case CLASS_MINDCRAFTER:
            use_mind = MIND_MINDCRAFTER;
            break;
        case CLASS_FORCETRAINER:
            use_mind = MIND_KI;
            break;
        case CLASS_BERSERKER:
            use_mind = MIND_BERSERKER;
            use_hp = TRUE;
            break;
        case CLASS_MIRROR_MASTER:
            use_mind = MIND_MIRROR_MASTER;
            break;
        case CLASS_NINJA:
            use_mind = MIND_NINJUTSU;
            use_hp = TRUE;
            break;
        default:
            use_mind = 0;
            break;
        }

        for (int i = 0; i < MAX_MIND_POWERS; i++) {
            byte a = TERM_WHITE;
            spell = mind_powers[use_mind].info[i];
            if (spell.min_lev > plev)
                break;

            chance = spell.fail;
            chance -= 3 * (caster_ptr->lev - spell.min_lev);
            chance -= 3 * (adj_mag_stat[caster_ptr->stat_ind[mp_ptr->spell_stat]] - 1);
            if (!use_hp) {
                if (spell.mana_cost > caster_ptr->csp) {
                    chance += 5 * (spell.mana_cost - caster_ptr->csp);
                    a = TERM_ORANGE;
                }
            } else {
                if (spell.mana_cost > caster_ptr->chp) {
                    chance += 100;
                    a = TERM_RED;
                }
            }

            minfail = adj_mag_fail[caster_ptr->stat_ind[mp_ptr->spell_stat]];
            if (chance < minfail)
                chance = minfail;

            if (caster_ptr->stun > 50)
                chance += 25;
            else if (caster_ptr->stun)
                chance += 15;

            if (chance > 95)
                chance = 95;

            mindcraft_info(caster_ptr, comment, use_mind, i);
            sprintf(psi_desc, "  %c) %-30s%2d %4d %3d%%%s", I2A(i), spell.name, spell.min_lev, spell.mana_cost, chance, comment);

            Term_putstr(x, y + i + 1, -1, a, psi_desc);
        }

        return;
    }

    if (REALM_NONE == caster_ptr->realm1)
        return;

    for (int j = 0; j < ((caster_ptr->realm2 > REALM_NONE) ? 2 : 1); j++) {
        m[j] = 0;
        y = (j < 3) ? 0 : (m[j - 3] + 2);
        x = 27 * (j % 3);
        int n = 0;
        for (int i = 0; i < 32; i++) {
            byte a = TERM_WHITE;

            if (!is_magic((j < 1) ? caster_ptr->realm1 : caster_ptr->realm2)) {
                s_ptr = &technic_info[((j < 1) ? caster_ptr->realm1 : caster_ptr->realm2) - MIN_TECHNIC][i % 32];
            } else {
                s_ptr = &mp_ptr->info[((j < 1) ? caster_ptr->realm1 : caster_ptr->realm2) - 1][i % 32];
            }

            strcpy(name, exe_spell(caster_ptr, (j < 1) ? caster_ptr->realm1 : caster_ptr->realm2, i % 32, SPELL_NAME));

            if (s_ptr->slevel >= 99) {
                strcpy(name, _("(判読不能)", "(illegible)"));
                a = TERM_L_DARK;
            } else if ((j < 1) ? ((caster_ptr->spell_forgotten1 & (1L << i))) : ((caster_ptr->spell_forgotten2 & (1L << (i % 32))))) {
                a = TERM_ORANGE;
            } else if (!((j < 1) ? (caster_ptr->spell_learned1 & (1L << i)) : (caster_ptr->spell_learned2 & (1L << (i % 32))))) {
                a = TERM_RED;
            } else if (!((j < 1) ? (caster_ptr->spell_worked1 & (1L << i)) : (caster_ptr->spell_worked2 & (1L << (i % 32))))) {
                a = TERM_YELLOW;
            }

            sprintf(out_val, "%c/%c) %-20.20s", I2A(n / 8), I2A(n % 8), name);

            m[j] = y + n;
            Term_putstr(x, m[j], -1, a, out_val);
            n++;
        }
    }
}

/*!
 * @brief 変身処理向けにモンスターの近隣レベル帯モンスターを返す /
 * Helper function -- return a "nearby" race for polymorphing
 * @param floor_ptr 配置するフロアの参照ポインタ
 * @param r_idx 基準となるモンスター種族ID
 * @return 変更先のモンスター種族ID
 * @details
 * Note that this function is one of the more "dangerous" ones...
 */
static MONRACE_IDX poly_r_idx(player_type *caster_ptr, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    if ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags1 & RF1_QUESTOR))
        return (r_idx);

    DEPTH lev1 = r_ptr->level - ((randint1(20) / randint1(9)) + 1);
    DEPTH lev2 = r_ptr->level + ((randint1(20) / randint1(9)) + 1);
    MONRACE_IDX r;
    for (int i = 0; i < 1000; i++) {
        r = get_mon_num(caster_ptr, (caster_ptr->current_floor_ptr->dun_level + r_ptr->level) / 2 + 5, 0);
        if (!r)
            break;

        r_ptr = &r_info[r];
        if (r_ptr->flags1 & RF1_UNIQUE)
            continue;
        if ((r_ptr->level < lev1) || (r_ptr->level > lev2))
            continue;

        r_idx = r;
        break;
    }

    return r_idx;
}

/*!
 * @brief 指定座標にいるモンスターを変身させる /
 * Helper function -- return a "nearby" race for polymorphing
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param y 指定のY座標
 * @param x 指定のX座標
 * @return 実際に変身したらTRUEを返す
 */
bool polymorph_monster(player_type *caster_ptr, POSITION y, POSITION x)
{
    floor_type *floor_ptr = caster_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];
    monster_type *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
    MONRACE_IDX new_r_idx;
    MONRACE_IDX old_r_idx = m_ptr->r_idx;
    bool targeted = (target_who == g_ptr->m_idx) ? TRUE : FALSE;
    bool health_tracked = (caster_ptr->health_who == g_ptr->m_idx) ? TRUE : FALSE;

    if (floor_ptr->inside_arena || caster_ptr->phase_out)
        return FALSE;
    if ((caster_ptr->riding == g_ptr->m_idx) || (m_ptr->mflag2 & MFLAG2_KAGE))
        return FALSE;

    monster_type back_m = *m_ptr;
    new_r_idx = poly_r_idx(caster_ptr, old_r_idx);
    if (new_r_idx == old_r_idx)
        return FALSE;

    bool preserve_hold_objects = back_m.hold_o_idx ? TRUE : FALSE;
    OBJECT_IDX this_o_idx, next_o_idx = 0;

    BIT_FLAGS mode = 0L;
    if (is_friendly(m_ptr))
        mode |= PM_FORCE_FRIENDLY;
    if (is_pet(m_ptr))
        mode |= PM_FORCE_PET;
    if (m_ptr->mflag2 & MFLAG2_NOPET)
        mode |= PM_NO_PET;

    m_ptr->hold_o_idx = 0;
    delete_monster_idx(caster_ptr, g_ptr->m_idx);
    bool polymorphed = FALSE;
    if (place_monster_aux(caster_ptr, 0, y, x, new_r_idx, mode)) {
        floor_ptr->m_list[hack_m_idx_ii].nickname = back_m.nickname;
        floor_ptr->m_list[hack_m_idx_ii].parent_m_idx = back_m.parent_m_idx;
        floor_ptr->m_list[hack_m_idx_ii].hold_o_idx = back_m.hold_o_idx;
        polymorphed = TRUE;
    } else {
        if (place_monster_aux(caster_ptr, 0, y, x, old_r_idx, (mode | PM_NO_KAGE | PM_IGNORE_TERRAIN))) {
            floor_ptr->m_list[hack_m_idx_ii] = back_m;
            mproc_init(floor_ptr);
        } else
            preserve_hold_objects = FALSE;
    }

    if (preserve_hold_objects) {
        for (this_o_idx = back_m.hold_o_idx; this_o_idx; this_o_idx = next_o_idx) {
            object_type *o_ptr = &floor_ptr->o_list[this_o_idx];
            next_o_idx = o_ptr->next_o_idx;
            o_ptr->held_m_idx = hack_m_idx_ii;
        }
    } else if (back_m.hold_o_idx) {
        for (this_o_idx = back_m.hold_o_idx; this_o_idx; this_o_idx = next_o_idx) {
            next_o_idx = floor_ptr->o_list[this_o_idx].next_o_idx;
            delete_object_idx(caster_ptr, this_o_idx);
        }
    }

    if (targeted)
        target_who = hack_m_idx_ii;
    if (health_tracked)
        health_track(caster_ptr, hack_m_idx_ii);
    return polymorphed;
}

/*!
 * @brief 皆殺し(全方向攻撃)処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void massacre(player_type *caster_ptr)
{
    grid_type *g_ptr;
    monster_type *m_ptr;
    for (DIRECTION dir = 0; dir < 8; dir++) {
        POSITION y = caster_ptr->y + ddy_ddd[dir];
        POSITION x = caster_ptr->x + ddx_ddd[dir];
        g_ptr = &caster_ptr->current_floor_ptr->grid_array[y][x];
        m_ptr = &caster_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
        if (g_ptr->m_idx && (m_ptr->ml || cave_have_flag_bold(caster_ptr->current_floor_ptr, y, x, FF_PROJECT)))
            do_cmd_attack(caster_ptr, y, x, 0);
    }
}

/*!
 * 岩石食い
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return コマンドの入力方向に地形があればTRUE
 */
bool eat_rock(player_type *caster_ptr)
{
    DIRECTION dir;
    if (!get_direction(caster_ptr, &dir, FALSE, FALSE))
        return FALSE;
    POSITION y = caster_ptr->y + ddy[dir];
    POSITION x = caster_ptr->x + ddx[dir];
    grid_type *g_ptr;
    g_ptr = &caster_ptr->current_floor_ptr->grid_array[y][x];
    feature_type *f_ptr, *mimic_f_ptr;
    f_ptr = &f_info[g_ptr->feat];
    mimic_f_ptr = &f_info[get_feat_mimic(g_ptr)];

    stop_mouth(caster_ptr);
    if (!have_flag(mimic_f_ptr->flags, FF_HURT_ROCK)) {
        msg_print(_("この地形は食べられない。", "You cannot eat this feature."));
    } else if (have_flag(f_ptr->flags, FF_PERMANENT)) {
        msg_format(_("いてっ！この%sはあなたの歯より硬い！", "Ouch!  This %s is harder than your teeth!"), f_name + mimic_f_ptr->name);
    } else if (g_ptr->m_idx) {
        monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
        msg_print(_("何かが邪魔しています！", "There's something in the way!"));

        if (!m_ptr->ml || !is_pet(m_ptr))
            do_cmd_attack(caster_ptr, y, x, 0);
    } else if (have_flag(f_ptr->flags, FF_TREE)) {
        msg_print(_("木の味は好きじゃない！", "You don't like the woody taste!"));
    } else if (have_flag(f_ptr->flags, FF_GLASS)) {
        msg_print(_("ガラスの味は好きじゃない！", "You don't like the glassy taste!"));
    } else if (have_flag(f_ptr->flags, FF_DOOR) || have_flag(f_ptr->flags, FF_CAN_DIG)) {
        (void)set_food(caster_ptr, caster_ptr->food + 3000);
    } else if (have_flag(f_ptr->flags, FF_MAY_HAVE_GOLD) || have_flag(f_ptr->flags, FF_HAS_GOLD)) {
        (void)set_food(caster_ptr, caster_ptr->food + 5000);
    } else {
        msg_format(_("この%sはとてもおいしい！", "This %s is very filling!"), f_name + mimic_f_ptr->name);
        (void)set_food(caster_ptr, caster_ptr->food + 10000);
    }

    cave_alter_feat(caster_ptr, y, x, FF_HURT_ROCK);
    (void)move_player_effect(caster_ptr, y, x, MPE_DONT_PICKUP);
    return TRUE;
}

bool shock_power(player_type *caster_ptr)
{
    int boost = get_current_ki(caster_ptr);
    if (heavy_armor(caster_ptr))
        boost /= 2;

    project_length = 1;
    DIRECTION dir;
    if (!get_aim_dir(caster_ptr, &dir))
        return FALSE;

    POSITION y = caster_ptr->y + ddy[dir];
    POSITION x = caster_ptr->x + ddx[dir];
    PLAYER_LEVEL plev = caster_ptr->lev;
    HIT_POINT dam = damroll(8 + ((plev - 5) / 4) + boost / 12, 8);
    fire_beam(caster_ptr, GF_MISSILE, dir, dam);
    if (!caster_ptr->current_floor_ptr->grid_array[y][x].m_idx)
        return TRUE;

    POSITION ty = y, tx = x;
    POSITION oy = y, ox = x;
    MONSTER_IDX m_idx = caster_ptr->current_floor_ptr->grid_array[y][x].m_idx;
    monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    GAME_TEXT m_name[MAX_NLEN];
    monster_desc(caster_ptr, m_name, m_ptr, 0);

    if (randint1(r_ptr->level * 3 / 2) > randint0(dam / 2) + dam / 2) {
        msg_format(_("%sは飛ばされなかった。", "%^s was not blown away."), m_name);
        return TRUE;
    }

    for (int i = 0; i < 5; i++) {
        y += ddy[dir];
        x += ddx[dir];
        if (is_cave_empty_bold(caster_ptr, y, x)) {
            ty = y;
            tx = x;
        } else {
            break;
        }
    }

    bool is_shock_successful = ty != oy;
    is_shock_successful |= tx != ox;
    if (is_shock_successful)
        return TRUE;

    msg_format(_("%sを吹き飛ばした！", "You blow %s away!"), m_name);
    caster_ptr->current_floor_ptr->grid_array[oy][ox].m_idx = 0;
    caster_ptr->current_floor_ptr->grid_array[ty][tx].m_idx = m_idx;
    m_ptr->fy = ty;
    m_ptr->fx = tx;

    update_monster(caster_ptr, m_idx, TRUE);
    lite_spot(caster_ptr, oy, ox);
    lite_spot(caster_ptr, ty, tx);

    if (r_ptr->flags7 & (RF7_LITE_MASK | RF7_DARK_MASK))
        caster_ptr->update |= (PU_MON_LITE);
    return TRUE;
}

bool fetch_monster(player_type *caster_ptr)
{
    monster_type *m_ptr;
    MONSTER_IDX m_idx;
    GAME_TEXT m_name[MAX_NLEN];
    int i;
    int path_n;
    u16b path_g[512];
    POSITION ty, tx;

    if (!target_set(caster_ptr, TARGET_KILL))
        return FALSE;
    m_idx = caster_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx;
    if (!m_idx)
        return FALSE;
    if (m_idx == caster_ptr->riding)
        return FALSE;
    if (!player_has_los_bold(caster_ptr, target_row, target_col))
        return FALSE;
    if (!projectable(caster_ptr, caster_ptr->y, caster_ptr->x, target_row, target_col))
        return FALSE;
    m_ptr = &caster_ptr->current_floor_ptr->m_list[m_idx];
    monster_desc(caster_ptr, m_name, m_ptr, 0);
    msg_format(_("%sを引き戻した。", "You pull back %s."), m_name);
    path_n = project_path(caster_ptr, path_g, get_max_range(caster_ptr), target_row, target_col, caster_ptr->y, caster_ptr->x, 0);
    ty = target_row, tx = target_col;
    for (i = 1; i < path_n; i++) {
        POSITION ny = GRID_Y(path_g[i]);
        POSITION nx = GRID_X(path_g[i]);
        grid_type *g_ptr = &caster_ptr->current_floor_ptr->grid_array[ny][nx];

        if (in_bounds(caster_ptr->current_floor_ptr, ny, nx) && is_cave_empty_bold(caster_ptr, ny, nx) && !(g_ptr->info & CAVE_OBJECT)
            && !pattern_tile(caster_ptr->current_floor_ptr, ny, nx)) {
            ty = ny;
            tx = nx;
        }
    }

    caster_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx = 0;
    caster_ptr->current_floor_ptr->grid_array[ty][tx].m_idx = m_idx;
    m_ptr->fy = ty;
    m_ptr->fx = tx;
    (void)set_monster_csleep(caster_ptr, m_idx, 0);
    update_monster(caster_ptr, m_idx, TRUE);
    lite_spot(caster_ptr, target_row, target_col);
    lite_spot(caster_ptr, ty, tx);
    if (r_info[m_ptr->r_idx].flags7 & (RF7_LITE_MASK | RF7_DARK_MASK))
        caster_ptr->update |= (PU_MON_LITE);

    if (m_ptr->ml) {
        if (!caster_ptr->image)
            monster_race_track(caster_ptr, m_ptr->ap_r_idx);

        health_track(caster_ptr, m_idx);
    }

    return TRUE;
}

bool booze(player_type *creature_ptr)
{
    bool ident = FALSE;
    if (creature_ptr->pclass != CLASS_MONK)
        chg_virtue(creature_ptr, V_HARMONY, -1);
    else if (!creature_ptr->resist_conf)
        creature_ptr->special_attack |= ATTACK_SUIKEN;
    if (!creature_ptr->resist_conf && set_confused(creature_ptr, randint0(20) + 15)) {
        ident = TRUE;
    }

    if (creature_ptr->resist_chaos) {
        return ident;
    }

    if (one_in_(2) && set_image(creature_ptr, creature_ptr->image + randint0(150) + 150)) {
        ident = TRUE;
    }

    if (one_in_(13) && (creature_ptr->pclass != CLASS_MONK)) {
        ident = TRUE;
        if (one_in_(3))
            lose_all_info(creature_ptr);
        else
            wiz_dark(creature_ptr);
        (void)teleport_player_aux(creature_ptr, 100, FALSE, TELEPORT_NONMAGICAL | TELEPORT_PASSIVE);
        wiz_dark(creature_ptr);
        msg_print(_("知らない場所で目が醒めた。頭痛がする。", "You wake up somewhere with a sore head..."));
        msg_print(_("何も思い出せない。どうやってここへ来たのかも分からない！", "You can't remember a thing or how you got here!"));
    }

    return ident;
}

/*!
 * @brief 爆発の薬の効果処理 / Fumble ramble
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return 常にTRUE
 */
bool detonation(player_type *creature_ptr)
{
    msg_print(_("体の中で激しい爆発が起きた！", "Massive explosions rupture your body!"));
    take_hit(creature_ptr, DAMAGE_NOESCAPE, damroll(50, 20), _("爆発の薬", "a potion of Detonation"), -1);
    (void)set_stun(creature_ptr, creature_ptr->stun + 75);
    (void)set_cut(creature_ptr, creature_ptr->cut + 5000);
    return TRUE;
}

void blood_curse_to_enemy(player_type *caster_ptr, MONSTER_IDX m_idx)
{
    monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[m_idx];
    grid_type *g_ptr = &caster_ptr->current_floor_ptr->grid_array[m_ptr->fy][m_ptr->fx];
    BIT_FLAGS curse_flg = (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP);
    int count = 0;
    bool is_first_loop = TRUE;
    while (is_first_loop || one_in_(5)) {
        is_first_loop = FALSE;
        switch (randint1(28)) {
        case 1:
        case 2:
            if (!count) {
                msg_print(_("地面が揺れた...", "The ground trembles..."));
                earthquake(caster_ptr, m_ptr->fy, m_ptr->fx, 4 + randint0(4), 0);
                if (!one_in_(6))
                    break;
            }
            /* Fall through */
        case 3:
        case 4:
        case 5:
        case 6:
            if (!count) {
                int extra_dam = damroll(10, 10);
                msg_print(_("純粋な魔力の次元への扉が開いた！", "A portal opens to a plane of raw mana!"));

                project(caster_ptr, 0, 8, m_ptr->fy, m_ptr->fx, extra_dam, GF_MANA, curse_flg, -1);
                if (!one_in_(6))
                    break;
            }
            /* Fall through */
        case 7:
        case 8:
            if (!count) {
                msg_print(_("空間が歪んだ！", "Space warps about you!"));

                if (m_ptr->r_idx)
                    teleport_away(caster_ptr, g_ptr->m_idx, damroll(10, 10), TELEPORT_PASSIVE);
                if (one_in_(13))
                    count += activate_hi_summon(caster_ptr, m_ptr->fy, m_ptr->fx, TRUE);
                if (!one_in_(6))
                    break;
            }
            /* Fall through */
        case 9:
        case 10:
        case 11:
            msg_print(_("エネルギーのうねりを感じた！", "You feel a surge of energy!"));
            project(caster_ptr, 0, 7, m_ptr->fy, m_ptr->fx, 50, GF_DISINTEGRATE, curse_flg, -1);
            if (!one_in_(6))
                break;
            /* Fall through */
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
            aggravate_monsters(caster_ptr, 0);
            if (!one_in_(6))
                break;
            /* Fall through */
        case 17:
        case 18:
            count += activate_hi_summon(caster_ptr, m_ptr->fy, m_ptr->fx, TRUE);
            if (!one_in_(6))
                break;
            /* Fall through */
        case 19:
        case 20:
        case 21:
        case 22: {
            bool pet = !one_in_(3);
            BIT_FLAGS mode = PM_ALLOW_GROUP;

            if (pet)
                mode |= PM_FORCE_PET;
            else
                mode |= (PM_NO_PET | PM_FORCE_FRIENDLY);

            count += summon_specific(caster_ptr, (pet ? -1 : 0), caster_ptr->y, caster_ptr->x,
                (pet ? caster_ptr->lev * 2 / 3 + randint1(caster_ptr->lev / 2) : caster_ptr->current_floor_ptr->dun_level), 0, mode);
            if (!one_in_(6))
                break;
        }
            /* Fall through */
        case 23:
        case 24:
        case 25:
            if (caster_ptr->hold_exp && (randint0(100) < 75))
                break;
            msg_print(_("経験値が体から吸い取られた気がする！", "You feel your experience draining away..."));

            if (caster_ptr->hold_exp)
                lose_exp(caster_ptr, caster_ptr->exp / 160);
            else
                lose_exp(caster_ptr, caster_ptr->exp / 16);
            if (!one_in_(6))
                break;
            /* Fall through */
        case 26:
        case 27:
        case 28: {
            if (one_in_(13)) {
                for (int i = 0; i < A_MAX; i++) {
                    bool is_first_dec_stat = TRUE;
                    while (is_first_dec_stat || one_in_(2)) {
                        (void)do_dec_stat(caster_ptr, i);
                    }
                }
            } else {
                (void)do_dec_stat(caster_ptr, randint0(6));
            }

            break;
        }
        }
    }
}

/*!
 * @brief クリムゾンを発射する / Fire Crimson, evoluting gun.
 @ @param shooter_ptr 射撃を行うクリーチャー参照
 * @return キャンセルした場合 false.
 * @details
 * Need to analyze size of the window.
 * Need more color coding.
 */
bool fire_crimson(player_type *shooter_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(shooter_ptr, &dir))
        return FALSE;

    POSITION tx = shooter_ptr->x + 99 * ddx[dir];
    POSITION ty = shooter_ptr->y + 99 * ddy[dir];
    if ((dir == 5) && target_okay(shooter_ptr)) {
        tx = target_col;
        ty = target_row;
    }

    int num = 1;
    if (shooter_ptr->pclass == CLASS_ARCHER) {
        if (shooter_ptr->lev >= 10)
            num++;
        if (shooter_ptr->lev >= 30)
            num++;
        if (shooter_ptr->lev >= 45)
            num++;
    }

    BIT_FLAGS flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
    for (int i = 0; i < num; i++)
        project(shooter_ptr, 0, shooter_ptr->lev / 20 + 1, ty, tx, shooter_ptr->lev * shooter_ptr->lev * 6 / 50, GF_ROCKET, flg, -1);

    return TRUE;
}

/*!
 * @brief 町間のテレポートを行うメインルーチン
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return テレポート処理を決定したか否か
 */
bool tele_town(player_type *caster_ptr)
{
    if (caster_ptr->current_floor_ptr->dun_level) {
        msg_print(_("この魔法は地上でしか使えない！", "This spell can only be used on the surface!"));
        return FALSE;
    }

    if (caster_ptr->current_floor_ptr->inside_arena || caster_ptr->phase_out) {
        msg_print(_("この魔法は外でしか使えない！", "This spell can only be used outside!"));
        return FALSE;
    }

    screen_save();
    clear_bldg(4, 10);

    int i;
    int num = 0;
    for (i = 1; i < max_towns; i++) {
        char buf[80];

        if ((i == NO_TOWN) || (i == SECRET_TOWN) || (i == caster_ptr->town_num) || !(caster_ptr->visit & (1L << (i - 1))))
            continue;

        sprintf(buf, "%c) %-20s", I2A(i - 1), town_info[i].name);
        prt(buf, 5 + i, 5);
        num++;
    }

    if (num == 0) {
        msg_print(_("まだ行けるところがない。", "You have not yet visited any town."));
        msg_print(NULL);
        screen_load();
        return FALSE;
    }

    prt(_("どこに行きますか:", "Where do you want to go: "), 0, 0);
    while (TRUE) {
        i = inkey();

        if (i == ESCAPE) {
            screen_load();
            return FALSE;
        }

        else if ((i < 'a') || (i > ('a' + max_towns - 2)))
            continue;
        else if (((i - 'a' + 1) == caster_ptr->town_num) || ((i - 'a' + 1) == NO_TOWN) || ((i - 'a' + 1) == SECRET_TOWN)
            || !(caster_ptr->visit & (1L << (i - 'a'))))
            continue;
        break;
    }

    for (POSITION y = 0; y < current_world_ptr->max_wild_y; y++) {
        for (POSITION x = 0; x < current_world_ptr->max_wild_x; x++) {
            if (wilderness[y][x].town == (i - 'a' + 1)) {
                caster_ptr->wilderness_y = y;
                caster_ptr->wilderness_x = x;
            }
        }
    }

    caster_ptr->leaving = TRUE;
    caster_ptr->leave_bldg = TRUE;
    caster_ptr->teleport_town = TRUE;
    screen_load();
    return TRUE;
}
