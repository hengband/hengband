/*!
 * @brief 町の施設処理 / Building commands
 * @date 2013/12/23
 * @author
 * Created by Ken Wigle for Kangband - a variant of Angband 2.8.3
 * -KMW-
 *
 * Rewritten for Kangband 2.8.3i using Kamband's version of
 * building.c as written by Ivan Tkatchev
 *
 * Changed for ZAngband by Robert Ruehlmann
 */

#include "cmd-building/cmd-building.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-building/cmd-inn.h"
#include "cmd-io/cmd-dump.h"
#include "core/asking-player.h"
#include "core/scores.h"
#include "core/show-file.h"
#include "core/special-internal-keys.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "floor/cave.h"
#include "floor/floor-events.h"
#include "floor/floor-mode-changer.h"
#include "floor/wild.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/music-definitions-table.h"
#include "main/sound-of-music.h"
#include "market/arena.h"
#include "market/bounty.h"
#include "market/building-actions-table.h"
#include "market/building-craft-armor.h"
#include "market/building-craft-fix.h"
#include "market/building-craft-weapon.h"
#include "market/building-enchanter.h"
#include "market/building-monster.h"
#include "market/building-quest.h"
#include "market/building-recharger.h"
#include "market/building-service.h"
#include "market/building-util.h"
#include "market/play-gamble.h"
#include "market/poker.h"
#include "monster-race/monster-race.h"
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-investor-remover.h"
#include "object-hook/hook-armor.h"
#include "object-hook/hook-weapon.h"
#include "object/item-tester-hooker.h"
#include "player-status/player-energy.h"
#include "player/player-personality-types.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-world.h"
#include "spell/spells-status.h"
#include "system/angband-system.h"
#include "system/building-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "world/world.h"

uint32_t mon_odds[4];
int battle_odds;
PRICE kakekin;
int sel_monster;

bool reinit_wilderness = false;

/*!
 * @brief 町に関するヘルプを表示する / Display town history
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void town_history(PlayerType *player_ptr)
{
    screen_save();
    (void)show_file(player_ptr, true, _("jbldg.txt", "bldg.txt"), 0, 0);
    screen_load();
}

/*!
 * @brief 施設の処理実行メインルーチン / Execute a building command
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param bldg 施設構造体の参照ポインタ
 * @param i 実行したい施設のサービステーブルの添字
 */
