#include "cmd-action/cmd-pet.h"
#include "action/action-limited.h"
#include "cmd-action/cmd-attack.h"
#include "cmd-io/cmd-dump.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "effect/spells-effect-util.h"
#include "floor/geometry.h"
#include "floor/pattern-walk.h"
#include "game-option/input-options.h"
#include "game-option/map-screen-options.h"
#include "game-option/play-record-options.h"
#include "game-option/text-display-options.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "io/command-repeater.h"
#include "io/cursor.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "io/write-diary.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-object.h"
#include "monster-floor/monster-remover.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "object-hook/hook-weapon.h"
#include "pet/pet-util.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/equipment-info.h"
#include "player-info/samurai-data-type.h"
#include "player-status/player-energy.h"
#include "player-status/player-hand-types.h"
#include "player/attack-defense-types.h"
#include "player/player-damage.h"
#include "player/player-move.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "status/action-setter.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "term/screen-processor.h"
#include "timed-effect/timed-effects.h"
#include "tracking/health-bar-tracker.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <sstream>

/*!
 * @brief ペットを開放するコマンドのメインルーチン
 */
void do_cmd_pet_dismiss(PlayerType *player_ptr)
{
    auto cu = game_term->scr->cu;
    auto cv = game_term->scr->cv;
    game_term->scr->cu = false;
    game_term->scr->cv = true;
    const auto &floor = *player_ptr->current_floor_ptr;
    std::vector<short> pet_index;
    for (short pet_indice = floor.m_max - 1; pet_indice >= 1; pet_indice--) {
        const auto &monster = floor.m_list[pet_indice];
        if (monster.is_pet()) {
            pet_index.push_back(pet_indice);
        }
    }

    const auto riding_index = player_ptr->riding;
    std::stable_sort(pet_index.begin(), pet_index.end(),
        [&floor, riding_index](auto x, auto y) { return floor.order_pet_dismission(x, y, riding_index); });

    /* Process the monsters (backwards) */
    auto all_pets = false;
    auto num_dismissed = 0;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const int num_pet_index = std::ssize(pet_index);
    for (auto i = 0; i < num_pet_index; i++) {
        const auto pet_ctr = pet_index[i];
        const auto &monster = floor.m_list[pet_ctr];
        auto delete_this = false;
        const auto should_ask = (pet_ctr == riding_index) || monster.is_named();
        const auto friend_name = monster_desc(player_ptr, monster, MD_ASSUME_VISIBLE);
        if (!all_pets) {
            health_track(player_ptr, pet_ctr);
            handle_stuff(player_ptr);
            constexpr auto mes = _("%sを放しますか？ [Yes/No/Unnamed (%d体)]", "Dismiss %s? [Yes/No/Unnamed (%d remain)]");
            msg_format(mes, friend_name.data(), num_pet_index - i);
            if (monster.ml) {
                move_cursor_relative(monster.fy, monster.fx);
            }

            while (true) {
                auto ch = inkey();
                if (ch == 'Y' || ch == 'y') {
                    delete_this = true;
                    if (should_ask) {
                        msg_format(_("本当によろしいですか？ (%s) ", "Are you sure? (%s) "), friend_name.data());
                        ch = inkey();
                        if (ch != 'Y' && ch != 'y') {
                            delete_this = false;
                        }
                    }

                    break;
                }

                if (ch == 'U' || ch == 'u') {
                    all_pets = true;
                    break;
                }

                if (ch == ESCAPE || ch == 'N' || ch == 'n') {
                    break;
                }

                bell();
            }
        }

        if ((all_pets && !should_ask) || (!all_pets && delete_this)) {
            if (record_named_pet && monster.is_named()) {
                const auto m_name = monster_desc(player_ptr, monster, MD_INDEF_VISIBLE);
                exe_write_diary(floor, DiaryKind::NAMED_PET, RECORD_NAMED_PET_DISMISS, m_name);
            }

            if (monster.is_riding()) {
                msg_format(_("%sから降りた。", "You dismount from %s. "), friend_name.data());
                player_ptr->ride_monster(0);
                rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
                static constexpr auto flags = {
                    MainWindowRedrawingFlag::EXTRA,
                    MainWindowRedrawingFlag::UHEALTH,
                };
                rfu.set_flags(flags);
            }

            msg_format(_("%s を放した。", "Dismissed %s."), friend_name.data());
            rfu.set_flag(StatusRecalculatingFlag::BONUS);
            rfu.set_flag(SubWindowRedrawingFlag::MESSAGE);
            delete_monster_idx(player_ptr, pet_ctr);
            num_dismissed++;
        }
    }

    game_term->scr->cu = cu;
    game_term->scr->cv = cv;
    term_fresh();

#ifdef JP
    msg_format("%d 体のペットを放しました。", num_dismissed);
#else
    msg_format("You have dismissed %d pet%s.", num_dismissed, (num_dismissed == 1 ? "" : "s"));
#endif
    if ((num_dismissed == 0) && all_pets) {
        msg_print(_("'U'nnamed は、乗馬以外の名前のないペットだけを全て解放します。", "'U'nnamed means all your pets except named pets and your mount."));
    }

    handle_stuff(player_ptr);
}

