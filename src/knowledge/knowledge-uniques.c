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

/*
 * Display known uniques
 * With "XTRA HACK UNIQHIST" (Originally from XAngband)
 */
void do_cmd_knowledge_uniques(player_type *creature_ptr)
{
    u16b why = 2;
    IDX *who;
    int n_alive[10];
    int n_alive_surface = 0;
    int n_alive_over100 = 0;
    int n_alive_total = 0;
    int max_lev = -1;
    for (IDX i = 0; i < 10; i++)
        n_alive[i] = 0;

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
        if (r_ptr->max_num == 0)
            continue;

        if (r_ptr->level) {
            int lev = (r_ptr->level - 1) / 10;
            if (lev < 10) {
                n_alive[lev]++;
                if (max_lev < lev)
                    max_lev = lev;
            } else
                n_alive_over100++;
        } else
            n_alive_surface++;

        who[n++] = i;
    }

    ang_sort(who, &why, n, ang_sort_comp_hook, ang_sort_swap_hook);
    if (n_alive_surface) {
        fprintf(fff, _("     地上  生存: %3d体\n", "      Surface  alive: %3d\n"), n_alive_surface);
        n_alive_total += n_alive_surface;
    }

    for (IDX i = 0; i <= max_lev; i++) {
        fprintf(fff, _("%3d-%3d階  生存: %3d体\n", "Level %3d-%3d  alive: %3d\n"), 1 + i * 10, 10 + i * 10, n_alive[i]);
        n_alive_total += n_alive[i];
    }

    if (n_alive_over100) {
        fprintf(fff, _("101-   階  生存: %3d体\n", "Level 101-     alive: %3d\n"), n_alive_over100);
        n_alive_total += n_alive_over100;
    }

    if (n_alive_total) {
        fputs(_("---------  -----------\n", "-------------  ----------\n"), fff);
        fprintf(fff, _("     合計  生存: %3d体\n\n", "        Total  alive: %3d\n\n"), n_alive_total);
    } else {
        fputs(_("現在は既知の生存ユニークはいません。\n", "No known uniques alive.\n"), fff);
    }

    for (int k = 0; k < n; k++) {
        monster_race *r_ptr = &r_info[who[k]];
        fprintf(fff, _("     %s (レベル%d)\n", "     %s (level %d)\n"), r_name + r_ptr->name, (int)r_ptr->level);
    }

    C_KILL(who, max_r_idx, s16b);
    my_fclose(fff);
    (void)show_file(creature_ptr, TRUE, file_name, _("まだ生きているユニーク・モンスター", "Alive Uniques"), 0, 0);
    fd_kill(file_name);
}
