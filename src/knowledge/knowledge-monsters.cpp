﻿/*!
 * todo サブルーチン分割を行うと行数が膨れ上がりそう、再分割も検討すべし
 * @brief 既知のモンスターに関する情報を表示する
 * @date 2020/04/24
 * @author Hourier
 */

#include "knowledge/knowledge-monsters.h"
#include "core/show-file.h"
#include "core/stuff-handler.h"
#include "game-option/cheat-options.h"
#include "game-option/special-options.h"
#include "io-dump/dump-util.h"
#include "io/input-key-acceptor.h"
#include "knowledge/monster-group-table.h"
#include "locale/english.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "system/floor-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "util/sort.h"
#include "util/string-processor.h"
#include "view/display-lore.h"
#include "view/display-monster-status.h"
#include "world/world.h"

/*
 * todo 引数と戻り値について追記求む
 * Build a list of monster indexes in the given group.
 *
 * mode & 0x01 : check for non-empty group
 * mode & 0x02 : visual operation only

 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param grp_cur ？？？
 * @param mon_idx[] ？？？
 * @param mode ？？？
 * @return The number of monsters in the group
 */
static IDX collect_monsters(player_type *creature_ptr, IDX grp_cur, IDX mon_idx[], monster_lore_mode mode)
{
    concptr group_char = monster_group_char[grp_cur];
    bool grp_unique = (monster_group_char[grp_cur] == (char *)-1L);
    bool grp_riding = (monster_group_char[grp_cur] == (char *)-2L);
    bool grp_wanted = (monster_group_char[grp_cur] == (char *)-3L);
    bool grp_amberite = (monster_group_char[grp_cur] == (char *)-4L);

    IDX mon_cnt = 0;
    for (IDX i = 0; i < max_r_idx; i++) {
        monster_race *r_ptr = &r_info[i];
        if (!r_ptr->name)
            continue;
        if (((mode != MONSTER_LORE_DEBUG) && (mode != MONSTER_LORE_RESEARCH)) && !cheat_know && !r_ptr->r_sights)
            continue;

        if (grp_unique) {
            if (none_bits(r_ptr->flags1, RF1_UNIQUE))
                continue;
        } else if (grp_riding) {
            if (none_bits(r_ptr->flags7, RF7_RIDING))
                continue;
        } else if (grp_wanted) {
            bool wanted = FALSE;
            for (int j = 0; j < MAX_BOUNTY; j++) {
                if (current_world_ptr->bounty_r_idx[j] == i || current_world_ptr->bounty_r_idx[j] - 10000 == i
                    || (creature_ptr->today_mon && creature_ptr->today_mon == i)) {
                    wanted = TRUE;
                    break;
                }
            }

            if (!wanted)
                continue;
        } else if (grp_amberite) {
            if (none_bits(r_ptr->flags3, RF3_AMBERITE))
                continue;
        } else {
            if (!angband_strchr(group_char, r_ptr->d_char))
                continue;
        }

        mon_idx[mon_cnt++] = i;
        if (mode == MONSTER_LORE_NORMAL)
            break;
        if (mode == MONSTER_LORE_DEBUG)
            break;
    }

    mon_idx[mon_cnt] = -1;
    int dummy_why;
    ang_sort(creature_ptr, mon_idx, &dummy_why, mon_cnt, ang_sort_comp_monster_level, ang_sort_swap_hook);
    return mon_cnt;
}

/*!
 * @brief 現在のペットを表示するコマンドのメインルーチン /
 * Display current pets
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_knowledge_pets(player_type *creature_ptr)
{
    FILE *fff = NULL;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name))
        return;

    monster_type *m_ptr;
    GAME_TEXT pet_name[MAX_NLEN];
    int t_friends = 0;
    for (int i = creature_ptr->current_floor_ptr->m_max - 1; i >= 1; i--) {
        m_ptr = &creature_ptr->current_floor_ptr->m_list[i];
        if (!monster_is_valid(m_ptr) || !is_pet(m_ptr))
            continue;

        t_friends++;
        monster_desc(creature_ptr, pet_name, m_ptr, MD_ASSUME_VISIBLE | MD_INDEF_VISIBLE);
        fprintf(fff, "%s (%s)\n", pet_name, look_mon_desc(m_ptr, 0x00));
    }

    int show_upkeep = calculate_upkeep(creature_ptr);

    fprintf(fff, "----------------------------------------------\n");
#ifdef JP
    fprintf(fff, "    合計: %d 体のペット\n", t_friends);
#else
    fprintf(fff, "   Total: %d pet%s.\n", t_friends, (t_friends == 1 ? "" : "s"));
#endif
    fprintf(fff, _(" 維持コスト: %d%% MP\n", "   Upkeep: %d%% mana.\n"), show_upkeep);

    angband_fclose(fff);
    (void)show_file(creature_ptr, TRUE, file_name, _("現在のペット", "Current Pets"), 0, 0);
    fd_kill(file_name);
}

/*!
 * @brief 現在までに倒したモンスターを表示するコマンドのメインルーチン /
 * @param creature_ptr プレーヤーへの参照ポインタ
 * Total kill count
 * @return なし
 * @note the player ghosts are ignored.
 */
