#include "cmd-action/cmd-pet.h"
#include "action/action-limited.h"
#include "cmd-action/cmd-attack.h"
#include "cmd-io/cmd-dump.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
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
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
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
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "term/screen-processor.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "util/quarks.h"
#include "util/sort.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <sstream>

/*!
 * @brief ペットを開放するコマンドのメインルーチン
 */
void do_cmd_pet_dismiss(PlayerType *player_ptr)
{
    MonsterEntity *m_ptr;
    bool all_pets = false;
    int Dismissed = 0;

    uint16_t dummy_why;
    bool cu, cv;

    cu = game_term->scr->cu;
    cv = game_term->scr->cv;
    game_term->scr->cu = 0;
    game_term->scr->cv = 1;

    /* Allocate the "who" array */
    std::vector<MONSTER_IDX> who;

    /* Process the monsters (backwards) */
    for (MONSTER_IDX pet_ctr = player_ptr->current_floor_ptr->m_max - 1; pet_ctr >= 1; pet_ctr--) {
        const auto &m_ref = player_ptr->current_floor_ptr->m_list[pet_ctr];
        if (m_ref.is_pet()) {
            who.push_back(pet_ctr);
        }
    }

    ang_sort(player_ptr, who.data(), &dummy_why, who.size(), ang_sort_comp_pet_dismiss, ang_sort_swap_hook);

    /* Process the monsters (backwards) */
    for (auto i = 0U; i < who.size(); i++) {
        auto pet_ctr = who[i];
        m_ptr = &player_ptr->current_floor_ptr->m_list[pet_ctr];

        auto delete_this = false;
        auto should_ask = (pet_ctr == player_ptr->riding) || m_ptr->is_named();
        const auto friend_name = monster_desc(player_ptr, m_ptr, MD_ASSUME_VISIBLE);

        if (!all_pets) {
            /* Hack -- health bar for this monster */
            health_track(player_ptr, pet_ctr);
            handle_stuff(player_ptr);

            msg_format(_("%sを放しますか？ [Yes/No/Unnamed (%lu体)]", "Dismiss %s? [Yes/No/Unnamed (%lu remain)]"), friend_name.data(), who.size() - i);

            if (m_ptr->ml) {
                move_cursor_relative(m_ptr->fy, m_ptr->fx);
            }

            while (true) {
                char ch = inkey();

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
            if (record_named_pet && m_ptr->is_named()) {
                const auto m_name = monster_desc(player_ptr, m_ptr, MD_INDEF_VISIBLE);
                exe_write_diary(player_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_DISMISS, m_name.data());
            }

            if (pet_ctr == player_ptr->riding) {
                msg_format(_("%sから降りた。", "You dismount from %s. "), friend_name.data());

                player_ptr->riding = 0;

                player_ptr->update |= (PU_MONSTERS);
                player_ptr->redraw |= (PR_EXTRA | PR_UHEALTH);
            }

            /* HACK : Add the line to message buffer */
            msg_format(_("%s を放した。", "Dismissed %s."), friend_name.data());
            player_ptr->update |= (PU_BONUS);
            player_ptr->window_flags |= (PW_MESSAGE);

            delete_monster_idx(player_ptr, pet_ctr);
            Dismissed++;
        }
    }

    game_term->scr->cu = cu;
    game_term->scr->cv = cv;
    term_fresh();

#ifdef JP
    msg_format("%d 体のペットを放しました。", Dismissed);
#else
    msg_format("You have dismissed %d pet%s.", Dismissed, (Dismissed == 1 ? "" : "s"));
#endif
    if (Dismissed == 0 && all_pets) {
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
    POSITION x, y;
    DIRECTION dir = 0;
    grid_type *g_ptr;
    MonsterEntity *m_ptr;

    if (!get_direction(player_ptr, &dir, false, false)) {
        return false;
    }
    y = player_ptr->y + ddy[dir];
    x = player_ptr->x + ddx[dir];
    g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];

    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });

    if (player_ptr->riding) {
        /* Skip non-empty grids */
        if (!can_player_ride_pet(player_ptr, g_ptr, false)) {
            msg_print(_("そちらには降りられません。", "You cannot go that direction."));
            return false;
        }

        if (!pattern_seq(player_ptr, player_ptr->y, player_ptr->x, y, x)) {
            return false;
        }

        if (g_ptr->m_idx) {
            PlayerEnergy(player_ptr).set_player_turn_energy(100);

            msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));

            do_cmd_attack(player_ptr, y, x, HISSATSU_NONE);
            return false;
        }

        player_ptr->riding = 0;
        player_ptr->pet_extra_flags &= ~(PF_TWO_HANDS);
        player_ptr->riding_ryoute = player_ptr->old_riding_ryoute = false;
    } else {
        if (cmd_limit_confused(player_ptr)) {
            return false;
        }

        m_ptr = &player_ptr->current_floor_ptr->m_list[g_ptr->m_idx];

        if (!g_ptr->m_idx || !m_ptr->ml) {
            msg_print(_("その場所にはモンスターはいません。", "There is no monster here."));
            return false;
        }
        if (!m_ptr->is_pet() && !force) {
            msg_print(_("そのモンスターはペットではありません。", "That monster is not a pet."));
            return false;
        }
        if (!(monraces_info[m_ptr->r_idx].flags7 & RF7_RIDING)) {
            msg_print(_("そのモンスターには乗れなさそうだ。", "This monster doesn't seem suitable for riding."));
            return false;
        }

        if (!pattern_seq(player_ptr, player_ptr->y, player_ptr->x, y, x)) {
            return false;
        }

        if (!can_player_ride_pet(player_ptr, g_ptr, true)) {
            /* Feature code (applying "mimic" field) */
            auto *f_ptr = &terrains_info[g_ptr->get_feat_mimic()];
#ifdef JP
            msg_format("そのモンスターは%sの%sにいる。", f_ptr->name.data(),
                (f_ptr->flags.has_none_of({ TerrainCharacteristics::MOVE, TerrainCharacteristics::CAN_FLY }) || f_ptr->flags.has_none_of({ TerrainCharacteristics::LOS, TerrainCharacteristics::TREE })) ? "中" : "上");
#else
            msg_format("This monster is %s the %s.",
                (f_ptr->flags.has_none_of({ TerrainCharacteristics::MOVE, TerrainCharacteristics::CAN_FLY }) || f_ptr->flags.has_none_of({ TerrainCharacteristics::LOS, TerrainCharacteristics::TREE })) ? "in" : "on", f_ptr->name.data());
#endif

            return false;
        }
        if (monraces_info[m_ptr->r_idx].level > randint1((player_ptr->skill_exp[PlayerSkillKindType::RIDING] / 50 + player_ptr->lev / 2 + 20))) {
            msg_print(_("うまく乗れなかった。", "You failed to ride."));
            PlayerEnergy(player_ptr).set_player_turn_energy(100);
            return false;
        }

        if (m_ptr->is_asleep()) {
            const auto m_name = monster_desc(player_ptr, m_ptr, 0);
            (void)set_monster_csleep(player_ptr, g_ptr->m_idx, 0);
            msg_format(_("%sを起こした。", "You have woken %s up."), m_name.data());
        }

        if (player_ptr->action == ACTION_MONK_STANCE) {
            set_action(player_ptr, ACTION_NONE);
        }

        player_ptr->riding = g_ptr->m_idx;

        /* Hack -- remove tracked monster */
        if (player_ptr->riding == player_ptr->health_who) {
            health_track(player_ptr, 0);
        }
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);

    /* Mega-Hack -- Forget the view and lite */
    player_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);
    player_ptr->update |= (PU_BONUS);
    player_ptr->redraw |= (PR_MAP | PR_EXTRA);
    player_ptr->redraw |= (PR_UHEALTH);

    (void)move_player_effect(player_ptr, y, x, MPE_HANDLE_STUFF | MPE_ENERGY_USE | MPE_DONT_PICKUP | MPE_DONT_SWAP_MON);

    return true;
}