/*!
 * @brief ペットから騎乗/下馬するコマンドのメインルーチン /
 * @param force 強制的に騎乗/下馬するならばTRUE
 * @return 騎乗/下馬できたらTRUE
 */
bool do_cmd_riding(PlayerType *player_ptr, bool force)
{
    const auto dir = get_direction(player_ptr);
    if (!dir) {
        return false;
    }

    const auto pos = player_ptr->get_neighbor(dir);
    auto &grid = player_ptr->current_floor_ptr->get_grid(pos);

    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });

    if (player_ptr->riding) {
        /* Skip non-empty grids */
        if (!can_player_ride_pet(player_ptr, grid, false)) {
            msg_print(_("そちらには降りられません。", "You cannot go that direction."));
            return false;
        }

        if (!pattern_seq(player_ptr, pos)) {
            return false;
        }

        if (grid.has_monster()) {
            PlayerEnergy(player_ptr).set_player_turn_energy(100);

            msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));

            do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_NONE);
            return false;
        }

        player_ptr->ride_monster(0);
        player_ptr->pet_extra_flags &= ~(PF_TWO_HANDS);
        player_ptr->riding_ryoute = player_ptr->old_riding_ryoute = false;
    } else {
        if (cmd_limit_confused(player_ptr)) {
            return false;
        }

        const auto &monster = player_ptr->current_floor_ptr->m_list[grid.m_idx];

        if (!grid.has_monster() || !monster.ml) {
            msg_print(_("その場所にはモンスターはいません。", "There is no monster here."));
            return false;
        }
        if (!monster.is_pet() && !force) {
            msg_print(_("そのモンスターはペットではありません。", "That monster is not a pet."));
            return false;
        }
        if (monster.get_monrace().misc_flags.has_not(MonsterMiscType::RIDING)) {
            msg_print(_("そのモンスターには乗れなさそうだ。", "This monster doesn't seem suitable for riding."));
            return false;
        }

        if (!pattern_seq(player_ptr, pos)) {
            return false;
        }

        if (!can_player_ride_pet(player_ptr, grid, true)) {
            /* Feature code (applying "mimic" field) */
            const auto &terrain = grid.get_terrain(TerrainKind::MIMIC);
            using Tc = TerrainCharacteristics;
#ifdef JP
            msg_format("そのモンスターは%sの%sにいる。", terrain.name.data(),
                (terrain.flags.has_none_of({ Tc::MOVE, Tc::CAN_FLY }) || terrain.flags.has_none_of({ Tc::LOS, Tc::TREE })) ? "中" : "上");
#else
            msg_format("This monster is %s the %s.",
                (terrain.flags.has_none_of({ Tc::MOVE, Tc::CAN_FLY }) || terrain.flags.has_none_of({ Tc::LOS, Tc::TREE })) ? "in" : "on", terrain.name.data());
#endif

            return false;
        }
        if (monster.get_monrace().level > randint1((player_ptr->skill_exp[PlayerSkillKindType::RIDING] / 50 + player_ptr->lev / 2 + 20))) {
            msg_print(_("うまく乗れなかった。", "You failed to ride."));
            PlayerEnergy(player_ptr).set_player_turn_energy(100);
            return false;
        }

        if (monster.is_asleep()) {
            const auto m_name = monster_desc(player_ptr, monster, 0);
            (void)set_monster_csleep(*player_ptr->current_floor_ptr, grid.m_idx, 0);
            msg_format(_("%sを起こした。", "You have woken %s up."), m_name.data());
        }

        if (player_ptr->action == ACTION_MONK_STANCE) {
            set_action(player_ptr, ACTION_NONE);
        }

        player_ptr->ride_monster(grid.m_idx);
        if (HealthBarTracker::get_instance().is_tracking(player_ptr->riding)) {
            health_track(player_ptr, 0);
        }
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::UN_VIEW,
        StatusRecalculatingFlag::UN_LITE,
        StatusRecalculatingFlag::BONUS,
    };
    rfu.set_flags(flags_srf);
    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::MAP,
        MainWindowRedrawingFlag::EXTRA,
        MainWindowRedrawingFlag::UHEALTH,
    };
    rfu.set_flags(flags_mwrf);
    (void)move_player_effect(player_ptr, pos.y, pos.x, MPE_HANDLE_STUFF | MPE_ENERGY_USE | MPE_DONT_PICKUP | MPE_DONT_SWAP_MON);
    return true;
}

