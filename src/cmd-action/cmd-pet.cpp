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
#include "grid/feature.h"
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
#include "player-info/equipment-info.h"
#include "player-status/player-energy.h"
#include "player-status/player-hand-types.h"
#include "player/attack-defense-types.h"
#include "player/player-class.h"
#include "player/player-damage.h"
#include "player/player-move.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "status/action-setter.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "util/quarks.h"
#include "util/sort.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief ペットを開放するコマンドのメインルーチン
 */
void do_cmd_pet_dismiss(player_type *creature_ptr)
{
    monster_type *m_ptr;
    bool all_pets = false;
    MONSTER_IDX pet_ctr;
    int i;
    int Dismissed = 0;

    MONSTER_IDX *who;
    uint16_t dummy_why;
    int max_pet = 0;
    bool cu, cv;

    cu = Term->scr->cu;
    cv = Term->scr->cv;
    Term->scr->cu = 0;
    Term->scr->cv = 1;

    /* Allocate the "who" array */
    C_MAKE(who, current_world_ptr->max_m_idx, MONSTER_IDX);

    /* Process the monsters (backwards) */
    for (pet_ctr = creature_ptr->current_floor_ptr->m_max - 1; pet_ctr >= 1; pet_ctr--) {
        if (is_pet(&creature_ptr->current_floor_ptr->m_list[pet_ctr]))
            who[max_pet++] = pet_ctr;
    }

    ang_sort(creature_ptr, who, &dummy_why, max_pet, ang_sort_comp_pet_dismiss, ang_sort_swap_hook);

    /* Process the monsters (backwards) */
    for (i = 0; i < max_pet; i++) {
        bool delete_this;
        GAME_TEXT friend_name[MAX_NLEN];
        bool kakunin;

        pet_ctr = who[i];
        m_ptr = &creature_ptr->current_floor_ptr->m_list[pet_ctr];

        delete_this = false;
        kakunin = ((pet_ctr == creature_ptr->riding) || (m_ptr->nickname));
        monster_desc(creature_ptr, friend_name, m_ptr, MD_ASSUME_VISIBLE);

        if (!all_pets) {
            /* Hack -- health bar for this monster */
            health_track(creature_ptr, pet_ctr);
            handle_stuff(creature_ptr);

            msg_format(_("%sを放しますか？ [Yes/No/Unnamed (%d体)]", "Dismiss %s? [Yes/No/Unnamed (%d remain)]"), friend_name, max_pet - i);

            if (m_ptr->ml)
                move_cursor_relative(m_ptr->fy, m_ptr->fx);

            while (true) {
                char ch = inkey();

                if (ch == 'Y' || ch == 'y') {
                    delete_this = true;

                    if (kakunin) {
                        msg_format(_("本当によろしいですか？ (%s) ", "Are you sure? (%s) "), friend_name);
                        ch = inkey();
                        if (ch != 'Y' && ch != 'y')
                            delete_this = false;
                    }
                    break;
                }

                if (ch == 'U' || ch == 'u') {
                    all_pets = true;
                    break;
                }

                if (ch == ESCAPE || ch == 'N' || ch == 'n')
                    break;

                bell();
            }
        }

        if ((all_pets && !kakunin) || (!all_pets && delete_this)) {
            if (record_named_pet && m_ptr->nickname) {
                GAME_TEXT m_name[MAX_NLEN];

                monster_desc(creature_ptr, m_name, m_ptr, MD_INDEF_VISIBLE);
                exe_write_diary(creature_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_DISMISS, m_name);
            }

            if (pet_ctr == creature_ptr->riding) {
                msg_format(_("%sから降りた。", "You dismount from %s. "), friend_name);

                creature_ptr->riding = 0;

                creature_ptr->update |= (PU_MONSTERS);
                creature_ptr->redraw |= (PR_EXTRA | PR_UHEALTH);
            }

            /* HACK : Add the line to message buffer */
            msg_format(_("%s を放した。", "Dismissed %s."), friend_name);
            creature_ptr->update |= (PU_BONUS);
            creature_ptr->window_flags |= (PW_MESSAGE);

            delete_monster_idx(creature_ptr, pet_ctr);
            Dismissed++;
        }
    }

    Term->scr->cu = cu;
    Term->scr->cv = cv;
    term_fresh();

    C_KILL(who, current_world_ptr->max_m_idx, MONSTER_IDX);

#ifdef JP
    msg_format("%d 体のペットを放しました。", Dismissed);
#else
    msg_format("You have dismissed %d pet%s.", Dismissed, (Dismissed == 1 ? "" : "s"));
#endif
    if (Dismissed == 0 && all_pets)
        msg_print(_("'U'nnamed は、乗馬以外の名前のないペットだけを全て解放します。", "'U'nnamed means all your pets except named pets and your mount."));

    handle_stuff(creature_ptr);
}