void do_cmd_knowledge_kill_count(player_type *creature_ptr)
{
    FILE *fff = NULL;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name))
        return;

    MONRACE_IDX *who;
    C_MAKE(who, max_r_idx, MONRACE_IDX);
    s32b total = 0;
    for (int kk = 1; kk < max_r_idx; kk++) {
        monster_race *r_ptr = &r_info[kk];

        if (any_bits(r_ptr->flags1, RF1_UNIQUE)) {
            bool dead = (r_ptr->max_num == 0);

            if (dead) {
                total++;
            }
        } else {
            MONSTER_NUMBER this_monster = r_ptr->r_pkills;

            if (this_monster > 0) {
                total += this_monster;
            }
        }
    }

    if (total < 1)
        fprintf(fff, _("あなたはまだ敵を倒していない。\n\n", "You have defeated no enemies yet.\n\n"));
    else
#ifdef JP
        fprintf(fff, "あなたは%ld体の敵を倒している。\n\n", (long int)total);
#else
        fprintf(fff, "You have defeated %ld %s.\n\n", (long int)total, (total == 1) ? "enemy" : "enemies");
#endif

    total = 0;
    int n = 0;
    for (MONRACE_IDX i = 1; i < max_r_idx; i++) {
        monster_race *r_ptr = &r_info[i];
        if (r_ptr->name)
            who[n++] = i;
    }

    u16b why = 2;
    ang_sort(creature_ptr, who, &why, n, ang_sort_comp_hook, ang_sort_swap_hook);
    for (int k = 0; k < n; k++) {
        monster_race *r_ptr = &r_info[who[k]];
        if (any_bits(r_ptr->flags1, RF1_UNIQUE)) {
            bool dead = (r_ptr->max_num == 0);
            if (dead) {
                fprintf(fff, "     %s\n", (r_name + r_ptr->name));
                total++;
            }

            continue;
        }

        MONSTER_NUMBER this_monster = r_ptr->r_pkills;
        if (this_monster <= 0)
            continue;

#ifdef JP
        concptr number_of_kills = angband_strchr("pt", r_ptr->d_char) ? "人" : "体";
        fprintf(fff, "     %3d %sの %s\n", (int)this_monster, number_of_kills, r_name + r_ptr->name);
#else
        if (this_monster < 2) {
            if (angband_strstr(r_name + r_ptr->name, "coins")) {
                fprintf(fff, "     1 pile of %s\n", (r_name + r_ptr->name));
            } else {
                fprintf(fff, "     1 %s\n", (r_name + r_ptr->name));
            }
        } else {
            char ToPlural[80];
            strcpy(ToPlural, (r_name + r_ptr->name));
            plural_aux(ToPlural);
            fprintf(fff, "     %d %s\n", this_monster, ToPlural);
        }
#endif
        total += this_monster;
    }

    fprintf(fff, "----------------------------------------------\n");
#ifdef JP
    fprintf(fff, "    合計: %lu 体を倒した。\n", (unsigned long int)total);
#else
    fprintf(fff, "   Total: %lu creature%s killed.\n", (unsigned long int)total, (total == 1 ? "" : "s"));
#endif

    C_KILL(who, max_r_idx, s16b);
    angband_fclose(fff);
    (void)show_file(creature_ptr, TRUE, file_name, _("倒した敵の数", "Kill Count"), 0, 0);
    fd_kill(file_name);
}

/*
 * Display the monsters in a group.
 */
