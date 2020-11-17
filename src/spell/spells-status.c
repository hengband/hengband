/*!
 * @brief スピード等のステータスに影響のある魔法の処理
 * @date 2019/01/22
 * @author deskull
 */

#include "spell/spells-status.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-item/cmd-magiceat.h"
#include "core/hp-mp-processor.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "effect/effect-characteristics.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "grid/feature-flag-types.h"
#include "grid/grid.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "mind/mind-force-trainer.h"
#include "monster/monster-describer.h"
#include "object/object-generator.h"
#include "object/object-kind-hook.h"
#include "object/object-kind.h"
#include "player/attack-defense-types.h"
#include "player-info/avatar.h"
#include "player/player-class.h"
#include "player/player-status.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell/spell-types.h"
#include "status/action-setter.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/experience.h"
#include "status/shape-changer.h"
#include "status/sight-setter.h"
#include "system/floor-type-definition.h"
#include "target/target-getter.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief モンスター回復処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool heal_monster(player_type *caster_ptr, DIRECTION dir, HIT_POINT dam)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
    return (project_hook(caster_ptr, GF_OLD_HEAL, dir, dam, flg));
}

/*!
 * @brief モンスター加速処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param power 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool speed_monster(player_type *caster_ptr, DIRECTION dir, int power)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
    return (project_hook(caster_ptr, GF_OLD_SPEED, dir, power, flg));
}

/*!
 * @brief モンスター減速処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param power 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool slow_monster(player_type *caster_ptr, DIRECTION dir, int power)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
    return (project_hook(caster_ptr, GF_OLD_SLOW, dir, power, flg));
}

/*!
 * @brief モンスター催眠処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param power 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool sleep_monster(player_type *caster_ptr, DIRECTION dir, int power)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
    return (project_hook(caster_ptr, GF_OLD_SLEEP, dir, power, flg));
}

/*!
 * @brief モンスター拘束(STASIS)処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 * @details 威力はプレイヤーレベル*2に固定
 */
bool stasis_monster(player_type *caster_ptr, DIRECTION dir) { return (fire_ball_hide(caster_ptr, GF_STASIS, dir, caster_ptr->lev * 2, 0)); }

/*!
 * @brief 邪悪なモンスター拘束(STASIS)処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 * @details 威力はプレイヤーレベル*2に固定
 */
bool stasis_evil(player_type *caster_ptr, DIRECTION dir) { return (fire_ball_hide(caster_ptr, GF_STASIS_EVIL, dir, caster_ptr->lev * 2, 0)); }

/*!
 * @brief モンスター混乱処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev プレイヤーレベル(=効力)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool confuse_monster(player_type *caster_ptr, DIRECTION dir, PLAYER_LEVEL plev)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
    return (project_hook(caster_ptr, GF_OLD_CONF, dir, plev, flg));
}

/*!
 * @brief モンスター朦朧処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev プレイヤーレベル(=効力)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool stun_monster(player_type *caster_ptr, DIRECTION dir, PLAYER_LEVEL plev)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
    return (project_hook(caster_ptr, GF_STUN, dir, plev, flg));
}

/*!
 * @brief チェンジモンスター処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param power 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool poly_monster(player_type *caster_ptr, DIRECTION dir, int power)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
    bool tester = (project_hook(caster_ptr, GF_OLD_POLY, dir, power, flg));
    if (tester)
        chg_virtue(caster_ptr, V_CHANCE, 1);
    return (tester);
}

/*!
 * @brief クローンモンスター処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool clone_monster(player_type *caster_ptr, DIRECTION dir)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
    return (project_hook(caster_ptr, GF_OLD_CLONE, dir, 0, flg));
}

/*!
 * @brief モンスター恐慌処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev プレイヤーレベル(=効力)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool fear_monster(player_type *caster_ptr, DIRECTION dir, PLAYER_LEVEL plev)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
    return (project_hook(caster_ptr, GF_TURN_ALL, dir, plev, flg));
}

bool time_walk(player_type *creature_ptr)
{
    if (creature_ptr->timewalk) {
        msg_print(_("既に時は止まっている。", "Time is already stopped."));
        return FALSE;
    }

    creature_ptr->timewalk = TRUE;
    msg_print(_("「時よ！」", "You yell 'Time!'"));
    //	msg_print(_("「『ザ・ワールド』！時は止まった！」", "You yell 'The World! Time has stopped!'"));
    msg_print(NULL);

    creature_ptr->energy_need -= 1000 + (100 + creature_ptr->csp - 50) * TURNS_PER_TICK / 10;
    creature_ptr->redraw |= (PR_MAP);
    creature_ptr->update |= (PU_MONSTERS);
    creature_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
    handle_stuff(creature_ptr);
    return TRUE;
}

/*!
 * @brief プレイヤーのヒットダイスを振る / Role Hitpoints
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param options スペル共通オプション
 * @return なし
 */