static void bldg_process_command(PlayerType *player_ptr, building_type *bldg, int i)
{
    msg_flag = false;
    msg_erase();

    PRICE bcost;
    if (is_owner(player_ptr, bldg)) {
        bcost = bldg->member_costs[i];
    } else {
        bcost = bldg->other_costs[i];
    }

    /* action restrictions */
    const auto can_be_owner = is_owner(player_ptr, bldg);
    if (((bldg->action_restr[i] == 1) && !is_member(player_ptr, bldg)) || ((bldg->action_restr[i] == 2) && !can_be_owner)) {
        msg_print(_("それを選択する権利はありません！", "You have no right to choose that!"));
        return;
    }

    auto bact = bldg->actions[i];
    if ((bact != BACT_RECHARGE) && (((bldg->member_costs[i] > player_ptr->au) && can_be_owner) || ((bldg->other_costs[i] > player_ptr->au) && !can_be_owner))) {
        msg_print(_("お金が足りません！", "You do not have the gold!"));
        return;
    }

    bool paid = false;
    switch (bact) {
    case BACT_NOTHING:
        /* Do nothing */
        break;
    case BACT_RESEARCH_ITEM:
        paid = identify_fully(player_ptr, false);
        break;
    case BACT_TOWN_HISTORY:
        town_history(player_ptr);
        break;
    case BACT_RACE_LEGENDS:
        race_legends(player_ptr);
        break;
    case BACT_QUEST:
        castle_quest(player_ptr);
        break;
    case BACT_KING_LEGENDS:
    case BACT_ARENA_LEGENDS:
    case BACT_LEGENDS:
        show_highclass(player_ptr);
        break;
    case BACT_POSTER:
    case BACT_ARENA_RULES:
    case BACT_ARENA:
        arena_comm(player_ptr, bact);
        break;
    case BACT_IN_BETWEEN:
    case BACT_CRAPS:
    case BACT_SPIN_WHEEL:
    case BACT_DICE_SLOTS:
    case BACT_GAMBLE_RULES:
    case BACT_POKER:
        gamble_comm(player_ptr, bact);
        break;
    case BACT_REST:
    case BACT_RUMORS:
    case BACT_FOOD:
        paid = inn_comm(player_ptr, bact);
        break;
    case BACT_RESEARCH_MONSTER:
        paid = research_mon(player_ptr);
        break;
    case BACT_COMPARE_WEAPONS:
        paid = true;
        bcost = compare_weapons(player_ptr, bcost);
        break;
    case BACT_ENCHANT_WEAPON:
        enchant_item(player_ptr, bcost, 1, 1, 0, FuncItemTester(&ItemEntity::allow_enchant_melee_weapon));
        break;
    case BACT_ENCHANT_ARMOR:
        enchant_item(player_ptr, bcost, 0, 0, 1, FuncItemTester(&ItemEntity::is_protector));
        break;
    case BACT_RECHARGE:
        building_recharge(player_ptr);
        break;
    case BACT_RECHARGE_ALL:
        building_recharge_all(player_ptr);
        break;
    case BACT_IDENTS:
        if (!input_check(_("持ち物を全て鑑定してよろしいですか？", "Do you pay to identify all your possession? "))) {
            break;
        }
        identify_pack(player_ptr);
        msg_print(_(" 持ち物全てが鑑定されました。", "Your possessions have been identified."));
        paid = true;
        break;
    case BACT_IDENT_ONE:
        paid = ident_spell(player_ptr, false);
        break;
    case BACT_LEARN:
        do_cmd_study(player_ptr);
        break;
    case BACT_HEALING:
        paid = cure_critical_wounds(player_ptr, 200);
        break;
    case BACT_RESTORE:
        paid = restore_all_status(player_ptr);
        break;
    case BACT_ENCHANT_ARROWS:
        enchant_item(player_ptr, bcost, 1, 1, 0, FuncItemTester(&ItemEntity::is_ammo));
        break;
    case BACT_ENCHANT_BOW:
        enchant_item(player_ptr, bcost, 1, 1, 0, TvalItemTester(ItemKindType::BOW));
        break;

    case BACT_RECALL:
        if (recall_player(player_ptr, 1)) {
            paid = true;
        }
        break;

    case BACT_TELEPORT_LEVEL:
        screen_save();
        clear_bldg(4, 20);
        paid = free_level_recall(player_ptr);
        screen_load();
        break;

    case BACT_LOSE_MUTATION: {
        auto muta = player_ptr->muta;
        if (player_ptr->ppersonality == PERSONALITY_LUCKY) {
            // ラッキーマンの白オーラは突然変異治療の対象外
            muta.reset(PlayerMutationType::GOOD_LUCK);
        }
        if (muta.any()) {
            while (!lose_mutation(player_ptr, 0)) {
                ;
            }
            paid = true;
            break;
        }

        msg_print(_("治すべき突然変異が無い。", "You have no mutations."));
        msg_print(nullptr);
        break;
    }

    case BACT_BATTLE:
        monster_arena_comm(player_ptr);
        break;

    case BACT_TSUCHINOKO:
        tsuchinoko();
        break;

    case BACT_BOUNTY:
        show_bounty();
        break;

    case BACT_TARGET:
        today_target(player_ptr);
        break;

    case BACT_KANKIN:
        exchange_cash(player_ptr);
        break;

    case BACT_HEIKOUKA:
        msg_print(_("平衡化の儀式を行なった。", "You received an equalization ritual."));
        set_virtue(player_ptr, Virtue::COMPASSION, 0);
        set_virtue(player_ptr, Virtue::HONOUR, 0);
        set_virtue(player_ptr, Virtue::JUSTICE, 0);
        set_virtue(player_ptr, Virtue::SACRIFICE, 0);
        set_virtue(player_ptr, Virtue::KNOWLEDGE, 0);
        set_virtue(player_ptr, Virtue::FAITH, 0);
        set_virtue(player_ptr, Virtue::ENLIGHTEN, 0);
        set_virtue(player_ptr, Virtue::ENCHANT, 0);
        set_virtue(player_ptr, Virtue::CHANCE, 0);
        set_virtue(player_ptr, Virtue::NATURE, 0);
        set_virtue(player_ptr, Virtue::HARMONY, 0);
        set_virtue(player_ptr, Virtue::VITALITY, 0);
        set_virtue(player_ptr, Virtue::UNLIFE, 0);
        set_virtue(player_ptr, Virtue::PATIENCE, 0);
        set_virtue(player_ptr, Virtue::TEMPERANCE, 0);
        set_virtue(player_ptr, Virtue::DILIGENCE, 0);
        set_virtue(player_ptr, Virtue::VALOUR, 0);
        set_virtue(player_ptr, Virtue::INDIVIDUALISM, 0);
        initialize_virtues(player_ptr);
        paid = true;
        break;

    case BACT_TELE_TOWN:
        paid = tele_town(player_ptr);
        break;

    case BACT_EVAL_AC:
        paid = eval_ac(player_ptr->dis_ac + player_ptr->dis_to_a);
        break;

    case BACT_BROKEN_WEAPON:
        paid = true;
        bcost = repair_broken_weapon(player_ptr, bcost);
        break;
    }

    if (paid) {
        player_ptr->au -= bcost;
    }
}