/*!
 * @brief ペットに名前をつけるコマンドのメインルーチン
 */
static void do_name_pet(PlayerType *player_ptr)
{
    MonsterEntity *m_ptr;
    bool old_name = false;
    bool old_target_pet = target_pet;

    target_pet = true;
    if (!target_set(player_ptr, TARGET_KILL)) {
        target_pet = old_target_pet;
        return;
    }

    target_pet = old_target_pet;

    if (player_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx) {
        m_ptr = &player_ptr->current_floor_ptr->m_list[player_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx];

        if (!m_ptr->is_pet()) {
            msg_print(_("そのモンスターはペットではない。", "This monster is not a pet."));
            return;
        }
        if (monraces_info[m_ptr->r_idx].kind_flags.has(MonsterKindType::UNIQUE)) {
            msg_print(_("そのモンスターの名前は変えられない！", "You cannot change the name of this monster!"));
            return;
        }

        msg_format(_("%sに名前をつける。", "Name %s."), monster_desc(player_ptr, m_ptr, 0).data());
        msg_print(nullptr);

        /* Start with nothing */
        char out_val[20]{};

        /* Use old inscription */
        if (m_ptr->is_named()) {
            /* Start with the old inscription */
            angband_strcpy(out_val, m_ptr->nickname.data(), sizeof(out_val));
            old_name = true;
        }

        /* Get a new inscription (possibly empty) */
        if (get_string(_("名前: ", "Name: "), out_val, 15)) {
            if (out_val[0]) {
                /* Save the inscription */
                m_ptr->nickname = out_val;
                if (record_named_pet) {
                    exe_write_diary(player_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_NAME, monster_desc(player_ptr, m_ptr, MD_INDEF_VISIBLE).data());
                }
            } else {
                if (record_named_pet && old_name) {
                    exe_write_diary(player_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_UNNAME, monster_desc(player_ptr, m_ptr, MD_INDEF_VISIBLE).data());
                }
                m_ptr->nickname.clear();
            }
        }
    }
}