void roll_hitdice(player_type *creature_ptr, spell_operation options)
{
    HIT_POINT min_value = creature_ptr->hitdie + ((PY_MAX_LEVEL + 2) * (creature_ptr->hitdie + 1)) * 3 / 8;
    HIT_POINT max_value = creature_ptr->hitdie + ((PY_MAX_LEVEL + 2) * (creature_ptr->hitdie + 1)) * 5 / 8;

    /* Rerate */
    while (TRUE) {
        /* Pre-calculate level 1 hitdice */
        creature_ptr->player_hp[0] = (HIT_POINT)creature_ptr->hitdie;

        for (int i = 1; i < 4; i++) {
            creature_ptr->player_hp[0] += randint1(creature_ptr->hitdie);
        }

        /* Roll the hitpoint values */
        for (int i = 1; i < PY_MAX_LEVEL; i++) {
            creature_ptr->player_hp[i] = creature_ptr->player_hp[i - 1] + randint1(creature_ptr->hitdie);
        }

        /* Require "valid" hitpoints at highest level */
        if ((creature_ptr->player_hp[PY_MAX_LEVEL - 1] >= min_value) && (creature_ptr->player_hp[PY_MAX_LEVEL - 1] <= max_value))
            break;
    }

    PERCENTAGE percent
        = (int)(((long)creature_ptr->player_hp[PY_MAX_LEVEL - 1] * 200L) / (2 * creature_ptr->hitdie + ((PY_MAX_LEVEL - 1 + 3) * (creature_ptr->hitdie + 1))));

    /* Update and redraw hitpoints */
    creature_ptr->update |= (PU_HP);
    creature_ptr->redraw |= (PR_HP);
    creature_ptr->window |= (PW_PLAYER);

    if (!(options & SPOP_NO_UPDATE))
        handle_stuff(creature_ptr);

    if (!(options & SPOP_DISPLAY_MES))
        return;

    if (options & SPOP_DEBUG) {
        msg_format(_("現在の体力ランクは %d/100 です。", "Your life rate is %d/100 now."), percent);
        creature_ptr->knowledge |= KNOW_HPRATE;
        return;
    }

    msg_print(_("体力ランクが変わった。", "Life rate is changed."));
    creature_ptr->knowledge &= ~(KNOW_HPRATE);
}

bool life_stream(player_type *creature_ptr, bool message, bool virtue_change)
{
    if (virtue_change) {
        chg_virtue(creature_ptr, V_VITALITY, 1);
        chg_virtue(creature_ptr, V_UNLIFE, -5);
    }

    if (message) {
        msg_print(_("体中に生命力が満ちあふれてきた！", "You feel life flow through your body!"));
    }

    restore_level(creature_ptr);
    (void)set_poisoned(creature_ptr, 0);
    (void)set_blind(creature_ptr, 0);
    (void)set_confused(creature_ptr, 0);
    (void)set_image(creature_ptr, 0);
    (void)set_stun(creature_ptr, 0);
    (void)set_cut(creature_ptr, 0);
    (void)restore_all_status(creature_ptr);
    (void)set_shero(creature_ptr, 0, TRUE);
    handle_stuff(creature_ptr);
    hp_player(creature_ptr, 5000);

    return TRUE;
}

