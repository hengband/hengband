/*!
 * @brief スピード等のステータスに影響のある魔法の処理
 * @date 2019/01/22
 * @author deskull
 */

#include "spell/spells-status.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-item/cmd-magiceat.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "hpmp/hp-mp-processor.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-force-trainer.h"
#include "monster/monster-describer.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/magic-eater-data-type.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "status/action-setter.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/experience.h"
#include "status/shape-changer.h"
#include "status/sight-setter.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-getter.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief モンスター回復処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool heal_monster(PlayerType *player_ptr, const Direction &dir, int dam)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
    return project_hook(player_ptr, AttributeType::OLD_HEAL, dir, dam, flg);
}

/*!
 * @brief モンスター加速処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param power 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool speed_monster(PlayerType *player_ptr, const Direction &dir, int power)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
    return project_hook(player_ptr, AttributeType::OLD_SPEED, dir, power, flg);
}

/*!
 * @brief モンスター減速処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param power 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool slow_monster(PlayerType *player_ptr, const Direction &dir, int power)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
    return project_hook(player_ptr, AttributeType::OLD_SLOW, dir, power, flg);
}

/*!
 * @brief モンスター催眠処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param power 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool sleep_monster(PlayerType *player_ptr, const Direction &dir, int power)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
    return project_hook(player_ptr, AttributeType::OLD_SLEEP, dir, power, flg);
}

/*!
 * @brief モンスター拘束(STASIS)処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 * @details 威力はプレイヤーレベル*2に固定
 */
bool stasis_monster(PlayerType *player_ptr, const Direction &dir)
{
    return fire_ball_hide(player_ptr, AttributeType::STASIS, dir, player_ptr->lev * 2, 0);
}

/*!
 * @brief 邪悪なモンスター拘束(STASIS)処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 * @details 威力はプレイヤーレベル*2に固定
 */
bool stasis_evil(PlayerType *player_ptr, const Direction &dir)
{
    return fire_ball_hide(player_ptr, AttributeType::STASIS_EVIL, dir, player_ptr->lev * 2, 0);
}

/*!
 * @brief モンスター混乱処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev プレイヤーレベル(=効力)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool confuse_monster(PlayerType *player_ptr, const Direction &dir, PLAYER_LEVEL plev)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
    return project_hook(player_ptr, AttributeType::OLD_CONF, dir, plev, flg);
}

/*!
 * @brief モンスター朦朧処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev プレイヤーレベル(=効力)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool stun_monster(PlayerType *player_ptr, const Direction &dir, PLAYER_LEVEL plev)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
    return project_hook(player_ptr, AttributeType::STUN, dir, plev, flg);
}

/*!
 * @brief チェンジモンスター処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param power 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool poly_monster(PlayerType *player_ptr, const Direction &dir, int power)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
    bool tester = (project_hook(player_ptr, AttributeType::OLD_POLY, dir, power, flg));
    if (tester) {
        chg_virtue(player_ptr, Virtue::CHANCE, 1);
    }
    return tester;
}

/*!
 * @brief クローンモンスター処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool clone_monster(PlayerType *player_ptr, const Direction &dir)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
    return project_hook(player_ptr, AttributeType::OLD_CLONE, dir, 0, flg);
}

/*!
 * @brief モンスター恐慌処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev プレイヤーレベル(=効力)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool fear_monster(PlayerType *player_ptr, const Direction &dir, PLAYER_LEVEL plev)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
    return project_hook(player_ptr, AttributeType::TURN_ALL, dir, plev, flg);
}

bool time_walk(PlayerType *player_ptr)
{
    if (player_ptr->timewalk) {
        msg_print(_("既に時は止まっている。", "Time is already stopped."));
        return false;
    }

    player_ptr->timewalk = true;
    msg_print(_("「時よ！」", "You yell 'Time!'"));
    //	msg_print(_("「『ザ・ワールド』！時は止まった！」", "You yell 'The World! Time has stopped!'"));
    msg_erase();

    player_ptr->energy_need -= 1000 + (100 + player_ptr->csp - 50) * TURNS_PER_TICK / 10;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::MAP);
    rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
    static constexpr auto flags = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    rfu.set_flags(flags);
    handle_stuff(player_ptr);
    return true;
}

/*!
 * @brief プレイヤーのヒットダイスを振る / Role Hitpoints
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param options スペル共通オプション
 */
