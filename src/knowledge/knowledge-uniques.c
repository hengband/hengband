/*!
 * @brief 既知/存命のユニークを表示する
 * @date 2020/04/23
 * @author Hourier
 */

#include "core/show-file.h"
#include "core/sort.h"
#include "io-dump/dump-util.h"
#include "knowledge-items.h"
#include "system/angband.h"

/*!
 * @param is_alive 生きているユニークのリストならばTRUE、撃破したユニークのリストならばFALSE
 */
void do_cmd_knowledge_uniques(player_type *creature_ptr, bool is_alive)
{
    u16b why = 2;
    IDX *who;
    int num_uniques[10];
    int num_uniques_surface = 0;
    int num_uniques_over100 = 0;
    int num_uniques_total = 0;
    int max_lev = -1;
    for (IDX i = 0; i < 10; i++)
        num_uniques[i] = 0;

    FILE *fff = NULL;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name))
        return;

    C_MAKE(who, max_r_idx, MONRACE_IDX);
    int n = 0;
    for (IDX i = 1; i < max_r_idx; i++) {
        monster_race *r_ptr = &r_info[i];
        if (!r_ptr->name)
            continue;
        if (!(r_ptr->flags1 & RF1_UNIQUE))
            continue;
        if (!cheat_know && !r_ptr->r_sights)
            continue;
        if (!r_ptr->rarity || ((r_ptr->rarity > 100) && !(r_ptr->flags1 & RF1_QUESTOR)))
            continue;
        if (is_alive) {
            if (r_ptr->max_num == 0)
                continue;
        } else {
            if (r_ptr->max_num > 0)
                continue;
        }

        if (r_ptr->level) {
            int lev = (r_ptr->level - 1) / 10;
            if (lev < 10) {
                num_uniques[lev]++;
                if (max_lev < lev)
                    max_lev = lev;
            } else
                num_uniques_over100++;
        } else
            num_uniques_surface++;

        who[n++] = i;
    }

    ang_sort(who, &why, n, ang_sort_comp_hook, ang_sort_swap_hook);
    if (num_uniques_surface) {
        concptr surface_desc
            = is_alive ? _("     地上  生存: %3d体\n", "      Surface  alive: %3d\n") : _("     地上  死亡: %3d体\n", "      Surface  dead: %3d\n");
        fprintf(fff, surface_desc, num_uniques_surface);
        num_uniques_total += num_uniques_surface;
    }

    for (IDX i = 0; i <= max_lev; i++) {
        concptr dungeon_desc
            = is_alive ? _("%3d-%3d階  生存: %3d体\n", "Level %3d-%3d  alive: %3d\n") : _("%3d-%3d階  死亡: %3d体\n", "Level %3d-%3d  dead: %3d\n");
        fprintf(fff, dungeon_desc, 1 + i * 10, 10 + i * 10, num_uniques[i]);
        num_uniques_total += num_uniques[i];
    }

    if (num_uniques_over100) {
        concptr deep_desc
            = is_alive ? _("101-   階  生存: %3d体\n", "Level 101-     alive: %3d\n") : _("101-   階  死亡: %3d体\n", "Level 101-     dead: %3d\n");
        fprintf(fff, deep_desc, num_uniques_over100);
        num_uniques_total += num_uniques_over100;
    }

    if (num_uniques_total) {
        fputs(_("---------  -----------\n", "-------------  ----------\n"), fff);
        concptr total_desc
            = is_alive ? _("     合計  生存: %3d体\n\n", "        Total  alive: %3d\n\n") : _("     合計  死亡: %3d体\n\n", "        Total  dead: %3d\n\n");
        fprintf(fff, total_desc, num_uniques_total);
    } else {
        concptr no_unique_desc = is_alive ? _("現在は既知の生存ユニークはいません。\n", "No known uniques alive.\n")
                                          : _("現在は既知の撃破ユニークはいません。\n", "No known uniques dead.\n");
        fputs(no_unique_desc, fff);
    }

    for (int k = 0; k < n; k++) {
        monster_race *r_ptr = &r_info[who[k]];
        fprintf(fff, _("     %s (レベル%d)\n", "     %s (level %d)\n"), r_name + r_ptr->name, (int)r_ptr->level);
    }

    C_KILL(who, max_r_idx, s16b);
    my_fclose(fff);
    concptr title_desc = is_alive ? _("まだ生きているユニーク・モンスター", "Alive Uniques") : _("もう撃破したユニーク・モンスター", "Dead Uniques");
    (void)show_file(creature_ptr, TRUE, file_name, title_desc, 0, 0);
    fd_kill(file_name);
}