bool heroism(player_type *creature_ptr, int base)
{
    bool ident = FALSE;
    if (set_afraid(creature_ptr, 0))
        ident = TRUE;
    if (set_hero(creature_ptr, creature_ptr->hero + randint1(base) + base, FALSE))
        ident = TRUE;
    if (hp_player(creature_ptr, 10))
        ident = TRUE;
    return ident;
}

bool berserk(player_type *creature_ptr, int base)
{
    bool ident = FALSE;
    if (set_afraid(creature_ptr, 0))
        ident = TRUE;
    if (set_shero(creature_ptr, creature_ptr->shero + randint1(base) + base, FALSE))
        ident = TRUE;
    if (hp_player(creature_ptr, 30))
        ident = TRUE;
    return ident;
}

bool cure_light_wounds(player_type *creature_ptr, DICE_NUMBER dice, DICE_SID sides)
{
    bool ident = FALSE;
    if (hp_player(creature_ptr, damroll(dice, sides)))
        ident = TRUE;
    if (set_blind(creature_ptr, 0))
        ident = TRUE;
    if (set_cut(creature_ptr, creature_ptr->cut - 10))
        ident = TRUE;
    if (set_shero(creature_ptr, 0, TRUE))
        ident = TRUE;
    return ident;
}

bool cure_serious_wounds(player_type *creature_ptr, DICE_NUMBER dice, DICE_SID sides)
{
    bool ident = FALSE;
    if (hp_player(creature_ptr, damroll(dice, sides)))
        ident = TRUE;
    if (set_blind(creature_ptr, 0))
        ident = TRUE;
    if (set_confused(creature_ptr, 0))
        ident = TRUE;
    if (set_cut(creature_ptr, (creature_ptr->cut / 2) - 50))
        ident = TRUE;
    if (set_shero(creature_ptr, 0, TRUE))
        ident = TRUE;
    return ident;
}

bool cure_critical_wounds(player_type *creature_ptr, HIT_POINT pow)
{
    bool ident = FALSE;
    if (hp_player(creature_ptr, pow))
        ident = TRUE;
    if (set_blind(creature_ptr, 0))
        ident = TRUE;
    if (set_confused(creature_ptr, 0))
        ident = TRUE;
    if (set_poisoned(creature_ptr, 0))
        ident = TRUE;
    if (set_stun(creature_ptr, 0))
        ident = TRUE;
    if (set_cut(creature_ptr, 0))
        ident = TRUE;
    if (set_shero(creature_ptr, 0, TRUE))
        ident = TRUE;
    return ident;
}

bool true_healing(player_type *creature_ptr, HIT_POINT pow)
{
    bool ident = FALSE;
    if (hp_player(creature_ptr, pow))
        ident = TRUE;
    if (set_blind(creature_ptr, 0))
        ident = TRUE;
    if (set_confused(creature_ptr, 0))
        ident = TRUE;
    if (set_poisoned(creature_ptr, 0))
        ident = TRUE;
    if (set_stun(creature_ptr, 0))
        ident = TRUE;
    if (set_cut(creature_ptr, 0))
        ident = TRUE;
    if (set_image(creature_ptr, 0))
        ident = TRUE;
    return ident;
}

