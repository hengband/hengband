﻿#include "action/activation-execution.h"
#include "action/action-limited.h"
#include "artifact/random-art-effects.h"
#include "artifact/artifact-info.h"
#include "core/window-redrawer.h"
#include "effect/spells-effect-util.h"
#include "floor/geometry.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster/monster-info.h"
#include "monster/monster-util.h"
#include "object-activation/activation-switcher.h"
#include "object-activation/activation-util.h"
#include "object-enchant/object-ego.h"
#include "object-hook/hook-enchant.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "racial/racial-android.h"
#include "specific-object/monster-ball.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-hex.h"
#include "spell/spell-types.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-ring-types.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "util/quarks.h"
#include "util/sort.h"
#include "view/display-messages.h"
#include "world/world.h"

static void decide_activation_level(player_type *user_ptr, ae_type *ae_ptr)
{
    if (object_is_fixed_artifact(ae_ptr->o_ptr)) {
        ae_ptr->lev = a_info[ae_ptr->o_ptr->name1].level;
        return;
    }

    if (object_is_random_artifact(ae_ptr->o_ptr)) {
        const activation_type *const act_ptr = find_activation_info(user_ptr, ae_ptr->o_ptr);
        if (act_ptr != NULL)
            ae_ptr->lev = act_ptr->level;

        return;
    }

    if (((ae_ptr->o_ptr->tval == TV_RING) || (ae_ptr->o_ptr->tval == TV_AMULET)) && ae_ptr->o_ptr->name2)
        ae_ptr->lev = e_info[ae_ptr->o_ptr->name2].level;
}

static void decide_chance_fail(player_type *user_ptr, ae_type *ae_ptr)
{
    ae_ptr->chance = user_ptr->skill_dev;
    if (user_ptr->confused)
        ae_ptr->chance = ae_ptr->chance / 2;

    ae_ptr->fail = ae_ptr->lev + 5;
    if (ae_ptr->chance > ae_ptr->fail)
        ae_ptr->fail -= (ae_ptr->chance - ae_ptr->fail) * 2;
    else
        ae_ptr->chance -= (ae_ptr->fail - ae_ptr->chance) * 2;

    if (ae_ptr->fail < USE_DEVICE)
        ae_ptr->fail = USE_DEVICE;

    if (ae_ptr->chance < USE_DEVICE)
        ae_ptr->chance = USE_DEVICE;
}

static void decide_activation_success(player_type *user_ptr, ae_type *ae_ptr)
{
    if (user_ptr->pclass == CLASS_BERSERKER) {
        ae_ptr->success = FALSE;
        return;
    }

    if (ae_ptr->chance > ae_ptr->fail) {
        ae_ptr->success = randint0(ae_ptr->chance * 2) >= ae_ptr->fail;
        return;
    }

    ae_ptr->success = randint0(ae_ptr->fail * 2) < ae_ptr->chance;
}

static bool check_activation_success(ae_type *ae_ptr)
{
    if (ae_ptr->success)
        return TRUE;

    if (flush_failure)
        flush();

    msg_print(_("うまく始動させることができなかった。", "You failed to activate it properly."));
    sound(SOUND_FAIL);
    return FALSE;
}

static bool check_activation_conditions(player_type *user_ptr, ae_type *ae_ptr)
{
    if (!check_activation_success(ae_ptr))
        return FALSE;

    if (ae_ptr->o_ptr->timeout) {
        msg_print(_("それは微かに音を立て、輝き、消えた...", "It whines, glows and fades..."));
        return FALSE;
    }

    if (!ae_ptr->o_ptr->xtra4 && (ae_ptr->o_ptr->tval == TV_FLASK) && ((ae_ptr->o_ptr->sval == SV_LITE_TORCH) || (ae_ptr->o_ptr->sval == SV_LITE_LANTERN))) {
        msg_print(_("燃料がない。", "It has no fuel."));
        free_turn(user_ptr);
        return FALSE;
    }

    return TRUE;
}

/*!
 * @brief アイテムの発動効果を処理する。
 * @param user_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 発動実行の是非を返す。
 */