static void display_monster_list(int col, int row, int per_page, s16b mon_idx[], int mon_cur, int mon_top, bool visual_only)
{
    int i;
    for (i = 0; i < per_page && (mon_idx[mon_top + i] >= 0); i++) {
        TERM_COLOR attr;
        MONRACE_IDX r_idx = mon_idx[mon_top + i];
        monster_race *r_ptr = &r_info[r_idx];
        attr = ((i + mon_top == mon_cur) ? TERM_L_BLUE : TERM_WHITE);
        c_prt(attr, (r_name + r_ptr->name), row + i, col);
        if (per_page == 1)
            c_prt(attr, format("%02x/%02x", r_ptr->x_attr, r_ptr->x_char), row + i, (current_world_ptr->wizard || visual_only) ? 56 : 61);

        if (current_world_ptr->wizard || visual_only)
            c_prt(attr, format("%d", r_idx), row + i, 62);

        term_erase(69, row + i, 255);
        term_queue_bigchar(use_bigtile ? 69 : 70, row + i, r_ptr->x_attr, r_ptr->x_char, 0, 0);
        if (!visual_only) {
            if (none_bits(r_ptr->flags1, RF1_UNIQUE))
                put_str(format("%5d", r_ptr->r_pkills), row + i, 73);
            else
                c_put_str((r_ptr->max_num == 0 ? TERM_L_DARK : TERM_WHITE), (r_ptr->max_num == 0 ? _("死亡", " dead") : _("生存", "alive")), row + i, 74);
        }
    }

    for (; i < per_page; i++) {
        term_erase(col, row + i, 255);
    }
}

/*
 * todo 引数の詳細について加筆求む
 * Display known monsters.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param need_redraw 画面の再描画が必要な時TRUE
 * @param visual_only ？？？
 * @param direct_r_idx モンスターID
 * @return なし
 */