/*!
 * @brief ペットに名前をつけるコマンドのメインルーチン
 */
static void do_name_pet(PlayerType *player_ptr)
{
    auto old_target_pet = target_pet;
    target_pet = true;
    const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
    if (!pos) {
        target_pet = old_target_pet;
        return;
    }

    target_pet = old_target_pet;
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.get_grid(*pos);
    if (!grid.has_monster()) {
        return;
    }

    auto &monster = floor.m_list[grid.m_idx];
    if (!monster.is_pet()) {
        msg_print(_("そのモンスターはペットではない。", "This monster is not a pet."));
        return;
    }

    if (monster.get_monrace().kind_flags.has(MonsterKindType::UNIQUE)) {
        msg_print(_("そのモンスターの名前は変えられない！", "You cannot change the name of this monster!"));
        return;
    }

    msg_format(_("%sに名前をつける。", "Name %s."), monster_desc(player_ptr, monster, 0).data());
    msg_erase();

    auto old_name = false;
    std::string initial_name("");
    if (monster.is_named()) {
        initial_name = monster.nickname;
        old_name = true;
    }

    const auto new_name = input_string(_("名前: ", "Name: "), 15, initial_name);
    if (!new_name) {
        return;
    }

    if (!new_name->empty()) {
        monster.nickname = *new_name;
        if (record_named_pet) {
            exe_write_diary(floor, DiaryKind::NAMED_PET, RECORD_NAMED_PET_NAME, monster_desc(player_ptr, monster, MD_INDEF_VISIBLE));
        }

        return;
    }

    if (record_named_pet && old_name) {
        exe_write_diary(floor, DiaryKind::NAMED_PET, RECORD_NAMED_PET_UNNAME, monster_desc(player_ptr, monster, MD_INDEF_VISIBLE));
    }

    monster.nickname.clear();
}

/*!
 * @brief ペットに関するコマンドリストのメインルーチン /
 * Issue a pet command
 */