/*!
 * @brief ペットから騎乗/下馬するコマンドのメインルーチン /
 * @param force 強制的に騎乗/下馬するならばTRUE
 * @return 騎乗/下馬できたらTRUE
 */
bool do_cmd_riding(player_type *creature_ptr, bool force)
{
    POSITION x, y;
    DIRECTION dir = 0;
    grid_type *g_ptr;
    monster_type *m_ptr;

    if (!get_direction(creature_ptr, &dir, false, false))
        return false;
    y = creature_ptr->y + ddy[dir];
    x = creature_ptr->x + ddx[dir];
    g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];

    if (creature_ptr->special_defense & KATA_MUSOU)
        set_action(creature_ptr, ACTION_NONE);

    if (creature_ptr->riding) {
        /* Skip non-empty grids */
        if (!can_player_ride_pet(creature_ptr, g_ptr, false)) {
            msg_print(_("そちらには降りられません。", "You cannot go that direction."));
            return false;
        }

        if (!pattern_seq(creature_ptr, creature_ptr->y, creature_ptr->x, y, x))
            return false;

        if (g_ptr->m_idx) {
            PlayerEnergy(creature_ptr).set_player_turn_energy(100);

            msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));

            do_cmd_attack(creature_ptr, y, x, HISSATSU_NONE);
            return false;
        }

        creature_ptr->riding = 0;
        creature_ptr->pet_extra_flags &= ~(PF_TWO_HANDS);
        creature_ptr->riding_ryoute = creature_ptr->old_riding_ryoute = false;
    } else {
        if (cmd_limit_confused(creature_ptr))
            return false;

        m_ptr = &creature_ptr->current_floor_ptr->m_list[g_ptr->m_idx];

        if (!g_ptr->m_idx || !m_ptr->ml) {
            msg_print(_("その場所にはモンスターはいません。", "There is no monster here."));
            return false;
        }
        if (!is_pet(m_ptr) && !force) {
            msg_print(_("そのモンスターはペットではありません。", "That monster is not a pet."));
            return false;
        }
        if (!(r_info[m_ptr->r_idx].flags7 & RF7_RIDING)) {
            msg_print(_("そのモンスターには乗れなさそうだ。", "This monster doesn't seem suitable for riding."));
            return false;
        }

        if (!pattern_seq(creature_ptr, creature_ptr->y, creature_ptr->x, y, x))
            return false;

        if (!can_player_ride_pet(creature_ptr, g_ptr, true)) {
            /* Feature code (applying "mimic" field) */
            feature_type *f_ptr = &f_info[g_ptr->get_feat_mimic()];
#ifdef JP
            msg_format("そのモンスターは%sの%sにいる。", f_ptr->name.c_str(),
                (f_ptr->flags.has_none_of({FF::MOVE, FF::CAN_FLY})
                    || f_ptr->flags.has_none_of({FF::LOS, FF::TREE}))
                    ? "中"
                    : "上");
#else
            msg_format("This monster is %s the %s.",
                (f_ptr->flags.has_none_of({FF::MOVE, FF::CAN_FLY})
                    || f_ptr->flags.has_none_of({FF::LOS, FF::TREE}))
                    ? "in"
                    : "on",
                f_ptr->name.c_str());
#endif

            return false;
        }
        if (r_info[m_ptr->r_idx].level > randint1((creature_ptr->skill_exp[SKILL_RIDING] / 50 + creature_ptr->lev / 2 + 20))) {
            msg_print(_("うまく乗れなかった。", "You failed to ride."));
            PlayerEnergy(creature_ptr).set_player_turn_energy(100);
            return false;
        }

        if (monster_csleep_remaining(m_ptr)) {
            GAME_TEXT m_name[MAX_NLEN];
            monster_desc(creature_ptr, m_name, m_ptr, 0);
            (void)set_monster_csleep(creature_ptr, g_ptr->m_idx, 0);
            msg_format(_("%sを起こした。", "You have woken %s up."), m_name);
        }

        if (creature_ptr->action == ACTION_KAMAE)
            set_action(creature_ptr, ACTION_NONE);

        creature_ptr->riding = g_ptr->m_idx;

        /* Hack -- remove tracked monster */
        if (creature_ptr->riding == creature_ptr->health_who)
            health_track(creature_ptr, 0);
    }

    PlayerEnergy(creature_ptr).set_player_turn_energy(100);

    /* Mega-Hack -- Forget the view and lite */
    creature_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);
    creature_ptr->update |= (PU_BONUS);
    creature_ptr->redraw |= (PR_MAP | PR_EXTRA);
    creature_ptr->redraw |= (PR_UHEALTH);

    (void)move_player_effect(creature_ptr, y, x, MPE_HANDLE_STUFF | MPE_ENERGY_USE | MPE_DONT_PICKUP | MPE_DONT_SWAP_MON);

    return true;
}