/*!
 * @brief 施設入り口にプレイヤーが乗った際の処理 / Do building commands
 * @param プレイヤーへの参照ポインタ
 */
void do_cmd_building(PlayerType *player_ptr)
{
    if (player_ptr->wild_mode) {
        return;
    }

    PlayerEnergy energy(player_ptr);
    energy.set_player_turn_energy(100);
    const auto p_pos = player_ptr->get_position();
    if (!cave_has_flag_bold(player_ptr->current_floor_ptr, p_pos.y, p_pos.x, TerrainCharacteristics::BLDG)) {
        msg_print(_("ここには建物はない。", "You see no building here."));
        return;
    }

    int which = player_ptr->current_floor_ptr->get_grid(p_pos).get_terrain().subtype;

    building_type *bldg;
    bldg = &buildings[which];

    reinit_wilderness = false;

    if ((which == 2) && (player_ptr->arena_number < 0)) {
        msg_print(_("「敗者に用はない。」", "'There's no place here for a LOSER like you!'"));
        return;
    }

    if ((which == 2) && player_ptr->current_floor_ptr->inside_arena) {
        if (!player_ptr->exit_bldg && player_ptr->current_floor_ptr->m_cnt > 0) {
            prt(_("ゲートは閉まっている。モンスターがあなたを待っている！", "The gates are closed.  The monster awaits!"), 0, 0);
        } else {
            prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS | CFM_NO_RETURN);
            player_ptr->current_floor_ptr->inside_arena = false;
            player_ptr->leaving = true;
            command_new = SPECIAL_KEY_BUILDING;
            energy.reset_player_turn();
        }

        return;
    }

    TermCenteredOffsetSetter tcos(MAIN_TERM_MIN_COLS, MAIN_TERM_MIN_ROWS);

    auto &system = AngbandSystem::get_instance();
    if (system.is_phase_out()) {
        prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS | CFM_NO_RETURN);
        player_ptr->leaving = true;
        system.set_phase_out(false);
        command_new = SPECIAL_KEY_BUILDING;
        energy.reset_player_turn();
        return;
    }

    player_ptr->oldpy = player_ptr->y;
    player_ptr->oldpx = player_ptr->x;
    forget_lite(player_ptr->current_floor_ptr);
    forget_view(player_ptr->current_floor_ptr);
    w_ptr->character_icky_depth++;

    command_arg = 0;
    command_rep = 0;
    command_new = 0;

    display_buikding_service(player_ptr, bldg);
    player_ptr->leave_bldg = false;
    play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_BUILD);

    bool validcmd;
    while (!player_ptr->leave_bldg) {
        validcmd = false;
        prt("", 1, 0);

        building_prt_gold(player_ptr);

        char command = inkey();

        if (command == ESCAPE) {
            player_ptr->leave_bldg = true;
            player_ptr->current_floor_ptr->inside_arena = false;
            system.set_phase_out(false);
            break;
        }

        int i;
        for (i = 0; i < 8; i++) {
            if (bldg->letters[i] && (bldg->letters[i] == command)) {
                validcmd = true;
                break;
            }
        }

        if (validcmd) {
            bldg_process_command(player_ptr, bldg, i);
        }

        handle_stuff(player_ptr);
    }

    select_floor_music(player_ptr);

    msg_flag = false;
    msg_erase();

    if (reinit_wilderness) {
        player_ptr->leaving = true;
    }

    w_ptr->character_icky_depth--;
    term_clear();

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::VIEW,
        StatusRecalculatingFlag::MONSTER_STATUSES,
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::LITE,
        StatusRecalculatingFlag::MONSTER_LITE,
    };
    rfu.set_flags(flags_srf);
    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::BASIC,
        MainWindowRedrawingFlag::EXTRA,
        MainWindowRedrawingFlag::EQUIPPY,
        MainWindowRedrawingFlag::MAP,
    };
    rfu.set_flags(flags_mwrf);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    rfu.set_flags(flags_swrf);
}