void do_cmd_pet(PlayerType *player_ptr)
{
    int powers[36]{};
    std::string power_desc[36];
    bool flag, redraw;
    char choice;
    int pet_ctr;
    auto command_idx = 0;
    int menu_line = use_menu ? 1 : 0;
    auto num = 0;
    if (AngbandWorld::get_instance().is_wild_mode()) {
        return;
    }

    power_desc[num] = _("ペットを放す", "dismiss pets");
    powers[num++] = PET_DISMISS;

    const auto is_hallucinated = player_ptr->effects()->hallucination().is_hallucinated();
    const auto taget_of_pet = player_ptr->current_floor_ptr->m_list[player_ptr->pet_t_m_idx].get_appearance_monrace().name.data();
    const auto target_of_pet_appearance = is_hallucinated ? _("何か奇妙な物", "something strange") : taget_of_pet;
    const auto mes = _("ペットのターゲットを指定 (現在：%s)", "specify a target of pet (now:%s)");
    const auto target_name = player_ptr->pet_t_m_idx > 0 ? target_of_pet_appearance : _("指定なし", "nothing");
    const auto target_ask = format(mes, target_name);
    power_desc[num] = target_ask;
    powers[num++] = PET_TARGET;
    power_desc[num] = _("近くにいろ", "stay close");

    if (player_ptr->pet_follow_distance == PET_CLOSE_DIST) {
        command_idx = num;
    }
    powers[num++] = PET_STAY_CLOSE;
    power_desc[num] = _("ついて来い", "follow me");

    if (player_ptr->pet_follow_distance == PET_FOLLOW_DIST) {
        command_idx = num;
    }
    powers[num++] = PET_FOLLOW_ME;
    power_desc[num] = _("敵を見つけて倒せ", "seek and destroy");

    if (player_ptr->pet_follow_distance == PET_DESTROY_DIST) {
        command_idx = num;
    }
    powers[num++] = PET_SEEK_AND_DESTROY;
    power_desc[num] = _("少し離れていろ", "give me space");

    if (player_ptr->pet_follow_distance == PET_SPACE_DIST) {
        command_idx = num;
    }
    powers[num++] = PET_ALLOW_SPACE;
    power_desc[num] = _("離れていろ", "stay away");

    if (player_ptr->pet_follow_distance == PET_AWAY_DIST) {
        command_idx = num;
    }
    powers[num++] = PET_STAY_AWAY;

    if (player_ptr->pet_extra_flags & PF_OPEN_DOORS) {
        power_desc[num] = _("ドアを開ける (現在:ON)", "pets open doors (now On)");
    } else {
        power_desc[num] = _("ドアを開ける (現在:OFF)", "pets open doors (now Off)");
    }
    powers[num++] = PET_OPEN_DOORS;

    if (player_ptr->pet_extra_flags & PF_PICKUP_ITEMS) {
        power_desc[num] = _("アイテムを拾う (現在:ON)", "pets pick up items (now On)");
    } else {
        power_desc[num] = _("アイテムを拾う (現在:OFF)", "pets pick up items (now Off)");
    }
    powers[num++] = PET_TAKE_ITEMS;

    if (player_ptr->pet_extra_flags & PF_TELEPORT) {
        power_desc[num] = _("テレポート系魔法を使う (現在:ON)", "allow teleport (now On)");
    } else {
        power_desc[num] = _("テレポート系魔法を使う (現在:OFF)", "allow teleport (now Off)");
    }
    powers[num++] = PET_TELEPORT;

    if (player_ptr->pet_extra_flags & PF_ATTACK_SPELL) {
        power_desc[num] = _("攻撃魔法を使う (現在:ON)", "allow cast attack spell (now On)");
    } else {
        power_desc[num] = _("攻撃魔法を使う (現在:OFF)", "allow cast attack spell (now Off)");
    }
    powers[num++] = PET_ATTACK_SPELL;

    if (player_ptr->pet_extra_flags & PF_SUMMON_SPELL) {
        power_desc[num] = _("召喚魔法を使う (現在:ON)", "allow cast summon spell (now On)");
    } else {
        power_desc[num] = _("召喚魔法を使う (現在:OFF)", "allow cast summon spell (now Off)");
    }
    powers[num++] = PET_SUMMON_SPELL;

    if (player_ptr->pet_extra_flags & PF_BALL_SPELL) {
        power_desc[num] = _("プレイヤーを巻き込む範囲魔法を使う (現在:ON)", "allow involve player in area spell (now On)");
    } else {
        power_desc[num] = _("プレイヤーを巻き込む範囲魔法を使う (現在:OFF)", "allow involve player in area spell (now Off)");
    }
    powers[num++] = PET_BALL_SPELL;

    if (player_ptr->riding) {
        power_desc[num] = _("ペットから降りる", "get off a pet");
    } else {
        power_desc[num] = _("ペットに乗る", "ride a pet");
    }
    powers[num++] = PET_RIDING;
    power_desc[num] = _("ペットに名前をつける", "name pets");
    powers[num++] = PET_NAME;

    bool empty_main = can_attack_with_main_hand(player_ptr);
    empty_main &= empty_hands(player_ptr, false) == EMPTY_HAND_SUB;
    empty_main &= player_ptr->inventory[INVEN_MAIN_HAND]->allow_two_hands_wielding();

    bool empty_sub = can_attack_with_sub_hand(player_ptr);
    empty_sub &= empty_hands(player_ptr, false) == EMPTY_HAND_MAIN;
    empty_sub &= player_ptr->inventory[INVEN_SUB_HAND]->allow_two_hands_wielding();

    if (player_ptr->riding) {
        if (empty_main || empty_sub) {
            if (player_ptr->pet_extra_flags & PF_TWO_HANDS) {
                power_desc[num] = _("武器を片手で持つ", "use one hand to control the pet you are riding");
            } else {
                power_desc[num] = _("武器を両手で持つ", "use both hands for a weapon");
            }

            powers[num++] = PET_TWO_HANDS;
        } else {
            switch (player_ptr->pclass) {
            case PlayerClassType::MONK:
            case PlayerClassType::FORCETRAINER:
            case PlayerClassType::BERSERKER: {
                if (empty_hands(player_ptr, false) == (EMPTY_HAND_MAIN | EMPTY_HAND_SUB)) {
                    if (player_ptr->pet_extra_flags & PF_TWO_HANDS) {
                        power_desc[num] = _("片手で格闘する", "use one hand to control the pet you are riding");
                    } else {
                        power_desc[num] = _("両手で格闘する", "use both hands for melee");
                    }

                    powers[num++] = PET_TWO_HANDS;
                    break;
                }

                auto has_any_melee_weapon = has_melee_weapon(player_ptr, INVEN_MAIN_HAND);
                has_any_melee_weapon |= has_melee_weapon(player_ptr, INVEN_SUB_HAND);
                if ((empty_hands(player_ptr, false) != EMPTY_HAND_NONE) && !has_any_melee_weapon) {
                    if (player_ptr->pet_extra_flags & PF_TWO_HANDS) {
                        power_desc[num] = _("格闘を行わない", "use one hand to control the pet you are riding");
                    } else {
                        power_desc[num] = _("格闘を行う", "use one hand for melee");
                    }

                    powers[num++] = PET_TWO_HANDS;
                    break;
                }

                break;
            }
            default:
                break;
            }
        }
    }

    auto code_repeat = repeat_pull();
    if (!code_repeat || (code_repeat < 0) || (code_repeat >= num)) {
        flag = false;
        redraw = false;

        std::string prompt;
        if (use_menu) {
            screen_save();
            prompt = _("(コマンド、ESC=終了) コマンドを選んでください:", "(Command, ESC=exit) Choose command from menu.");
        } else {
            constexpr auto fmt = _("(コマンド %c-%c、'*'=一覧、ESC=終了) コマンドを選んでください:",
                "(Command %c-%c, *=List, ESC=exit) Select a command: ");
            prompt = format(fmt, I2A(0), I2A(num - 1));
        }

        choice = (always_show_list || use_menu) ? ESCAPE : '\1';

        /* Get a command from the user */
        while (!flag) {
            if (choice == ESCAPE) {
                choice = ' ';
            } else {
                const auto new_choice = input_command(prompt);
                if (!new_choice) {
                    break;
                }

                choice = *new_choice;
            }

            auto should_redraw_cursor = true;
            if (use_menu && (choice != ' ')) {
                switch (choice) {
                case '0':
                    screen_load();
                    return;

                case '8':
                case 'k':
                case 'K':
                    menu_line += (num - 1);
                    break;

                case '2':
                case 'j':
                case 'J':
                    menu_line++;
                    break;

                case '4':
                case 'h':
                case 'H':
                    menu_line = 1;
                    break;

                case '6':
                case 'l':
                case 'L':
                    menu_line = num;
                    break;

                case 'x':
                case 'X':
                case '\r':
                case '\n':
                    code_repeat = static_cast<short>(menu_line - 1);
                    should_redraw_cursor = false;
                    break;
                }
                if (menu_line > num) {
                    menu_line -= num;
                }
            }

            /* Request redraw */
            if ((choice == ' ') || (choice == '*') || (choice == '?') || (use_menu && should_redraw_cursor)) {
                /* Show the list */
                if (!redraw || use_menu) {
                    byte y = 1, x = 0;
                    redraw = true;
                    if (!use_menu) {
                        screen_save();
                    }

                    prt("", y++, x);

                    /* Print list */
                    int control;
                    for (control = 0; control < num; control++) {
                        /* Letter/number for power selection */
                        std::stringstream ss;
                        if (use_menu) {
                            ss << format("%c%s ", (control == command_idx) ? '*' : ' ', (control == (menu_line - 1)) ? _("》", "> ") : "  ");
                        } else {
                            ss << format("%c%c) ", (control == command_idx) ? '*' : ' ', I2A(control));
                        }

                        ss << power_desc[control];
                        prt(ss.str(), y + control, x);
                    }

                    prt("", y + std::min(control, 17), x);
                }

                /* Hide the list */
                else {
                    /* Hide list */
                    redraw = false;
                    screen_load();
                }

                /* Redo asking */
                continue;
            }

            if (!use_menu) {
                code_repeat = A2I(choice);
            }

            /* Totally Illegal */
            if (!code_repeat || (code_repeat < 0) || (code_repeat >= num)) {
                bell();
                continue;
            }

            /* Stop the loop */
            flag = true;
        }
        if (redraw) {
            screen_load();
        }

        /* Abort if needed */
        if (!flag) {
            PlayerEnergy(player_ptr).reset_player_turn();
            return;
        }

        repeat_push(*code_repeat);
    }

    switch (powers[code_repeat.value_or(0)]) {
    case PET_DISMISS: /* Dismiss pets */
    {
        /* Check pets (backwards) */
        for (pet_ctr = player_ptr->current_floor_ptr->m_max - 1; pet_ctr >= 1; pet_ctr--) {
            const auto &m_ref = player_ptr->current_floor_ptr->m_list[pet_ctr];
            if (m_ref.is_pet()) {
                break;
            }
        }

        if (!pet_ctr) {
            msg_print(_("ペットがいない！", "You have no pets!"));
            break;
        }
        do_cmd_pet_dismiss(player_ptr);
        (void)calculate_upkeep(player_ptr);
        break;
    }
    case PET_TARGET: {
        project_length = -1;
        const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
        if (!pos) {
            player_ptr->pet_t_m_idx = 0;
        } else {
            const auto &grid = player_ptr->current_floor_ptr->get_grid(*pos);
            if (grid.has_monster() && (player_ptr->current_floor_ptr->m_list[grid.m_idx].ml)) {
                player_ptr->pet_t_m_idx = player_ptr->current_floor_ptr->get_grid(*pos).m_idx;
                player_ptr->pet_follow_distance = PET_DESTROY_DIST;
            } else {
                player_ptr->pet_t_m_idx = 0;
            }
        }
        project_length = 0;

        break;
    }
    /* Call pets */
    case PET_STAY_CLOSE: {
        player_ptr->pet_follow_distance = PET_CLOSE_DIST;
        player_ptr->pet_t_m_idx = 0;
        break;
    }
    /* "Follow Me" */
    case PET_FOLLOW_ME: {
        player_ptr->pet_follow_distance = PET_FOLLOW_DIST;
        player_ptr->pet_t_m_idx = 0;
        break;
    }
    /* "Seek and destoy" */
    case PET_SEEK_AND_DESTROY: {
        player_ptr->pet_follow_distance = PET_DESTROY_DIST;
        break;
    }
    /* "Give me space" */
    case PET_ALLOW_SPACE: {
        player_ptr->pet_follow_distance = PET_SPACE_DIST;
        break;
    }
    /* "Stay away" */
    case PET_STAY_AWAY: {
        player_ptr->pet_follow_distance = PET_AWAY_DIST;
        break;
    }
    /* flag - allow pets to open doors */
    case PET_OPEN_DOORS: {
        if (player_ptr->pet_extra_flags & PF_OPEN_DOORS) {
            player_ptr->pet_extra_flags &= ~(PF_OPEN_DOORS);
        } else {
            player_ptr->pet_extra_flags |= (PF_OPEN_DOORS);
        }
        break;
    }
    /* flag - allow pets to pickup items */
    case PET_TAKE_ITEMS: {
        if (player_ptr->pet_extra_flags & PF_PICKUP_ITEMS) {
            player_ptr->pet_extra_flags &= ~(PF_PICKUP_ITEMS);
            for (pet_ctr = player_ptr->current_floor_ptr->m_max - 1; pet_ctr >= 1; pet_ctr--) {
                auto &monster = player_ptr->current_floor_ptr->m_list[pet_ctr];
                if (monster.is_pet()) {
                    monster_drop_carried_objects(player_ptr, monster);
                }
            }
        } else {
            player_ptr->pet_extra_flags |= (PF_PICKUP_ITEMS);
        }

        break;
    }
    /* flag - allow pets to teleport */
    case PET_TELEPORT: {
        if (player_ptr->pet_extra_flags & PF_TELEPORT) {
            player_ptr->pet_extra_flags &= ~(PF_TELEPORT);
        } else {
            player_ptr->pet_extra_flags |= (PF_TELEPORT);
        }
        break;
    }
    /* flag - allow pets to cast attack spell */
    case PET_ATTACK_SPELL: {
        if (player_ptr->pet_extra_flags & PF_ATTACK_SPELL) {
            player_ptr->pet_extra_flags &= ~(PF_ATTACK_SPELL);
        } else {
            player_ptr->pet_extra_flags |= (PF_ATTACK_SPELL);
        }
        break;
    }
    /* flag - allow pets to cast attack spell */
    case PET_SUMMON_SPELL: {
        if (player_ptr->pet_extra_flags & PF_SUMMON_SPELL) {
            player_ptr->pet_extra_flags &= ~(PF_SUMMON_SPELL);
        } else {
            player_ptr->pet_extra_flags |= (PF_SUMMON_SPELL);
        }
        break;
    }
    /* flag - allow pets to cast attack spell */
    case PET_BALL_SPELL: {
        if (player_ptr->pet_extra_flags & PF_BALL_SPELL) {
            player_ptr->pet_extra_flags &= ~(PF_BALL_SPELL);
        } else {
            player_ptr->pet_extra_flags |= (PF_BALL_SPELL);
        }
        break;
    }

    case PET_RIDING: {
        (void)do_cmd_riding(player_ptr, false);
        break;
    }

    case PET_NAME: {
        do_name_pet(player_ptr);
        break;
    }

    case PET_TWO_HANDS: {
        if (player_ptr->pet_extra_flags & PF_TWO_HANDS) {
            player_ptr->pet_extra_flags &= ~(PF_TWO_HANDS);
        } else {
            player_ptr->pet_extra_flags |= (PF_TWO_HANDS);
        }

        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
        handle_stuff(player_ptr);
        break;
    }
    }
}