static bool activate_artifact(player_type *user_ptr, object_type *o_ptr)
{
    concptr name = k_name + k_info[o_ptr->k_idx].name;
    const activation_type *const act_ptr = find_activation_info(user_ptr, o_ptr);
    if (!act_ptr) {
        msg_print("Activation information is not found.");
        return FALSE;
    }

    if (!switch_activation(user_ptr, o_ptr, act_ptr, name))
        return FALSE;

    if (act_ptr->timeout.constant >= 0) {
        o_ptr->timeout = (s16b)act_ptr->timeout.constant;
        if (act_ptr->timeout.dice > 0)
            o_ptr->timeout += randint1(act_ptr->timeout.dice);

        return TRUE;
    }

    switch (act_ptr->index) {
    case ACT_BR_FIRE:
        o_ptr->timeout = ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_FLAMES)) ? 200 : 250;
        return TRUE;
    case ACT_BR_COLD:
        o_ptr->timeout = ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ICE)) ? 200 : 250;
        return TRUE;
    case ACT_TERROR:
        o_ptr->timeout = 3 * (user_ptr->lev + 10);
        return TRUE;
    case ACT_MURAMASA:
        return TRUE;
    default:
        msg_format("Special timeout is not implemented: %d.", act_ptr->index);
        return FALSE;
    }
}

static bool activate_whistle(player_type *user_ptr, ae_type *ae_ptr)
{
    if (ae_ptr->o_ptr->tval != TV_WHISTLE)
        return FALSE;

    if (music_singing_any(user_ptr))
        stop_singing(user_ptr);

    if (hex_spelling_any(user_ptr))
        stop_hex_spell_all(user_ptr);

    MONSTER_IDX pet_ctr;
    MONSTER_IDX *who;
    int max_pet = 0;
    C_MAKE(who, current_world_ptr->max_m_idx, MONSTER_IDX);
    for (pet_ctr = user_ptr->current_floor_ptr->m_max - 1; pet_ctr >= 1; pet_ctr--)
        if (is_pet(&user_ptr->current_floor_ptr->m_list[pet_ctr]) && (user_ptr->riding != pet_ctr))
            who[max_pet++] = pet_ctr;

    u16b dummy_why;
    ang_sort(user_ptr, who, &dummy_why, max_pet, ang_sort_comp_pet, ang_sort_swap_hook);
    for (MONSTER_IDX i = 0; i < max_pet; i++) {
        pet_ctr = who[i];
        teleport_monster_to(user_ptr, pet_ctr, user_ptr->y, user_ptr->x, 100, TELEPORT_PASSIVE);
    }

    C_KILL(who, current_world_ptr->max_m_idx, MONSTER_IDX);
    ae_ptr->o_ptr->timeout = 100 + randint1(100);
    return TRUE;
}

/*!
 * @brief 装備を発動するコマンドのサブルーチン /
 * Activate a wielded object.  Wielded objects never stack.
 * And even if they did, activatable objects never stack.
 * @param item 発動するオブジェクトの所持品ID
 * @return なし
 * @details
 * <pre>
 * Currently, only (some) artifacts, and Dragon Scale Mail, can be activated.
 * But one could, for example, easily make an activatable "Ring of Plasma".
 * Note that it always takes a turn to activate an artifact, even if
 * the user hits "escape" at the "direction" prompt.
 * </pre>
 */
void exe_activate(player_type *user_ptr, INVENTORY_IDX item)
{
    take_turn(user_ptr, 100);
    ae_type tmp_ae;
    ae_type *ae_ptr = initialize_ae_type(user_ptr, &tmp_ae, item);
    decide_activation_level(user_ptr, ae_ptr);
    decide_chance_fail(user_ptr, ae_ptr);
    if (cmd_limit_time_walk(user_ptr))
        return;

    decide_activation_success(user_ptr, ae_ptr);
    if (!check_activation_conditions(user_ptr, ae_ptr))
        return;

    msg_print(_("始動させた...", "You activate it..."));
    sound(SOUND_ZAP);
    if (activation_index(user_ptr, ae_ptr->o_ptr)) {
        (void)activate_artifact(user_ptr, ae_ptr->o_ptr);
        user_ptr->window_flags |= PW_INVEN | PW_EQUIP;
        return;
    }

    if (activate_whistle(user_ptr, ae_ptr))
        return;

    if (exe_monster_capture(user_ptr, ae_ptr))
        return;

    msg_print(_("おっと、このアイテムは始動できない。", "Oops.  That object cannot be activated."));
}
