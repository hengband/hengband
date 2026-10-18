#include "view/display-self-info.h"
#include "avatar/avatar.h"
#include "io/input-key-acceptor.h"
#include "player-info/alignment.h"
#include "player-info/race-info.h"
#include "player-info/self-info-util.h"
#include "player/player-status-table.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include <fmt/format.h>
#include <string>

void display_life_rating(PlayerType *player_ptr, self_info_type *self_ptr)
{
    player_ptr->knowledge |= KNOW_STAT | KNOW_HPRATE;
    auto info = fmt::format(_("現在の体力ランク : {}/100", "Your current Life Rating is {}/100."), player_ptr->calc_life_rating());
    self_ptr->info_list.push_back(std::move(info));
    self_ptr->info_list.emplace_back("");
}

void display_max_base_status(PlayerType *player_ptr, self_info_type *self_ptr)
{
    self_ptr->info_list.emplace_back(_("能力の最大値", "Limits of maximum stats"));
    for (int v_nr = 0; v_nr < A_MAX; v_nr++) {
        auto stat = fmt::format("{} 18/{}", stat_names[v_nr], player_ptr->stat_max_max[v_nr] - 18);
        self_ptr->info_list.push_back(std::move(stat));
    }
}

void display_virtue(PlayerType *player_ptr, self_info_type *self_ptr)
{
    self_ptr->info_list.emplace_back("");
    const std::string alg = PlayerAlignment(player_ptr).get_alignment_description(true);
    self_ptr->info_list.push_back(fmt::format(_("現在の属性 : {}", "Your alignment : {}"), alg));

    for (int v_nr = 0; v_nr < 8; v_nr++) {
        const auto vir_name = virtue_names.at(player_ptr->vir_types[v_nr]).data();
        std::string vir_desc;
        int tester = player_ptr->virtues[v_nr];
        if (tester < -100) {
            vir_desc = fmt::format(_("[{}]の対極 ({})", "You are the polar opposite of {} ({})."), vir_name, tester);
        } else if (tester < -80) {
            vir_desc = fmt::format(_("[{}]の大敵 ({})", "You are an arch-enemy of {} ({})."), vir_name, tester);
        } else if (tester < -60) {
            vir_desc = fmt::format(_("[{}]の強敵 ({})", "You are a bitter enemy of {} ({})."), vir_name, tester);
        } else if (tester < -40) {
            vir_desc = fmt::format(_("[{}]の敵 ({})", "You are an enemy of {} ({})."), vir_name, tester);
        } else if (tester < -20) {
            vir_desc = fmt::format(_("[{}]の罪者 ({})", "You have sinned against {} ({})."), vir_name, tester);
        } else if (tester < 0) {
            vir_desc = fmt::format(_("[{}]の迷道者 ({})", "You have strayed from the path of {} ({})."), vir_name, tester);
        } else if (tester == 0) {
            vir_desc = fmt::format(_("[{}]の中立者 ({})", "You are neutral to {} ({})."), vir_name, tester);
        } else if (tester < 20) {
            vir_desc = fmt::format(_("[{}]の小徳者 ({})", "You are somewhat virtuous in {} ({})."), vir_name, tester);
        } else if (tester < 40) {
            vir_desc = fmt::format(_("[{}]の中徳者 ({})", "You are virtuous in {} ({})."), vir_name, tester);
        } else if (tester < 60) {
            vir_desc = fmt::format(_("[{}]の高徳者 ({})", "You are very virtuous in {} ({})."), vir_name, tester);
        } else if (tester < 80) {
            vir_desc = fmt::format(_("[{}]の覇者 ({})", "You are a champion of {} ({})."), vir_name, tester);
        } else if (tester < 100) {
            vir_desc = fmt::format(_("[{}]の偉大な覇者 ({})", "You are a great champion of {} ({})."), vir_name, tester);
        } else {
            vir_desc = fmt::format(_("[{}]の具現者 ({})", "You are the living embodiment of {} ({})."), vir_name, tester);
        }

        self_ptr->info_list.push_back(std::move(vir_desc));
    }
}

void display_mimic_race_ability(PlayerType *player_ptr, self_info_type *self_ptr)
{
    switch (player_ptr->mimic_form) {
    case MimicKindType::NONE:
        return;
    case MimicKindType::DEMON:
    case MimicKindType::DEMON_LORD: {
        constexpr auto fmt = _("あなたは {} ダメージの地獄か火炎のブレスを吐くことができる。({} MP)", "You can breathe nether, dam. {} (cost {}).");
        const auto dam = 3 * player_ptr->lev;
        const auto cost = 10 + player_ptr->lev / 3;
        self_ptr->info_list.push_back(fmt::format(fmt, dam, cost));
        return;
    }
    case MimicKindType::VAMPIRE:
        if (player_ptr->lev >= 2) {
            constexpr auto fmt = _("あなたは敵から {}-{} HP の生命力を吸収できる。({} MP)", "You can steal life from a foe, dam. {}-{} (cost {}).");
            const auto lev = player_ptr->lev;
            const auto min_dam = lev + std::max(1, lev / 10);
            const auto max_dam = lev + lev * std::max(1, lev / 10);
            const auto cost = 1 + lev / 3;
            self_ptr->info_list.push_back(fmt::format(fmt, min_dam, max_dam, cost));
        }
        return;
    case MimicKindType::ANGEL:
        return;
    case MimicKindType::DEMIGOD:
        return;
    default:
        return;
    }
}

void display_self_info(self_info_type *self_ptr)
{
    screen_save();
    for (int k = 1; k < MAIN_TERM_MIN_ROWS; k++) {
        prt("", k, 13);
    }

    prt(_("        あなたの状態:", "     Your Attributes:"), 1, 15);
    int k = 2;
    for (auto j = 0; j < std::ssize(self_ptr->info_list); j++) {
        prt(self_ptr->info_list[j], k++, 15);

        /* Every 20 entries (lines 2 to 21), start over */
        if ((k != 22) || (j + 1 >= std::ssize(self_ptr->info_list))) {
            continue;
        }

        prt(_("-- 続く --", "-- more --"), k, 15);
        inkey();
        for (; k > 2; k--) {
            prt("", k, 15);
        }
    }

    prt(_("[何かキーを押すとゲームに戻ります]", "[Press any key to continue]"), k, 13);
    inkey();
    screen_load();
}
