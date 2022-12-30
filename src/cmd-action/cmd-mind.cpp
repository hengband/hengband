/*!
 * @brief 各職業の特殊技能実装 / Special magics
 * @date 2014/01/15
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2005 henkma \n
 * 2014 Deskull rearranged comment for Doxygen.\n
 */

#include "cmd-action/cmd-mind.h"
#include "action/action-limited.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "core/window-redrawer.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-berserker.h"
#include "mind/mind-explanations-table.h"
#include "mind/mind-force-trainer.h"
#include "mind/mind-mindcrafter.h"
#include "mind/mind-mirror-master.h"
#include "mind/mind-ninja.h"
#include "mind/mind-numbers.h"
#include "mind/mind-power-getter.h"
#include "mind/mind-types.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/equipment-info.h"
#include "player-status/player-energy.h"
#include "player/player-damage.h"
#include "player/player-status-table.h"
#include "player/player-status.h"
#include "spell-kind/spells-teleport.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"
#include "view/display-util.h"

/*!
 * @brief 職業別特殊技能の処理用構造体
 */
struct cm_type {
    MindKindType use_mind; //!< 使った職業技能構造体
    concptr mind_explanation; //!< 特殊技能の説明
    SPELL_IDX n; //!< 職業種別毎ID
    int b; //!< 失敗時チャート用乱数
    PERCENTAGE chance; //!< 失敗率
    PERCENTAGE minfail; //!< 最低失敗率
    PLAYER_LEVEL plev; //!< 取得レベル
    int old_csp; //!< 行使前のMP
    mind_type spell; //!< 職業技能の基本設定構造体
    bool cast; //!< 行使の成否
    int mana_cost; //!< 最終算出消費MP
    bool on_mirror; //!< 鏡の上に乗っているどうかの判定
};

/*!
 * @brief 職業技能処理構造体の初期化
 */
static cm_type *initialize_cm_type(PlayerType *player_ptr, cm_type *cm_ptr)
{
    cm_ptr->n = 0;
    cm_ptr->b = 0;
    cm_ptr->minfail = 0;
    cm_ptr->plev = player_ptr->lev;
    cm_ptr->old_csp = player_ptr->csp;
    cm_ptr->on_mirror = false;
    return cm_ptr;
}

/*!
 * @brief 職業別の行使可能な技能種別を構造体に付加する
 */
static void switch_mind_kind(PlayerType *player_ptr, cm_type *cm_ptr)
{
    switch (player_ptr->pclass) {
    case PlayerClassType::MINDCRAFTER:
        cm_ptr->use_mind = MindKindType::MINDCRAFTER;
        cm_ptr->mind_explanation = _("精神", "skill");
        break;
    case PlayerClassType::FORCETRAINER:
        cm_ptr->use_mind = MindKindType::KI;
        cm_ptr->mind_explanation = _("気", "skill");
        break;
    case PlayerClassType::BERSERKER:
        cm_ptr->use_mind = MindKindType::BERSERKER;
        cm_ptr->mind_explanation = _("怒り", "skill");
        break;
    case PlayerClassType::MIRROR_MASTER:
        cm_ptr->use_mind = MindKindType::MIRROR_MASTER;
        cm_ptr->mind_explanation = _("鏡魔法", "skill");
        break;
    case PlayerClassType::NINJA:
        cm_ptr->use_mind = MindKindType::NINJUTSU;
        cm_ptr->mind_explanation = _("精神", "skill");
        break;
    default:
        cm_ptr->use_mind = (MindKindType)0;
        cm_ptr->mind_explanation = _("超能力", "skill");
        break;
    }
}

static void decide_mind_ki_chance(PlayerType *player_ptr, cm_type *cm_ptr)
{
    if (cm_ptr->use_mind != MindKindType::KI) {
        return;
    }

    if (heavy_armor(player_ptr)) {
        cm_ptr->chance += 20;
    }

    if (player_ptr->is_icky_wield[0]) {
        cm_ptr->chance += 20;
    } else if (has_melee_weapon(player_ptr, INVEN_MAIN_HAND)) {
        cm_ptr->chance += 10;
    }

    if (player_ptr->is_icky_wield[1]) {
        cm_ptr->chance += 20;
    } else if (has_melee_weapon(player_ptr, INVEN_SUB_HAND)) {
        cm_ptr->chance += 10;
    }

    if (cm_ptr->n == 5) {
        for (int j = 0; j < get_current_ki(player_ptr) / 50; j++) {
            cm_ptr->mana_cost += (j + 1) * 3 / 2;
        }
    }
}