void roll_hitdice(PlayerType *player_ptr, spell_operation options)
{
    constexpr auto roll_num = 3 + PY_MAX_LEVEL - 1;
    const auto expected_hp = player_ptr->hit_dice.maxroll() + player_ptr->hit_dice.floored_expected_value_multiplied_by(roll_num);
    const auto min_value = expected_hp * 3 / 4;
    const auto max_value = expected_hp * 5 / 4;

    /* Rerate */
    while (true) {
        /* Pre-calculate level 1 hitdice */
        player_ptr->player_hp[0] = player_ptr->hit_dice.maxroll();

        for (int i = 1; i < 4; i++) {
            player_ptr->player_hp[0] += player_ptr->hit_dice.roll();
        }

        /* Roll the hitpoint values */
        for (int i = 1; i < PY_MAX_LEVEL; i++) {
            player_ptr->player_hp[i] = player_ptr->player_hp[i - 1] + player_ptr->hit_dice.roll();
        }

        /* Require "valid" hitpoints at highest level */
        if ((player_ptr->player_hp[PY_MAX_LEVEL - 1] >= min_value) && (player_ptr->player_hp[PY_MAX_LEVEL - 1] <= max_value)) {
            break;
        }
    }

    player_ptr->knowledge &= ~(KNOW_HPRATE);

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::HP);
    rfu.set_flag(MainWindowRedrawingFlag::HP);
    rfu.set_flag(SubWindowRedrawingFlag::PLAYER);
    if (!(options & SPOP_NO_UPDATE)) {
        handle_stuff(player_ptr);
    }

    if (!(options & SPOP_DISPLAY_MES)) {
        return;
    }

    if (options & SPOP_DEBUG) {
        msg_format(_("現在の体力ランクは %d/100 です。", "Your life rate is %d/100 now."), player_ptr->calc_life_rating());
        player_ptr->knowledge |= KNOW_HPRATE;
        return;
    }

    msg_print(_("体力ランクが変わった。", "Life rate has changed."));
}

bool life_stream(PlayerType *player_ptr, bool message, bool virtue_change)
{
    if (virtue_change) {
        chg_virtue(player_ptr, Virtue::VITALITY, 1);
        chg_virtue(player_ptr, Virtue::UNLIFE, -5);
    }

    if (message) {
        msg_print(_("体中に生命力が満ちあふれてきた！", "You feel life flow through your body!"));
    }

    restore_level(player_ptr);
    BadStatusSetter bss(player_ptr);
    (void)bss.set_poison(0);
    (void)bss.set_blindness(0);
    (void)bss.set_confusion(0);
    (void)bss.hallucination(0);
    (void)bss.set_stun(0);
    (void)bss.set_cut(0);
    (void)bss.set_paralysis(0);
    (void)restore_all_status(player_ptr);
    (void)set_shero(player_ptr, 0, true);
    handle_stuff(player_ptr);
    hp_player(player_ptr, 5000);

    return true;
}

bool heroism(PlayerType *player_ptr, int base)
{
    auto ident = false;
    if (BadStatusSetter(player_ptr).set_fear(0)) {
        ident = true;
    }

    if (set_hero(player_ptr, player_ptr->hero + randint1(base) + base, false)) {
        ident = true;
    }

    if (hp_player(player_ptr, 10)) {
        ident = true;
    }

    return ident;
}

bool berserk(PlayerType *player_ptr, int base)
{
    auto ident = false;
    if (BadStatusSetter(player_ptr).set_fear(0)) {
        ident = true;
    }

    if (set_shero(player_ptr, player_ptr->shero + randint1(base) + base, false)) {
        ident = true;
    }

    if (hp_player(player_ptr, 30)) {
        ident = true;
    }

    return ident;
}

bool cure_light_wounds(PlayerType *player_ptr, int pow)
{
    auto ident = false;
    if (hp_player(player_ptr, pow)) {
        ident = true;
    }

    BadStatusSetter bss(player_ptr);
    if (bss.set_blindness(0)) {
        ident = true;
    }

    if (bss.mod_cut(-10)) {
        ident = true;
    }

    if (set_shero(player_ptr, 0, true)) {
        ident = true;
    }

    return ident;
}

bool cure_serious_wounds(PlayerType *player_ptr, int pow)
{
    auto ident = false;
    if (hp_player(player_ptr, pow)) {
        ident = true;
    }

    BadStatusSetter bss(player_ptr);
    if (bss.set_blindness(0)) {
        ident = true;
    }

    if (bss.set_confusion(0)) {
        ident = true;
    }

    if (bss.set_cut((player_ptr->effects()->cut().current() / 2) - 50)) {
        ident = true;
    }

    if (set_shero(player_ptr, 0, true)) {
        ident = true;
    }

    return ident;
}

