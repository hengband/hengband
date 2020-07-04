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