/*!
 * @brief ペットに名前をつけるコマンドのメインルーチン
 */
static void do_name_pet(player_type *creature_ptr)
{
    monster_type *m_ptr;
    char out_val[20];
    GAME_TEXT m_name[MAX_NLEN];
    bool old_name = false;
    bool old_target_pet = target_pet;

    target_pet = true;
    if (!target_set(creature_ptr, TARGET_KILL)) {
        target_pet = old_target_pet;
        return;
    }

    target_pet = old_target_pet;

    if (creature_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx) {
        m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx];

        if (!is_pet(m_ptr)) {
            msg_print(_("そのモンスターはペットではない。", "This monster is not a pet."));
            return;
        }
        if (r_info[m_ptr->r_idx].flags1 & RF1_UNIQUE) {
            msg_print(_("そのモンスターの名前は変えられない！", "You cannot change the name of this monster!"));
            return;
        }
        monster_desc(creature_ptr, m_name, m_ptr, 0);

        msg_format(_("%sに名前をつける。", "Name %s."), m_name);
        msg_print(NULL);

        /* Start with nothing */
        strcpy(out_val, "");

        /* Use old inscription */
        if (m_ptr->nickname) {
            /* Start with the old inscription */
            strcpy(out_val, quark_str(m_ptr->nickname));
            old_name = true;
        }

        /* Get a new inscription (possibly empty) */
        if (get_string(_("名前: ", "Name: "), out_val, 15)) {
            if (out_val[0]) {
                /* Save the inscription */
                m_ptr->nickname = quark_add(out_val);
                if (record_named_pet) {
                    monster_desc(creature_ptr, m_name, m_ptr, MD_INDEF_VISIBLE);
                    exe_write_diary(creature_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_NAME, m_name);
                }
            } else {
                if (record_named_pet && old_name) {
                    monster_desc(creature_ptr, m_name, m_ptr, MD_INDEF_VISIBLE);
                    exe_write_diary(creature_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_UNNAME, m_name);
                }
                m_ptr->nickname = 0;
            }
        }
    }
}

/*!
 * @brief ペットに関するコマンドリストのメインルーチン /
 * Issue a pet command
 */
void do_cmd_pet(player_type *creature_ptr)
{
    COMMAND_CODE i = 0;
    int num;
    int powers[36];
    concptr power_desc[36];
    bool flag, redraw;
    char choice;
    char out_val[160];
    int pet_ctr;
    monster_type *m_ptr;

    auto command_idx = 0;

    char buf[160];
    char target_buf[160];

    int menu_line = use_menu ? 1 : 0;

    num = 0;

    if (creature_ptr->wild_mode)
        return;

    power_desc[num] = _("ペットを放す", "dismiss pets");
    powers[num++] = PET_DISMISS;

#ifdef JP
    sprintf(target_buf, "ペットのターゲットを指定 (現在：%s)",
        (creature_ptr->pet_t_m_idx
                ? (creature_ptr->image ? "何か奇妙な物" : r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->pet_t_m_idx].ap_r_idx].name.c_str())
                : "指定なし"));
#else
    sprintf(target_buf, "specify a target of pet (now:%s)",
        (creature_ptr->pet_t_m_idx
                ? (creature_ptr->image ? "something strange" : r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->pet_t_m_idx].ap_r_idx].name.c_str())
                : "nothing"));