static bool check_mind_hp_mp_sufficiency(PlayerType *player_ptr, cm_type *cm_ptr)
{
    if ((cm_ptr->use_mind == MindKindType::BERSERKER) || (cm_ptr->use_mind == MindKindType::NINJUTSU)) {
        if (cm_ptr->mana_cost > player_ptr->chp) {
            msg_print(_("ＨＰが足りません。", "You do not have enough hp to use this power."));
            return false;
        }

        return true;
    }

    if (cm_ptr->mana_cost <= player_ptr->csp) {
        return true;
    }

    msg_print(_("ＭＰが足りません。", "You do not have enough mana to use this power."));
    if (!over_exert) {
        return false;
    }

    return get_check(_("それでも挑戦しますか? ", "Attempt it anyway? "));
}

static void decide_mind_chance(PlayerType *player_ptr, cm_type *cm_ptr)
{
    if (cm_ptr->chance == 0) {
        return;
    }

    cm_ptr->chance -= 3 * (cm_ptr->plev - cm_ptr->spell.min_lev);
    cm_ptr->chance += player_ptr->to_m_chance;
    cm_ptr->chance -= 3 * (adj_mag_stat[player_ptr->stat_index[mp_ptr->spell_stat]] - 1);
    if ((cm_ptr->mana_cost > player_ptr->csp) && (cm_ptr->use_mind != MindKindType::BERSERKER) && (cm_ptr->use_mind != MindKindType::NINJUTSU)) {
        cm_ptr->chance += 5 * (cm_ptr->mana_cost - player_ptr->csp);
    }

    cm_ptr->minfail = adj_mag_fail[player_ptr->stat_index[mp_ptr->spell_stat]];
    if (cm_ptr->chance < cm_ptr->minfail) {
        cm_ptr->chance = cm_ptr->minfail;
    }

    auto player_stun = player_ptr->effects()->stun();
    cm_ptr->chance += player_stun->get_magic_chance_penalty();

    if (cm_ptr->use_mind != MindKindType::KI) {
        return;
    }

    if (heavy_armor(player_ptr)) {
        cm_ptr->chance += 5;
    }

    if (player_ptr->is_icky_wield[0]) {
        cm_ptr->chance += 5;
    }

    if (player_ptr->is_icky_wield[1]) {
        cm_ptr->chance += 5;
    }
}

static void check_mind_mindcrafter(PlayerType *player_ptr, cm_type *cm_ptr)
{
    if (cm_ptr->use_mind != MindKindType::MINDCRAFTER) {
        return;
    }

    if (cm_ptr->b < 5) {
        msg_print(_("なんてこった！頭の中が真っ白になった！", "Oh, no! Your mind has gone blank!"));
        lose_all_info(player_ptr);
        return;
    }

    BadStatusSetter bss(player_ptr);
    if (cm_ptr->b < 15) {
        msg_print(_("奇妙な光景が目の前で踊っている...", "Weird visions seem to dance before your eyes..."));
        (void)bss.mod_hallucination(5 + randint1(10));
        return;
    }

    if (cm_ptr->b < 45) {
        msg_print(_("あなたの頭は混乱した！", "Your brain is addled!"));
        (void)bss.mod_confusion(randint1(8));
        return;
    }

    if (cm_ptr->b < 90) {
        (void)bss.mod_stun(randint1(8));
        return;
    }

    msg_print(_(format("%sの力が制御できない氾流となって解放された！", cm_ptr->mind_explanation), "Your mind unleashes its power in an uncontrollable storm!"));
    project(player_ptr, PROJECT_WHO_UNCTRL_POWER, 2 + cm_ptr->plev / 10, player_ptr->y, player_ptr->x, cm_ptr->plev * 2, AttributeType::MANA,
        PROJECT_JUMP | PROJECT_KILL | PROJECT_GRID | PROJECT_ITEM);
    player_ptr->csp = std::max(0, player_ptr->csp - cm_ptr->plev * std::max(1, cm_ptr->plev / 10));
}