bool cure_critical_wounds(PlayerType *player_ptr, int pow)
{
    auto ident = false;
    if (hp_player(player_ptr, pow)) {
        ident = true;
    }

    BadStatusSetter bss(player_ptr);
    if (bss.set_blindness(0)) {
        ident = true;
    }

    if (bss.set_confusion(0)) {
        ident = true;
    }

    if (bss.set_poison(0)) {
        ident = true;
    }

    if (bss.set_stun(0)) {
        ident = true;
    }

    if (bss.set_cut(0)) {
        ident = true;
    }

    if (set_shero(player_ptr, 0, true)) {
        ident = true;
    }

    return ident;
}

bool true_healing(PlayerType *player_ptr, int pow)
{
    auto ident = false;
    if (hp_player(player_ptr, pow)) {
        ident = true;
    }

    BadStatusSetter bss(player_ptr);
    if (bss.set_blindness(0)) {
        ident = true;
    }

    if (bss.set_confusion(0)) {
        ident = true;
    }

    if (bss.set_poison(0)) {
        ident = true;
    }

    if (bss.set_stun(0)) {
        ident = true;
    }

    if (bss.set_cut(0)) {
        ident = true;
    }

    if (bss.hallucination(0)) {
        ident = true;
    }

    return ident;
}

bool restore_mana(PlayerType *player_ptr, bool magic_eater)
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (PlayerClass(player_ptr).equals(PlayerClassType::MAGIC_EATER) && magic_eater) {
        // 魔力復活による、魔道具術師の取り込んだ魔法の回復量
        // 取り込み数が10回未満: 3 回分回復
        // 取り込み数が10回以上: 取り込み回数/3 回分回復
        auto magic_eater_data = PlayerClass(player_ptr).get_specific_data<MagicEaterDataList>();
        for (auto tval : { ItemKindType::STAFF, ItemKindType::WAND }) {
            for (auto &item : magic_eater_data->get_item_group(tval)) {
                item.charge += (item.count < 10) ? EATER_CHARGE * 3 : item.count * EATER_CHARGE / 3;
                item.charge = std::min(item.charge, item.count * EATER_CHARGE);
            }
        }

        auto sval = 0;
        const auto &baseitems = BaseitemList::get_instance();
        for (auto &item : magic_eater_data->get_item_group(ItemKindType::ROD)) {
            const auto &baseitem = baseitems.lookup_baseitem({ ItemKindType::ROD, sval });
            item.charge -= ((item.count < 10) ? EATER_ROD_CHARGE * 3 : item.count * EATER_ROD_CHARGE / 3) * baseitem.pval;
            item.charge = std::max(item.charge, 0);
            ++sval;
        }

        msg_print(_("頭がハッキリとした。", "You feel your head clear."));
        rfu.set_flag(SubWindowRedrawingFlag::PLAYER);
        return true;
    }

    if (player_ptr->csp >= player_ptr->msp) {
        return false;
    }

    player_ptr->csp = player_ptr->msp;
    player_ptr->csp_frac = 0;
    msg_print(_("頭がハッキリとした。", "You feel your head clear."));
    rfu.set_flag(MainWindowRedrawingFlag::MP);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::SPELL,
        SubWindowRedrawingFlag::PLAYER,
    };
    rfu.set_flags(flags_swrf);
    return true;
}

bool restore_all_status(PlayerType *player_ptr)
{
    bool ident = false;
    if (do_res_stat(player_ptr, A_STR)) {
        ident = true;
    }
    if (do_res_stat(player_ptr, A_INT)) {
        ident = true;
    }
    if (do_res_stat(player_ptr, A_WIS)) {
        ident = true;
    }
    if (do_res_stat(player_ptr, A_DEX)) {
        ident = true;
    }
    if (do_res_stat(player_ptr, A_CON)) {
        ident = true;
    }
    if (do_res_stat(player_ptr, A_CHR)) {
        ident = true;
    }
    return ident;
}

bool fishing(PlayerType *player_ptr)
{
    const auto dir = get_direction(player_ptr);
    if (!dir) {
        return false;
    }

    const auto pos = player_ptr->get_neighbor(dir);
    player_ptr->fishing_dir = dir.dir();
    const auto &floor = *player_ptr->current_floor_ptr;
    if (!floor.has_terrain_characteristics(pos, TerrainCharacteristics::WATER)) {
        msg_print(_("そこは水辺ではない。", "You can't fish here."));
        return false;
    }

    const auto &grid = floor.get_grid(pos);
    if (grid.has_monster()) {
        const auto m_name = monster_desc(player_ptr, floor.m_list[grid.m_idx], 0);
        msg_format(_("%sが邪魔だ！", "%s^ is standing in your way."), m_name.data());
        PlayerEnergy(player_ptr).reset_player_turn();
        return false;
    }

    set_action(player_ptr, ACTION_FISH);
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::ACTION);
    return true;
}