#endif
    power_desc[num] = target_buf;
    powers[num++] = PET_TARGET;
    power_desc[num] = _("近くにいろ", "stay close");

    if (creature_ptr->pet_follow_distance == PET_CLOSE_DIST)
        command_idx = num;
    powers[num++] = PET_STAY_CLOSE;
    power_desc[num] = _("ついて来い", "follow me");

    if (creature_ptr->pet_follow_distance == PET_FOLLOW_DIST)
        command_idx = num;
    powers[num++] = PET_FOLLOW_ME;
    power_desc[num] = _("敵を見つけて倒せ", "seek and destroy");

    if (creature_ptr->pet_follow_distance == PET_DESTROY_DIST)
        command_idx = num;
    powers[num++] = PET_SEEK_AND_DESTROY;
    power_desc[num] = _("少し離れていろ", "give me space");

    if (creature_ptr->pet_follow_distance == PET_SPACE_DIST)
        command_idx = num;
    powers[num++] = PET_ALLOW_SPACE;
    power_desc[num] = _("離れていろ", "stay away");

    if (creature_ptr->pet_follow_distance == PET_AWAY_DIST)
        command_idx = num;
    powers[num++] = PET_STAY_AWAY;

    if (creature_ptr->pet_extra_flags & PF_OPEN_DOORS) {
        power_desc[num] = _("ドアを開ける (現在:ON)", "pets open doors (now On)");
    } else {
        power_desc[num] = _("ドアを開ける (現在:OFF)", "pets open doors (now Off)");
    }
    powers[num++] = PET_OPEN_DOORS;

    if (creature_ptr->pet_extra_flags & PF_PICKUP_ITEMS) {
        power_desc[num] = _("アイテムを拾う (現在:ON)", "pets pick up items (now On)");
    } else {
        power_desc[num] = _("アイテムを拾う (現在:OFF)", "pets pick up items (now Off)");
    }
    powers[num++] = PET_TAKE_ITEMS;

    if (creature_ptr->pet_extra_flags & PF_TELEPORT) {
        power_desc[num] = _("テレポート系魔法を使う (現在:ON)", "allow teleport (now On)");
    } else {
        power_desc[num] = _("テレポート系魔法を使う (現在:OFF)", "allow teleport (now Off)");
    }
    powers[num++] = PET_TELEPORT;

    if (creature_ptr->pet_extra_flags & PF_ATTACK_SPELL) {
        power_desc[num] = _("攻撃魔法を使う (現在:ON)", "allow cast attack spell (now On)");
    } else {
        power_desc[num] = _("攻撃魔法を使う (現在:OFF)", "allow cast attack spell (now Off)");
    }
    powers[num++] = PET_ATTACK_SPELL;

    if (creature_ptr->pet_extra_flags & PF_SUMMON_SPELL) {
        power_desc[num] = _("召喚魔法を使う (現在:ON)", "allow cast summon spell (now On)");
    } else {
        power_desc[num] = _("召喚魔法を使う (現在:OFF)", "allow cast summon spell (now Off)");
    }
    powers[num++] = PET_SUMMON_SPELL;

    if (creature_ptr->pet_extra_flags & PF_BALL_SPELL) {
        power_desc[num] = _("プレイヤーを巻き込む範囲魔法を使う (現在:ON)", "allow involve player in area spell (now On)");
    } else {
        power_desc[num] = _("プレイヤーを巻き込む範囲魔法を使う (現在:OFF)", "allow involve player in area spell (now Off)");
    }
    powers[num++] = PET_BALL_SPELL;

    if (creature_ptr->riding) {
        power_desc[num] = _("ペットから降りる", "get off a pet");
    } else {
        power_desc[num] = _("ペットに乗る", "ride a pet");
    }
    powers[num++] = PET_RIDING;
    power_desc[num] = _("ペットに名前をつける", "name pets");
    powers[num++] = PET_NAME;

    if (creature_ptr->riding) {
        if ((can_attack_with_main_hand(creature_ptr) && (empty_hands(creature_ptr, false) == EMPTY_HAND_SUB)
                && object_allow_two_hands_wielding(&creature_ptr->inventory_list[INVEN_MAIN_HAND]))
            || (can_attack_with_sub_hand(creature_ptr) && (empty_hands(creature_ptr, false) == EMPTY_HAND_MAIN)
                && object_allow_two_hands_wielding(&creature_ptr->inventory_list[INVEN_SUB_HAND]))) {
            if (creature_ptr->pet_extra_flags & PF_TWO_HANDS) {
                power_desc[num] = _("武器を片手で持つ", "use one hand to control the pet you are riding");
            } else {
                power_desc[num] = _("武器を両手で持つ", "use both hands for a weapon");
            }

            powers[num++] = PET_TWO_HANDS;
        } else {
            switch (creature_ptr->pclass) {
            case CLASS_MONK:
            case CLASS_FORCETRAINER:
            case CLASS_BERSERKER:
                if (empty_hands(creature_ptr, false) == (EMPTY_HAND_MAIN | EMPTY_HAND_SUB)) {
                    if (creature_ptr->pet_extra_flags & PF_TWO_HANDS) {
                        power_desc[num] = _("片手で格闘する", "use one hand to control the pet you are riding");
                    } else {
                        power_desc[num] = _("両手で格闘する", "use both hands for melee");
                    }

                    powers[num++] = PET_TWO_HANDS;
                } else if ((empty_hands(creature_ptr, false) != EMPTY_HAND_NONE) && !has_melee_weapon(creature_ptr, INVEN_MAIN_HAND)
                    && !has_melee_weapon(creature_ptr, INVEN_SUB_HAND)) {
                    if (creature_ptr->pet_extra_flags & PF_TWO_HANDS) {
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

        if (use_menu) {
            screen_save();
            strnfmt(out_val, 78, _("(コマンド、ESC=終了) コマンドを選んでください:", "(Command, ESC=exit) Choose command from menu."));
        } else {
            strnfmt(out_val, 78, _("(コマンド %c-%c、'*'=一覧、ESC=終了) コマンドを選んでください:", "(Command %c-%c, *=List, ESC=exit) Select a command: "),
                I2A(0), I2A(num - 1));
        }

        choice = (always_show_list || use_menu) ? ESCAPE : 1;

        /* Get a command from the user */
        while (!flag) {
            int ask = true;

            if (choice == ESCAPE)
                choice = ' ';
            else if (!get_com(out_val, &choice, true))
                break;

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
                    ask = false;
                    break;
                }
                if (menu_line > num)
                    menu_line -= num;
            }

            /* Request redraw */
            if ((choice == ' ') || (choice == '*') || (choice == '?') || (use_menu && ask)) {
                /* Show the list */
                if (!redraw || use_menu) {
                    byte y = 1, x = 0;
                    redraw = true;
                    if (!use_menu)
                        screen_save();

                    prt("", y++, x);

                    /* Print list */
                    int control;
                    for (control = 0; control < num; control++) {
                        /* Letter/number for power selection */
                        if (use_menu)
                            sprintf(buf, "%c%s ", (control == command_idx) ? '*' : ' ', (control == (menu_line - 1)) ? _("》", "> ") : "  ");
                        else
                            sprintf(buf, "%c%c) ", (control == command_idx) ? '*' : ' ', I2A(control));

                        strcat(buf, power_desc[control]);

                        prt(buf, y + control, x);
                    }

                    prt("", y + MIN(control, 17), x);
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
                /* Note verify */
                ask = (isupper(choice));

                /* Lowercase */
                if (ask)
                    choice = (char)tolower(choice);

                /* Extract request */
                i = (islower(choice) ? A2I(choice) : -1);
            }

            /* Totally Illegal */
            if ((i < 0) || (i >= num)) {
                bell();
                continue;
            }

            /* Verify it */
            if (ask) {
                /* Prompt */
                strnfmt(buf, 78, _("%sを使いますか？ ", "Use %s? "), power_desc[i]);

                /* Belay that order */
                if (!get_check(buf))
                    continue;
            }

            /* Stop the loop */
            flag = true;
        }
        if (redraw)
            screen_load();

        /* Abort if needed */
        if (!flag) {
            PlayerEnergy(creature_ptr).reset_player_turn();
            return;
        }

        repeat_push(i);
    }
    switch (powers[i]) {
    case PET_DISMISS: /* Dismiss pets */
    {
        /* Check pets (backwards) */
        for (pet_ctr = creature_ptr->current_floor_ptr->m_max - 1; pet_ctr >= 1; pet_ctr--) {
            /* Player has pet */
            if (is_pet(&creature_ptr->current_floor_ptr->m_list[pet_ctr]))
                break;
        }

        if (!pet_ctr) {
            msg_print(_("ペットがいない！", "You have no pets!"));
            break;
        }
        do_cmd_pet_dismiss(creature_ptr);
        (void)calculate_upkeep(creature_ptr);
        break;
    }
    case PET_TARGET: {
        project_length = -1;
        if (!target_set(creature_ptr, TARGET_KILL))
            creature_ptr->pet_t_m_idx = 0;
        else {
            grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[target_row][target_col];
            if (g_ptr->m_idx && (creature_ptr->current_floor_ptr->m_list[g_ptr->m_idx].ml)) {
                creature_ptr->pet_t_m_idx = creature_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx;
                creature_ptr->pet_follow_distance = PET_DESTROY_DIST;
            } else
                creature_ptr->pet_t_m_idx = 0;
        }
        project_length = 0;

        break;
    }
    /* Call pets */
    case PET_STAY_CLOSE: {
        creature_ptr->pet_follow_distance = PET_CLOSE_DIST;
        creature_ptr->pet_t_m_idx = 0;
        break;
    }
    /* "Follow Me" */
    case PET_FOLLOW_ME: {
        creature_ptr->pet_follow_distance = PET_FOLLOW_DIST;
        creature_ptr->pet_t_m_idx = 0;
        break;
    }
    /* "Seek and destoy" */
    case PET_SEEK_AND_DESTROY: {
        creature_ptr->pet_follow_distance = PET_DESTROY_DIST;
        break;
    }
    /* "Give me space" */
    case PET_ALLOW_SPACE: {
        creature_ptr->pet_follow_distance = PET_SPACE_DIST;
        break;
    }
    /* "Stay away" */
    case PET_STAY_AWAY: {
        creature_ptr->pet_follow_distance = PET_AWAY_DIST;
        break;
    }
    /* flag - allow pets to open doors */
    case PET_OPEN_DOORS: {
        if (creature_ptr->pet_extra_flags & PF_OPEN_DOORS)
            creature_ptr->pet_extra_flags &= ~(PF_OPEN_DOORS);
        else
            creature_ptr->pet_extra_flags |= (PF_OPEN_DOORS);
        break;
    }
    /* flag - allow pets to pickup items */
    case PET_TAKE_ITEMS: {
        if (creature_ptr->pet_extra_flags & PF_PICKUP_ITEMS) {
            creature_ptr->pet_extra_flags &= ~(PF_PICKUP_ITEMS);
            for (pet_ctr = creature_ptr->current_floor_ptr->m_max - 1; pet_ctr >= 1; pet_ctr--) {
                m_ptr = &creature_ptr->current_floor_ptr->m_list[pet_ctr];

                if (is_pet(m_ptr)) {
                    monster_drop_carried_objects(creature_ptr, m_ptr);
                }
            }
        } else
            creature_ptr->pet_extra_flags |= (PF_PICKUP_ITEMS);

        break;
    }
    /* flag - allow pets to teleport */
    case PET_TELEPORT: {
        if (creature_ptr->pet_extra_flags & PF_TELEPORT)
            creature_ptr->pet_extra_flags &= ~(PF_TELEPORT);
        else
            creature_ptr->pet_extra_flags |= (PF_TELEPORT);
        break;
    }
    /* flag - allow pets to cast attack spell */
    case PET_ATTACK_SPELL: {
        if (creature_ptr->pet_extra_flags & PF_ATTACK_SPELL)
            creature_ptr->pet_extra_flags &= ~(PF_ATTACK_SPELL);
        else
            creature_ptr->pet_extra_flags |= (PF_ATTACK_SPELL);
        break;
    }
    /* flag - allow pets to cast attack spell */
    case PET_SUMMON_SPELL: {
        if (creature_ptr->pet_extra_flags & PF_SUMMON_SPELL)
            creature_ptr->pet_extra_flags &= ~(PF_SUMMON_SPELL);
        else
            creature_ptr->pet_extra_flags |= (PF_SUMMON_SPELL);
        break;
    }
    /* flag - allow pets to cast attack spell */
    case PET_BALL_SPELL: {
        if (creature_ptr->pet_extra_flags & PF_BALL_SPELL)
            creature_ptr->pet_extra_flags &= ~(PF_BALL_SPELL);
        else
            creature_ptr->pet_extra_flags |= (PF_BALL_SPELL);
        break;
    }

    case PET_RIDING: {
        (void)do_cmd_riding(creature_ptr, false);
        break;
    }

    case PET_NAME: {
        do_name_pet(creature_ptr);
        break;
    }

    case PET_TWO_HANDS: {
        if (creature_ptr->pet_extra_flags & PF_TWO_HANDS)
            creature_ptr->pet_extra_flags &= ~(PF_TWO_HANDS);
        else
            creature_ptr->pet_extra_flags |= (PF_TWO_HANDS);
        creature_ptr->update |= (PU_BONUS);
        handle_stuff(creature_ptr);
        break;
    }
    }
}