/*!
 * @brief ペットに関するコマンドリストのメインルーチン /
 * Issue a pet command
 */
void do_cmd_pet(PlayerType *player_ptr)
{
    COMMAND_CODE i = 0;
    int powers[36]{};
    std::string power_desc[36];
    bool flag, redraw;
    char choice;
    int pet_ctr;
    MonsterEntity *m_ptr;
    auto command_idx = 0;
    int menu_line = use_menu ? 1 : 0;
    auto num = 0;
    if (player_ptr->wild_mode) {
        return;
    }

    power_desc[num] = _("ペットを放す", "dismiss pets");
    powers[num++] = PET_DISMISS;

    auto is_hallucinated = player_ptr->effects()->hallucination()->is_hallucinated();
    auto taget_of_pet = monraces_info[player_ptr->current_floor_ptr->m_list[player_ptr->pet_t_m_idx].ap_r_idx].name.data();
    auto target_of_pet_appearance = is_hallucinated ? _("何か奇妙な物", "something strange") : taget_of_pet;
    auto mes = _("ペットのターゲットを指定 (現在：%s)", "specify a target of pet (now:%s)");
    auto target_name = player_ptr->pet_t_m_idx > 0 ? target_of_pet_appearance : _("指定なし", "nothing");
    auto target_ask = format(mes, target_name);
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
    empty_main &= player_ptr->inventory_list[INVEN_MAIN_HAND].allow_two_hands_wielding();

    bool empty_sub = can_attack_with_sub_hand(player_ptr);
    empty_sub &= empty_hands(player_ptr, false) == EMPTY_HAND_MAIN;
    empty_sub &= player_ptr->inventory_list[INVEN_SUB_HAND].allow_two_hands_wielding();

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
            case PlayerClassType::BERSERKER:
                if (empty_hands(player_ptr, false) == (EMPTY_HAND_MAIN | EMPTY_HAND_SUB)) {
                    if (player_ptr->pet_extra_flags & PF_TWO_HANDS) {
                        power_desc[num] = _("片手で格闘する", "use one hand to control the pet you are riding");
                    } else {
                        power_desc[num] = _("両手で格闘する", "use both hands for melee");
                    }

                    powers[num++] = PET_TWO_HANDS;
                } else if ((empty_hands(player_ptr, false) != EMPTY_HAND_NONE) && !has_melee_weapon(player_ptr, INVEN_MAIN_HAND) && !has_melee_weapon(player_ptr, INVEN_SUB_HAND)) {
                    if (player_ptr->pet_extra_flags & PF_TWO_HANDS) {
                        power_desc[num] = _("格闘を行わない", "use one hand to control the pet you are riding");
                    } else {
                        power_desc[num] = _("格闘を行う", "use one hand for melee");
                    }

                    powers[num++] = PET_TWO_HANDS;
                }
                break;

            default:
                break;
            }
        }
    }

    if (!(repeat_pull(&i) && (i >= 0) && (i < num))) {
        flag = false;
        redraw = false;

        std::string prompt;
        if (use_menu) {
            screen_save();
            prompt = _("(コマンド、ESC=終了) コマンドを選んでください:", "(Command, ESC=exit) Choose command from menu.");
        } else {
            prompt = format(_("(コマンド %c-%c、'*'=一覧、ESC=終了) コマンドを選んでください:", "(Command %c-%c, *=List, ESC=exit) Select a command: "),
                I2A(0), I2A(num - 1));
        }

        choice = (always_show_list || use_menu) ? ESCAPE : 1;

        /* Get a command from the user */
        while (!flag) {
            if (choice == ESCAPE) {
                choice = ' ';
            } else if (!get_com(prompt.data(), &choice, true)) {
                break;
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
                    i = menu_line - 1;
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
                        prt(ss.str().data(), y + control, x);
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
                i = A2I(choice);
            }

            /* Totally Illegal */
            if ((i < 0) || (i >= num)) {
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

        repeat_push(i);
    }
    switch (powers[i]) {
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
        if (!target_set(player_ptr, TARGET_KILL)) {
            player_ptr->pet_t_m_idx = 0;
        } else {
            auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[target_row][target_col];
            if (g_ptr->m_idx && (player_ptr->current_floor_ptr->m_list[g_ptr->m_idx].ml)) {
                player_ptr->pet_t_m_idx = player_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx;
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
                m_ptr = &player_ptr->current_floor_ptr->m_list[pet_ctr];
                if (m_ptr->is_pet()) {
                    monster_drop_carried_objects(player_ptr, m_ptr);
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
        player_ptr->update |= (PU_BONUS);
        handle_stuff(player_ptr);
        break;
    }
    }
}
