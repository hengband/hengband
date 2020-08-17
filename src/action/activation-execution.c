#include "action/activation-execution.h"
#include "action/action-limited.h"
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
#include "object-enchant/object-ego.h"
#include "object-hook/hook-enchant.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "racial/racial-android.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-hex.h"
#include "spell/spell-types.h"
#include "sv-definition/sv-lite-types.h"
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

// Activation Execution.
typedef struct ae_type {
    DIRECTION dir;
    bool success;
    object_type *o_ptr;
    DEPTH lev;
    int chance;
    int fail;
} ae_type;

static ae_type *initialize_ae_type(player_type *user_ptr, ae_type *ae_ptr, const INVENTORY_IDX item)
{
    ae_ptr->o_ptr = ref_item(user_ptr, item);
    ae_ptr->lev = k_info[ae_ptr->o_ptr->k_idx].level;
    return ae_ptr;
}

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

static void inscribe_nickname(ae_type *ae_ptr)
{
    if (!cap_nickname)
        return;

    concptr t;
    char *s;
    char buf[80] = "";
    if (ae_ptr->o_ptr->inscription)
        strcpy(buf, quark_str(ae_ptr->o_ptr->inscription));

    s = buf;
    for (s = buf; *s && (*s != '#'); s++) {
#ifdef JP
        if (iskanji(*s))
            s++;
#endif
    }

    *s = '#';
    s++;
#ifdef JP
#else
    *s++ = '\'';
#endif
    t = quark_str(cap_nickname);
    while (*t) {
        *s = *t;
        s++;
        t++;
    }
#ifdef JP
#else
    *s++ = '\'';
#endif
    *s = '\0';
    ae_ptr->o_ptr->inscription = quark_add(buf);
}

static bool set_activation_target(player_type *user_ptr, ae_type *ae_ptr)
{
    bool old_target_pet = target_pet;
    target_pet = TRUE;
    if (!get_aim_dir(user_ptr, &ae_ptr->dir)) {
        target_pet = old_target_pet;
        return FALSE;
    }

    target_pet = old_target_pet;
    if (!fire_ball(user_ptr, GF_CAPTURE, ae_ptr->dir, 0, 0))
        return TRUE;

    ae_ptr->o_ptr->pval = (PARAMETER_VALUE)cap_mon;
    ae_ptr->o_ptr->xtra3 = (XTRA8)cap_mspeed;
    ae_ptr->o_ptr->xtra4 = (XTRA16)cap_hp;
    ae_ptr->o_ptr->xtra5 = (XTRA16)cap_maxhp;
    inscribe_nickname(ae_ptr);
    return TRUE;
}

static void add_quark_to_inscription(player_type *user_ptr, ae_type *ae_ptr, concptr t, char *buf)
{
    if (!*t)
        return;

    char *s = buf;
    t++;
#ifdef JP
#else
    bool quote = FALSE;
    if (*t == '\'') {
        t++;
        quote = TRUE;
    }
#endif

    while (*t) {
        *s = *t;
        t++;
        s++;
    }

#ifdef JP
#else
    if (quote && *(s - 1) == '\'')
        s--;
#endif

    *s = '\0';
    user_ptr->current_floor_ptr->m_list[hack_m_idx_ii].nickname = quark_add(buf);
    t = quark_str(ae_ptr->o_ptr->inscription);
    s = buf;
    while (*t && (*t != '#')) {
        *s = *t;
        t++;
        s++;
    }

    *s = '\0';
    ae_ptr->o_ptr->inscription = quark_add(buf);
}

static void check_inscription_value(player_type *user_ptr, ae_type *ae_ptr)
{
    if (ae_ptr->o_ptr->inscription == 0)
        return;

    char buf[80];
    concptr t = quark_str(ae_ptr->o_ptr->inscription);
    for (t = quark_str(ae_ptr->o_ptr->inscription); *t && (*t != '#'); t++) {
#ifdef JP
        if (iskanji(*t))
            t++;
#endif
    }

    add_quark_to_inscription(user_ptr, ae_ptr, t, buf);
}

static void check_monster_ball_use(player_type *user_ptr, ae_type *ae_ptr)
{
    if (!monster_can_enter(user_ptr, user_ptr->y + ddy[ae_ptr->dir], user_ptr->x + ddx[ae_ptr->dir], &r_info[ae_ptr->o_ptr->pval], 0))
        return;

    if (!place_monster_aux(user_ptr, 0, user_ptr->y + ddy[ae_ptr->dir], user_ptr->x + ddx[ae_ptr->dir], ae_ptr->o_ptr->pval, PM_FORCE_PET | PM_NO_KAGE))
        return;

    floor_type *floor_ptr = user_ptr->current_floor_ptr;
    if (ae_ptr->o_ptr->xtra3)
        floor_ptr->m_list[hack_m_idx_ii].mspeed = ae_ptr->o_ptr->xtra3;

    if (ae_ptr->o_ptr->xtra5)
        floor_ptr->m_list[hack_m_idx_ii].max_maxhp = ae_ptr->o_ptr->xtra5;

    if (ae_ptr->o_ptr->xtra4)
        floor_ptr->m_list[hack_m_idx_ii].hp = ae_ptr->o_ptr->xtra4;

    floor_ptr->m_list[hack_m_idx_ii].maxhp = floor_ptr->m_list[hack_m_idx_ii].max_maxhp;
    check_inscription_value(user_ptr, ae_ptr);
    ae_ptr->o_ptr->pval = 0;
    ae_ptr->o_ptr->xtra3 = 0;
    ae_ptr->o_ptr->xtra4 = 0;
    ae_ptr->o_ptr->xtra5 = 0;
    ae_ptr->success = TRUE;
}

static bool exe_monster_capture(player_type *user_ptr, ae_type *ae_ptr)
{
    if (ae_ptr->o_ptr->tval != TV_CAPTURE)
        return FALSE;

    if (ae_ptr->o_ptr->pval == 0) {
        if (!set_activation_target(user_ptr, ae_ptr))
            return TRUE;

        calc_android_exp(user_ptr);
        return TRUE;
    }

    ae_ptr->success = FALSE;
    if (!get_direction(user_ptr, &ae_ptr->dir, FALSE, FALSE))
        return TRUE;

    check_monster_ball_use(user_ptr, ae_ptr);
    if (!ae_ptr->success)
        msg_print(_("おっと、解放に失敗した。", "Oops.  You failed to release your pet."));

    calc_android_exp(user_ptr);
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
        user_ptr->window |= PW_INVEN | PW_EQUIP;
        return;
    }

    if (activate_whistle(user_ptr, ae_ptr))
        return;

    if (exe_monster_capture(user_ptr, ae_ptr))
        return;

    msg_print(_("おっと、このアイテムは始動できない。", "Oops.  That object cannot be activated."));
}
