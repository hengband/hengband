#include "view/display-self-info.h"
#include "avatar/avatar.h"
#include "io/input-key-acceptor.h"
#include "player-info/alignment.h"
#include "player-info/race-info.h"
#include "player-info/self-info-util.h"
#include "player/player-status-table.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include <string>

void display_life_rating(player_type *player_ptr, self_info_type *self_ptr)
{
    player_ptr->knowledge |= KNOW_STAT | KNOW_HPRATE;
    strcpy(self_ptr->plev_buf, "");
    int percent
        = (int)(((long)player_ptr->player_hp[PY_MAX_LEVEL - 1] * 200L) / (2 * player_ptr->hitdie + ((PY_MAX_LEVEL - 1 + 3) * (player_ptr->hitdie + 1))));
    sprintf(self_ptr->plev_buf, _("現在の体力ランク : %d/100", "Your current Life Rating is %d/100."), percent);
    strcpy(self_ptr->buf[0], self_ptr->plev_buf);
    self_ptr->info[self_ptr->line++] = self_ptr->buf[0];
    self_ptr->info[self_ptr->line++] = "";
}

void display_max_base_status(player_type *player_ptr, self_info_type *self_ptr)
{
    self_ptr->info[self_ptr->line++] = _("能力の最大値", "Limits of maximum stats");
    for (int v_nr = 0; v_nr < A_MAX; v_nr++) {
        char stat_desc[80];
        sprintf(stat_desc, "%s 18/%d", stat_names[v_nr], player_ptr->stat_max_max[v_nr] - 18);
        strcpy(self_ptr->s_string[v_nr], stat_desc);
        self_ptr->info[self_ptr->line++] = self_ptr->s_string[v_nr];
    }
}

void display_virtue(player_type *player_ptr, self_info_type *self_ptr)
{
    self_ptr->info[self_ptr->line++] = "";
    std::string alg = PlayerAlignment(player_ptr).get_alignment_description(true);
    sprintf(self_ptr->plev_buf, _("現在の属性 : %s", "Your alignment : %s"), alg.c_str());
    strcpy(self_ptr->buf[1], self_ptr->plev_buf);
    self_ptr->info[self_ptr->line++] = self_ptr->buf[1];
    for (int v_nr = 0; v_nr < 8; v_nr++) {
        GAME_TEXT vir_name[20];
        char vir_desc[80];
        int tester = player_ptr->virtues[v_nr];
        strcpy(vir_name, virtue[(player_ptr->vir_types[v_nr]) - 1]);
        sprintf(vir_desc, _("おっと。%sの情報なし。", "Oops. No info about %s."), vir_name);
        if (tester < -100)
            sprintf(vir_desc, _("[%s]の対極 (%d)", "You are the polar opposite of %s (%d)."), vir_name, tester);
        else if (tester < -80)
            sprintf(vir_desc, _("[%s]の大敵 (%d)", "You are an arch-enemy of %s (%d)."), vir_name, tester);
        else if (tester < -60)
            sprintf(vir_desc, _("[%s]の強敵 (%d)", "You are a bitter enemy of %s (%d)."), vir_name, tester);
        else if (tester < -40)
            sprintf(vir_desc, _("[%s]の敵 (%d)", "You are an enemy of %s (%d)."), vir_name, tester);
        else if (tester < -20)
            sprintf(vir_desc, _("[%s]の罪者 (%d)", "You have sinned against %s (%d)."), vir_name, tester);
        else if (tester < 0)
            sprintf(vir_desc, _("[%s]の迷道者 (%d)", "You have strayed from the path of %s (%d)."), vir_name, tester);
        else if (tester == 0)
            sprintf(vir_desc, _("[%s]の中立者 (%d)", "You are neutral to %s (%d)."), vir_name, tester);
        else if (tester < 20)
            sprintf(vir_desc, _("[%s]の小徳者 (%d)", "You are somewhat virtuous in %s (%d)."), vir_name, tester);
        else if (tester < 40)
            sprintf(vir_desc, _("[%s]の中徳者 (%d)", "You are virtuous in %s (%d)."), vir_name, tester);
        else if (tester < 60)
            sprintf(vir_desc, _("[%s]の高徳者 (%d)", "You are very virtuous in %s (%d)."), vir_name, tester);
        else if (tester < 80)
            sprintf(vir_desc, _("[%s]の覇者 (%d)", "You are a champion of %s (%d)."), vir_name, tester);
        else if (tester < 100)
            sprintf(vir_desc, _("[%s]の偉大な覇者 (%d)", "You are a great champion of %s (%d)."), vir_name, tester);
        else
            sprintf(vir_desc, _("[%s]の具現者 (%d)", "You are the living embodiment of %s (%d)."), vir_name, tester);

        strcpy(self_ptr->v_string[v_nr], vir_desc);
        self_ptr->info[self_ptr->line++] = self_ptr->v_string[v_nr];
    }
}

void display_mimic_race_ability(player_type *player_ptr, self_info_type *self_ptr)
{
    switch (player_ptr->mimic_form) {
    case MIMIC_DEMON:
    case MIMIC_DEMON_LORD:
        sprintf(self_ptr->plev_buf, _("あなたは %d ダメージの地獄か火炎のブレスを吐くことができる。(%d MP)", "You can breathe nether, dam. %d (cost %d)."),
            3 * player_ptr->lev, 10 + player_ptr->lev / 3);

        self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
        break;
    case MIMIC_VAMPIRE:
        if (player_ptr->lev <= 1)
            break;

        sprintf(self_ptr->plev_buf, _("あなたは敵から %d-%d HP の生命力を吸収できる。(%d MP)", "You can steal life from a foe, dam. %d-%d (cost %d)."),
            player_ptr->lev + std::max(1, player_ptr->lev / 10), player_ptr->lev + player_ptr->lev * std::max(1, player_ptr->lev / 10),
            1 + (player_ptr->lev / 3));
        self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
        break;
    }
}

void display_self_info(self_info_type *self_ptr)
{
    screen_save();
    for (int k = 1; k < 24; k++)
        prt("", k, 13);

    prt(_("        あなたの状態:", "     Your Attributes:"), 1, 15);
    int k = 2;
    for (int j = 0; j < self_ptr->line; j++) {
        prt(self_ptr->info[j], k++, 15);

        /* Every 20 entries (lines 2 to 21), start over */
        if ((k != 22) || (j + 1 >= self_ptr->line))
            continue;

        prt(_("-- 続く --", "-- more --"), k, 15);
        inkey();
        for (; k > 2; k--)
            prt("", k, 15);
    }

    prt(_("[何かキーを押すとゲームに戻ります]", "[Press any key to continue]"), k, 13);
    inkey();
    screen_load();
}