static void check_mind_mirror_master(PlayerType *player_ptr, cm_type *cm_ptr)
{
    if (cm_ptr->use_mind != MindKindType::MIRROR_MASTER) {
        return;
    }

    if (cm_ptr->b < 51) {
        return;
    }

    if (cm_ptr->b < 81) {
        msg_print(_("鏡の世界の干渉を受けた！", "Weird visions seem to dance before your eyes..."));
        teleport_player(player_ptr, 10, TELEPORT_PASSIVE);
        return;
    }

    if (cm_ptr->b < 96) {
        msg_print(_("まわりのものがキラキラ輝いている！", "Your brain is addled!"));
        (void)BadStatusSetter(player_ptr).mod_hallucination(5 + randint1(10));
        return;
    }

    msg_print(_(format("%sの力が制御できない氾流となって解放された！", cm_ptr->mind_explanation), "Your mind unleashes its power in an uncontrollable storm!"));
    project(player_ptr, PROJECT_WHO_UNCTRL_POWER, 2 + cm_ptr->plev / 10, player_ptr->y, player_ptr->x, cm_ptr->plev * 2, AttributeType::MANA,
        PROJECT_JUMP | PROJECT_KILL | PROJECT_GRID | PROJECT_ITEM);
    player_ptr->csp = std::max(0, player_ptr->csp - cm_ptr->plev * std::max(1, cm_ptr->plev / 10));
}

static void check_mind_class(PlayerType *player_ptr, cm_type *cm_ptr)
{
    if ((cm_ptr->use_mind == MindKindType::BERSERKER) || (cm_ptr->use_mind == MindKindType::NINJUTSU)) {
        return;
    }

    if ((cm_ptr->use_mind == MindKindType::KI) && (cm_ptr->n != 5) && get_current_ki(player_ptr)) {
        msg_print(_("気が散ってしまった．．．", "Your improved Force has gone away..."));
        set_current_ki(player_ptr, true, 0);
    }

    if (randint1(100) >= (cm_ptr->chance / 2)) {
        return;
    }

    cm_ptr->b = randint1(100);
    check_mind_mindcrafter(player_ptr, cm_ptr);
    check_mind_mirror_master(player_ptr, cm_ptr);
}

static bool switch_mind_class(PlayerType *player_ptr, cm_type *cm_ptr)
{
    switch (cm_ptr->use_mind) {
    case MindKindType::MINDCRAFTER:
        cm_ptr->cast = cast_mindcrafter_spell(player_ptr, i2enum<MindMindcrafterType>(cm_ptr->n));
        return true;
    case MindKindType::KI:
        cm_ptr->cast = cast_force_spell(player_ptr, i2enum<MindForceTrainerType>(cm_ptr->n));
        return true;
    case MindKindType::BERSERKER:
        cm_ptr->cast = cast_berserk_spell(player_ptr, i2enum<MindBerserkerType>(cm_ptr->n));
        return true;
    case MindKindType::MIRROR_MASTER:
        if (player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].is_mirror()) {
            cm_ptr->on_mirror = true;
        }

        cm_ptr->cast = cast_mirror_spell(player_ptr, i2enum<MindMirrorMasterType>(cm_ptr->n));
        return true;
    case MindKindType::NINJUTSU:
        cm_ptr->cast = cast_ninja_spell(player_ptr, i2enum<MindNinjaType>(cm_ptr->n));
        return true;
    default:
        msg_format(_("謎の能力:%d, %d", "Mystery power:%d, %d"), enum2i(cm_ptr->use_mind), cm_ptr->n);
        return false;
    }
}

static void mind_turn_passing(PlayerType *player_ptr, cm_type *cm_ptr)
{
    PlayerEnergy energy(player_ptr);
    if (cm_ptr->on_mirror && PlayerClass(player_ptr).equals(PlayerClassType::MIRROR_MASTER)) {
        if (cm_ptr->n == 3 || cm_ptr->n == 5 || cm_ptr->n == 7 || cm_ptr->n == 16) {
            energy.set_player_turn_energy(50);
            return;
        }
    }

    energy.set_player_turn_energy(100);
}

static bool judge_mind_chance(PlayerType *player_ptr, cm_type *cm_ptr)
{
    if (randint0(100) >= cm_ptr->chance) {
        sound(SOUND_ZAP);
        return switch_mind_class(player_ptr, cm_ptr) && cm_ptr->cast;
    }

    if (flush_failure) {
        flush();
    }

    msg_format(_("%sの集中に失敗した！", "You failed to concentrate hard enough for %s!"), cm_ptr->mind_explanation);
    sound(SOUND_FAIL);
    check_mind_class(player_ptr, cm_ptr);
    return true;
}

static void mind_reflection(PlayerType *player_ptr, cm_type *cm_ptr)
{
    int oops = cm_ptr->mana_cost - cm_ptr->old_csp;
    if ((player_ptr->csp - cm_ptr->mana_cost) < 0) {
        player_ptr->csp_frac = 0;
    }

    player_ptr->csp = std::max(0, player_ptr->csp - cm_ptr->mana_cost);
    msg_print(_(format("%sを集中しすぎて気を失ってしまった！", cm_ptr->mind_explanation), "You faint from the effort!"));
    (void)BadStatusSetter(player_ptr).mod_paralysis(randint1(5 * oops + 1));
    if (randint0(100) >= 50) {
        return;
    }

    bool perm = randint0(100) < 25;
    msg_print(_("自分の精神を攻撃してしまった！", "You have damaged your mind!"));
    (void)dec_stat(player_ptr, A_WIS, 15 + randint1(10), perm);
}