void do_cmd_knowledge_monsters(player_type *creature_ptr, bool *need_redraw, bool visual_only, IDX direct_r_idx)
{
    TERM_LEN wid, hgt;
    term_get_size(&wid, &hgt);
    IDX *mon_idx;
    C_MAKE(mon_idx, max_r_idx, MONRACE_IDX);

    int max = 0;
    IDX grp_cnt = 0;
    IDX grp_idx[100];
    IDX mon_cnt;
    bool visual_list = FALSE;
    TERM_COLOR attr_top = 0;
    byte char_left = 0;
    monster_lore_mode mode;
    int browser_rows = hgt - 8;
    if (direct_r_idx < 0) {
        mode = visual_only ? MONSTER_LORE_DEBUG : MONSTER_LORE_NORMAL;
        int len;
        for (IDX i = 0; monster_group_text[i] != NULL; i++) {
            len = strlen(monster_group_text[i]);
            if (len > max)
                max = len;

            if ((monster_group_char[i] == ((char *)-1L)) || collect_monsters(creature_ptr, i, mon_idx, mode)) {
                grp_idx[grp_cnt++] = i;
            }
        }

        mon_cnt = 0;
    } else {
        mon_idx[0] = direct_r_idx;
        mon_cnt = 1;
        mon_idx[1] = -1;

        (void)visual_mode_command('v', &visual_list, browser_rows - 1, wid - (max + 3), &attr_top, &char_left, &r_info[direct_r_idx].x_attr,
            &r_info[direct_r_idx].x_char, need_redraw);
    }

    grp_idx[grp_cnt] = -1;
    mode = visual_only ? MONSTER_LORE_RESEARCH : MONSTER_LORE_NONE;
    IDX old_grp_cur = -1;
    IDX grp_cur = 0;
    IDX grp_top = 0;
    IDX mon_cur = 0;
    IDX mon_top = 0;
    int column = 0;
    bool flag = FALSE;
    bool redraw = TRUE;
    while (!flag) {
        if (redraw) {
            clear_from(0);
            prt(format(_("%s - モンスター", "%s - monsters"), !visual_only ? _("知識", "Knowledge") : _("表示", "Visuals")), 2, 0);
            if (direct_r_idx < 0)
                prt(_("グループ", "Group"), 4, 0);
            prt(_("名前", "Name"), 4, max + 3);
            if (current_world_ptr->wizard || visual_only)
                prt("Idx", 4, 62);
            prt(_("文字", "Sym"), 4, 67);
            if (!visual_only)
                prt(_("殺害数", "Kills"), 4, 72);

            for (IDX i = 0; i < 78; i++) {
                term_putch(i, 5, TERM_WHITE, '=');
            }

            if (direct_r_idx < 0) {
                for (IDX i = 0; i < browser_rows; i++) {
                    term_putch(max + 1, 6 + i, TERM_WHITE, '|');
                }
            }

            redraw = FALSE;
        }

        if (direct_r_idx < 0) {
            if (grp_cur < grp_top)
                grp_top = grp_cur;
            if (grp_cur >= grp_top + browser_rows)
                grp_top = grp_cur - browser_rows + 1;

            display_group_list(0, 6, max, browser_rows, grp_idx, monster_group_text, grp_cur, grp_top);
            if (old_grp_cur != grp_cur) {
                old_grp_cur = grp_cur;
                mon_cnt = collect_monsters(creature_ptr, grp_idx[grp_cur], mon_idx, mode);
            }

            while (mon_cur < mon_top)
                mon_top = MAX(0, mon_top - browser_rows / 2);
            while (mon_cur >= mon_top + browser_rows)
                mon_top = MIN(mon_cnt - browser_rows, mon_top + browser_rows / 2);
        }

        if (!visual_list) {
            display_monster_list(max + 3, 6, browser_rows, mon_idx, mon_cur, mon_top, visual_only);
        } else {
            mon_top = mon_cur;
            display_monster_list(max + 3, 6, 1, mon_idx, mon_cur, mon_top, visual_only);
            display_visual_list(max + 3, 7, browser_rows - 1, wid - (max + 3), attr_top, char_left);
        }

        prt(format(_("<方向>%s%s%s, ESC", "<dir>%s%s%s, ESC"), (!visual_list && !visual_only) ? _(", 'r'で思い出を見る", ", 'r' to recall") : "",
                visual_list ? _(", ENTERで決定", ", ENTER to accept") : _(", 'v'でシンボル変更", ", 'v' for visuals"),
                (attr_idx || char_idx) ? _(", 'c', 'p'でペースト", ", 'c', 'p' to paste") : _(", 'c'でコピー", ", 'c' to copy")),
            hgt - 1, 0);

        monster_race *r_ptr;
        r_ptr = &r_info[mon_idx[mon_cur]];

        if (!visual_only) {
            if (mon_cnt)
                monster_race_track(creature_ptr, mon_idx[mon_cur]);
            handle_stuff(creature_ptr);
        }

        if (visual_list) {
            place_visual_list_cursor(max + 3, 7, r_ptr->x_attr, r_ptr->x_char, attr_top, char_left);
        } else if (!column) {
            term_gotoxy(0, 6 + (grp_cur - grp_top));
        } else {
            term_gotoxy(max + 3, 6 + (mon_cur - mon_top));
        }

        char ch = inkey();
        if (visual_mode_command(ch, &visual_list, browser_rows - 1, wid - (max + 3), &attr_top, &char_left, &r_ptr->x_attr, &r_ptr->x_char, need_redraw)) {
            if (direct_r_idx >= 0) {
                switch (ch) {
                case '\n':
                case '\r':
                case ESCAPE:
                    flag = TRUE;
                    break;
                }
            }

            continue;
        }

        switch (ch) {
        case ESCAPE: {
            flag = TRUE;
            break;
        }

        case 'R':
        case 'r': {
            if (!visual_list && !visual_only && (mon_idx[mon_cur] > 0)) {
                screen_roff(creature_ptr, mon_idx[mon_cur], MONSTER_LORE_NORMAL);

                (void)inkey();

                redraw = TRUE;
            }

            break;
        }

        default: {
            browser_cursor(ch, &column, &grp_cur, grp_cnt, &mon_cur, mon_cnt);

            break;
        }
        }
    }

    C_KILL(mon_idx, max_r_idx, MONRACE_IDX);
}

/*
 * List wanted monsters
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_knowledge_bounty(player_type *creature_ptr)
{
    FILE *fff = NULL;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name))
        return;

    fprintf(fff, _("今日のターゲット : %s\n", "Today's target : %s\n"),
        (creature_ptr->today_mon ? r_name + r_info[creature_ptr->today_mon].name : _("不明", "unknown")));
    fprintf(fff, "\n");
    fprintf(fff, _("賞金首リスト\n", "List of wanted monsters\n"));
    fprintf(fff, "----------------------------------------------\n");

    bool listed = FALSE;
    for (int i = 0; i < MAX_BOUNTY; i++) {
        if (current_world_ptr->bounty_r_idx[i] <= 10000) {
            fprintf(fff, "%s\n", r_name + r_info[current_world_ptr->bounty_r_idx[i]].name);
            listed = TRUE;
        }
    }

    if (!listed)
        fprintf(fff, "\n%s\n", _("賞金首はもう残っていません。", "There are no more wanted monster."));

    angband_fclose(fff);
    (void)show_file(creature_ptr, TRUE, file_name, _("賞金首の一覧", "Wanted monsters"), 0, 0);
    fd_kill(file_name);
}