bool restore_mana(player_type *creature_ptr, bool magic_eater)
{
    if (creature_ptr->pclass == CLASS_MAGIC_EATER && magic_eater) {
        int i;
        for (i = 0; i < EATER_EXT * 2; i++) {
            creature_ptr->magic_num1[i] += (creature_ptr->magic_num2[i] < 10) ? EATER_CHARGE * 3 : creature_ptr->magic_num2[i] * EATER_CHARGE / 3;
            if (creature_ptr->magic_num1[i] > creature_ptr->magic_num2[i] * EATER_CHARGE)
                creature_ptr->magic_num1[i] = creature_ptr->magic_num2[i] * EATER_CHARGE;
        }

        for (; i < EATER_EXT * 3; i++) {
            KIND_OBJECT_IDX k_idx = lookup_kind(TV_ROD, i - EATER_EXT * 2);
            creature_ptr->magic_num1[i]
                -= ((creature_ptr->magic_num2[i] < 10) ? EATER_ROD_CHARGE * 3 : creature_ptr->magic_num2[i] * EATER_ROD_CHARGE / 3) * k_info[k_idx].pval;
            if (creature_ptr->magic_num1[i] < 0)
                creature_ptr->magic_num1[i] = 0;
        }

        msg_print(_("頭がハッキリとした。", "You feel your head clear."));
        creature_ptr->window |= (PW_PLAYER);
        return TRUE;
    }

    if (creature_ptr->csp >= creature_ptr->msp)
        return FALSE;

    creature_ptr->csp = creature_ptr->msp;
    creature_ptr->csp_frac = 0;
    msg_print(_("頭がハッキリとした。", "You feel your head clear."));
    creature_ptr->redraw |= (PR_MANA);
    creature_ptr->window |= (PW_PLAYER);
    creature_ptr->window |= (PW_SPELL);
    return TRUE;
}

bool restore_all_status(player_type *creature_ptr)
{
    bool ident = FALSE;
    if (do_res_stat(creature_ptr, A_STR))
        ident = TRUE;
    if (do_res_stat(creature_ptr, A_INT))
        ident = TRUE;
    if (do_res_stat(creature_ptr, A_WIS))
        ident = TRUE;
    if (do_res_stat(creature_ptr, A_DEX))
        ident = TRUE;
    if (do_res_stat(creature_ptr, A_CON))
        ident = TRUE;
    if (do_res_stat(creature_ptr, A_CHR))
        ident = TRUE;
    return ident;
}

bool fishing(player_type *creature_ptr)
{
    DIRECTION dir;
    if (!get_direction(creature_ptr, &dir, FALSE, FALSE))
        return FALSE;
    POSITION y = creature_ptr->y + ddy[dir];
    POSITION x = creature_ptr->x + ddx[dir];
    creature_ptr->fishing_dir = dir;
    if (!cave_has_flag_bold(creature_ptr->current_floor_ptr, y, x, FF_WATER)) {
        msg_print(_("そこは水辺ではない。", "There is no fishing place."));
        return FALSE;
    }

    if (creature_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
        GAME_TEXT m_name[MAX_NLEN];
        monster_desc(creature_ptr, m_name, &creature_ptr->current_floor_ptr->m_list[creature_ptr->current_floor_ptr->grid_array[y][x].m_idx], 0);
        msg_format(_("%sが邪魔だ！", "%^s is standing in your way."), m_name);
        free_turn(creature_ptr);
        return FALSE;
    }

    set_action(creature_ptr, ACTION_FISH);
    creature_ptr->redraw |= (PR_STATE);
    return TRUE;
}