static void process_hard_concentration(PlayerType *player_ptr, cm_type *cm_ptr)
{
    if ((cm_ptr->use_mind == MindKindType::BERSERKER) || (cm_ptr->use_mind == MindKindType::NINJUTSU)) {
        take_hit(player_ptr, DAMAGE_USELIFE, cm_ptr->mana_cost, _("過度の集中", "concentrating too hard"));
        player_ptr->redraw |= PR_HP;
        return;
    }

    if (cm_ptr->mana_cost > cm_ptr->old_csp) {
        mind_reflection(player_ptr, cm_ptr);
        return;
    }

    player_ptr->csp -= cm_ptr->mana_cost;
    if (player_ptr->csp < 0) {
        player_ptr->csp = 0;
    }

    if ((cm_ptr->use_mind == MindKindType::MINDCRAFTER) && (cm_ptr->n == 13)) {
        player_ptr->csp = 0;
        player_ptr->csp_frac = 0;
    }
}

/*!
 * @brief 特殊技能コマンドのメインルーチン /
 */
void do_cmd_mind(PlayerType *player_ptr)
{
    cm_type tmp_cm;
    cm_type *cm_ptr = initialize_cm_type(player_ptr, &tmp_cm);
    if (cmd_limit_confused(player_ptr) || !MindPowerGetter(player_ptr).get_mind_power(&cm_ptr->n, false)) {
        return;
    }

    switch_mind_kind(player_ptr, cm_ptr);
    cm_ptr->spell = mind_powers[enum2i(cm_ptr->use_mind)].info[cm_ptr->n];
    cm_ptr->chance = cm_ptr->spell.fail;
    cm_ptr->mana_cost = cm_ptr->spell.mana_cost;
    decide_mind_ki_chance(player_ptr, cm_ptr);
    if (!check_mind_hp_mp_sufficiency(player_ptr, cm_ptr)) {
        return;
    }

    decide_mind_chance(player_ptr, cm_ptr);
    if (cm_ptr->chance > 95) {
        cm_ptr->chance = 95;
    }

    if (!judge_mind_chance(player_ptr, cm_ptr)) {
        return;
    }

    mind_turn_passing(player_ptr, cm_ptr);
    process_hard_concentration(player_ptr, cm_ptr);
    player_ptr->redraw |= PR_MANA;
    player_ptr->window_flags |= PW_PLAYER;
    player_ptr->window_flags |= PW_SPELL;
}

static MindKindType decide_use_mind_browse(PlayerType *player_ptr)
{
    switch (player_ptr->pclass) {
    case PlayerClassType::MINDCRAFTER:
        return MindKindType::MINDCRAFTER;
    case PlayerClassType::FORCETRAINER:
        return MindKindType::KI;
    case PlayerClassType::BERSERKER:
        return MindKindType::BERSERKER;
    case PlayerClassType::NINJA:
        return MindKindType::NINJUTSU;
    case PlayerClassType::MIRROR_MASTER:
        return MindKindType::MIRROR_MASTER;
    default:
        return (MindKindType)0; // 実質PlayerClassType::MINDCRAFTERと同じ.
    }
}

/*!
 * @brief 現在プレイヤーが使用可能な特殊技能の一覧表示 /
 */
void do_cmd_mind_browse(PlayerType *player_ptr)
{
    SPELL_IDX n = 0;
    MindKindType use_mind = decide_use_mind_browse(player_ptr);
    screen_save();
    while (true) {
        if (!MindPowerGetter(player_ptr).get_mind_power(&n, true)) {
            screen_load();
            return;
        }

        term_erase(12, 21, 255);
        term_erase(12, 20, 255);
        term_erase(12, 19, 255);
        term_erase(12, 18, 255);
        term_erase(12, 17, 255);
        term_erase(12, 16, 255);
        display_wrap_around(mind_tips[enum2i(use_mind)][n], 62, 17, 15);

        switch (use_mind) {
        case MindKindType::MIRROR_MASTER:
        case MindKindType::NINJUTSU:
            prt(_("何かキーを押して下さい。", "Hit any key."), 0, 0);
            (void)inkey();
            break;
        default:
            break;
        }
    }
}