/*!
 * @brief 装備を脱ぎ捨てて小宇宙を燃やす
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param o_ptr_ptr 脱ぐ装備品への参照ポインタのポインタ
 * @return 脱いだらTRUE、脱がなかったらFALSE
 * @details
 * 脱いで落とした装備にtimeoutを設定するために装備品のアドレスを返す。
 */
bool cosmic_cast_off(PlayerType *player_ptr, ItemEntity **o_ptr_ptr)
{
    auto *o_ptr = *o_ptr_ptr;

    /* Cast off activated item */
    INVENTORY_IDX slot;
    for (slot = INVEN_MAIN_HAND; slot <= INVEN_FEET; slot++) {
        if (o_ptr == player_ptr->inventory[slot].get()) {
            break;
        }
    }

    if (slot > INVEN_FEET) {
        return false;
    }

    auto item = o_ptr->clone();
    inven_item_increase(player_ptr, slot, (0 - o_ptr->number));
    inven_item_optimize(player_ptr, slot);

    const auto old_o_idx = drop_near(player_ptr, item, player_ptr->get_position());
    *o_ptr_ptr = player_ptr->current_floor_ptr->o_list[old_o_idx].get();

    const auto item_name = describe_flavor(player_ptr, item, OD_NAME_ONLY);
    msg_format(_("%sを脱ぎ捨てた。", "You cast off %s."), item_name.data());
    sound(SoundKind::TAKE_OFF);

    /* Get effects */
    msg_print(_("「燃え上がれ俺の小宇宙！」", "You say, 'Burn up my cosmo!"));
    TIME_EFFECT t = 20 + randint1(20);
    BadStatusSetter bss(player_ptr);
    (void)bss.mod_blindness(t);
    (void)bss.set_fear(0);
    (void)set_tim_esp(player_ptr, player_ptr->tim_esp + t, false);
    (void)set_tim_regen(player_ptr, player_ptr->tim_regen + t, false);
    (void)set_hero(player_ptr, player_ptr->hero + t, false);
    (void)set_blessed(player_ptr, player_ptr->blessed + t, false);
    (void)mod_acceleration(player_ptr, t, false);
    (void)set_shero(player_ptr, player_ptr->shero + t, false);
    if (PlayerClass(player_ptr).equals(PlayerClassType::FORCETRAINER)) {
        set_current_ki(player_ptr, true, player_ptr->lev * 5 + 190);
        msg_print(_("気が爆発寸前になった。", "Your force absorbs the explosion."));
    }

    return true;
}

/*!
 * @brief プレイヤーの因果混乱処理 / Apply Nexus
 * @param m_ptr 因果混乱をプレイヤーに与えたモンスターの情報参照ポインタ
 */
void apply_nexus(const MonsterEntity &monster, PlayerType *player_ptr)
{
    switch (randint1(7)) {
    case 1:
    case 2:
    case 3: {
        teleport_player(player_ptr, 200, TELEPORT_PASSIVE);
        break;
    }

    case 4:
    case 5: {
        teleport_player_to(player_ptr, monster.fy, monster.fx, TELEPORT_PASSIVE);
        break;
    }

    case 6: {
        if (evaluate_percent(player_ptr->skill_sav)) {
            msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
            break;
        }

        teleport_level(player_ptr, 0);
        break;
    }

    case 7: {
        if (evaluate_percent(player_ptr->skill_sav)) {
            msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
            break;
        }

        msg_print(_("体がねじれ始めた...", "Your body starts to scramble..."));
        status_shuffle(player_ptr);
        break;
    }
    }
}

/*!
 * @brief プレイヤーのステータスシャッフル処理
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void status_shuffle(PlayerType *player_ptr)
{
    /* Pick a pair of stats */
    int i = randint0(A_MAX);
    int j;

    //!< @todo ここのループは一体何をしている？
    for (j = i; j == i; j = randint0(A_MAX)) { /* loop */
        ;
    }

    const auto max1 = player_ptr->stat_max[i];
    const auto cur1 = player_ptr->stat_cur[i];
    const auto max2 = player_ptr->stat_max[j];
    const auto cur2 = player_ptr->stat_cur[j];

    player_ptr->stat_max[i] = max2;
    player_ptr->stat_cur[i] = cur2;
    player_ptr->stat_max[j] = max1;
    player_ptr->stat_cur[j] = cur1;

    for (int k = 0; k < A_MAX; k++) {
        if (player_ptr->stat_max[k] > player_ptr->stat_max_max[k]) {
            player_ptr->stat_max[k] = player_ptr->stat_max_max[k];
        }
        if (player_ptr->stat_cur[k] > player_ptr->stat_max_max[k]) {
            player_ptr->stat_cur[k] = player_ptr->stat_max_max[k];
        }
    }

    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
}