bool cosmic_cast_off(player_type *creature_ptr, object_type *o_ptr)
{
    /* Cast off activated item */
    INVENTORY_IDX inv;
    for (inv = INVEN_RARM; inv <= INVEN_FEET; inv++) {
        if (o_ptr == &creature_ptr->inventory_list[inv])
            break;
    }

    if (inv > INVEN_FEET)
        return FALSE;

    object_type forge;
    object_copy(&forge, o_ptr);
    inven_item_increase(creature_ptr, inv, (0 - o_ptr->number));
    inven_item_optimize(creature_ptr, inv);
    OBJECT_IDX o_idx = drop_near(creature_ptr, &forge, 0, creature_ptr->y, creature_ptr->x);
    o_ptr = &creature_ptr->current_floor_ptr->o_list[o_idx];

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(creature_ptr, o_name, o_ptr, OD_NAME_ONLY);
    msg_format(_("%sを脱ぎ捨てた。", "You cast off %s."), o_name);

    /* Get effects */
    msg_print(_("「燃え上がれ俺の小宇宙！」", "You say, 'Burn up my cosmo!"));
    int t = 20 + randint1(20);
    (void)set_blind(creature_ptr, creature_ptr->blind + t);
    (void)set_afraid(creature_ptr, 0);
    (void)set_tim_esp(creature_ptr, creature_ptr->tim_esp + t, FALSE);
    (void)set_tim_regen(creature_ptr, creature_ptr->tim_regen + t, FALSE);
    (void)set_hero(creature_ptr, creature_ptr->hero + t, FALSE);
    (void)set_blessed(creature_ptr, creature_ptr->blessed + t, FALSE);
    (void)set_fast(creature_ptr, creature_ptr->fast + t, FALSE);
    (void)set_shero(creature_ptr, creature_ptr->shero + t, FALSE);
    if (creature_ptr->pclass == CLASS_FORCETRAINER) {
        set_current_ki(creature_ptr, TRUE, creature_ptr->lev * 5 + 190);
        msg_print(_("気が爆発寸前になった。", "Your force absorbs the explosion."));
    }

    return TRUE;
}

/*!
 * @brief プレイヤーの因果混乱処理 / Apply Nexus
 * @param m_ptr 因果混乱をプレイヤーに与えたモンスターの情報参照ポインタ
 * @return なし
 */
void apply_nexus(monster_type *m_ptr, player_type *target_ptr)
{
    switch (randint1(7)) {
    case 1:
    case 2:
    case 3: {
        teleport_player(target_ptr, 200, TELEPORT_PASSIVE);
        break;
    }

    case 4:
    case 5: {
        teleport_player_to(target_ptr, m_ptr->fy, m_ptr->fx, TELEPORT_PASSIVE);
        break;
    }

    case 6: {
        if (randint0(100) < target_ptr->skill_sav) {
            msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
            break;
        }

        teleport_level(target_ptr, 0);
        break;
    }

    case 7: {
        if (randint0(100) < target_ptr->skill_sav) {
            msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
            break;
        }

        msg_print(_("体がねじれ始めた...", "Your body starts to scramble..."));
        status_shuffle(target_ptr);
        break;
    }
    }
}

/*!
 * @brief プレイヤーのステータスシャッフル処理
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void status_shuffle(player_type *creature_ptr)
{
    /* Pick a pair of stats */
    int i = randint0(A_MAX);
    int j;

    // todo ここのループは一体何をしている？
    for (j = i; j == i; j = randint0(A_MAX)) /* loop */
        ;

    BASE_STATUS max1 = creature_ptr->stat_max[i];
    BASE_STATUS cur1 = creature_ptr->stat_cur[i];
    BASE_STATUS max2 = creature_ptr->stat_max[j];
    BASE_STATUS cur2 = creature_ptr->stat_cur[j];

    creature_ptr->stat_max[i] = max2;
    creature_ptr->stat_cur[i] = cur2;
    creature_ptr->stat_max[j] = max1;
    creature_ptr->stat_cur[j] = cur1;

    for (int k = 0; k < A_MAX; k++) {
        if (creature_ptr->stat_max[k] > creature_ptr->stat_max_max[k])
            creature_ptr->stat_max[k] = creature_ptr->stat_max_max[k];
        if (creature_ptr->stat_cur[k] > creature_ptr->stat_max_max[k])
            creature_ptr->stat_cur[k] = creature_ptr->stat_max_max[k];
    }

    creature_ptr->update |= PU_BONUS;
}
