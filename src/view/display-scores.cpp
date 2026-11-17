#include "view/display-scores.h"
#include "core/asking-player.h"
#include "core/score-file.h"
#include "io/input-context.h"
#include "locale/multibyte-char.h"
#include "player-info/class-info.h"
#include "player/player-personality.h"
#include "player/race-info-table.h"
#include "system/angband.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/int-char-converter.h"
#include <fmt/format.h>

/*!
 * @brief 死因が長すぎるときに切る
 * @param died_from 死因
 * @return 死因。行が画面からはみ出さないよう、前の版のスコアファイルの欄に入った 39 桁までにし、切ったときは末尾に省略の記号を付ける
 */
static std::string shorten_died_from(std::string &&died_from)
{
    constexpr size_t max_width = 39;
    constexpr std::string_view ellipsis = _("…", "...");
    if (str_display_width(died_from) <= max_width) {
        return std::move(died_from);
    }

    return str_prefix_by_width(std::move(died_from), max_width - str_display_width(ellipsis)).append(ellipsis);
}

/*!
 * @brief 指定された順位範囲でスコアを並べて表示する / Display the scores in a given range.
 * @param scores スコアファイルの記録 (スコアの高い順)
 * @param from 順位先頭
 * @param to 順位末尾
 * @param note 黄色表示でハイライトする順位
 * @param score note の順位に差し込んで表示する記録 (スコアの予想)。無ければ nullptr
 * @details
 * <pre>
 * Only five entries per line, too much info.
 *
 * Mega-Hack -- allow "fake" entry at the given position.
 * </pre>
 */
void display_scores(const std::vector<HighScore> &scores, int from, int to, int note, const HighScore *score)
{
    if (from < 0) {
        from = 0;
    }

    if (to < 0) {
        to = 10;
    }

    if (to > MAX_HISCORES) {
        to = MAX_HISCORES;
    }

    auto num_scores = std::min(static_cast<int>(scores.size()), MAX_HISCORES);

    if ((note == num_scores) && score) {
        num_scores++;
    }

    if (num_scores > to) {
        num_scores = to;
    }

    constexpr auto per_screen = (MAIN_TERM_MIN_ROWS - 4) / 4;
    for (auto k = from, place = k + 1; k < num_scores; k += per_screen) {
        TermCenteredOffsetSetter tcos(MAIN_TERM_MIN_COLS, MAIN_TERM_MIN_ROWS);

        term_clear();
        put_str(_("                変愚蛮怒: 勇者の殿堂", "                Hengband Hall of Fame"), 0, 0);
        if (k > 0) {
            put_str(fmt::format(_("( {} 位以下 )", "(from position {})"), k + 1), 0, 40);
        }

        for (auto n = 0, j = k; (j < num_scores) && (n < per_screen); place++, j++, n++) {
            auto attr = (j == note) ? TERM_YELLOW : TERM_WHITE;
            const HighScore *the_score_ptr = nullptr;
            if ((note == j) && score) {
                the_score_ptr = score;
                attr = TERM_L_GREEN;
                score = nullptr;
                note = -1;
                j--;
            } else if (j < std::ssize(scores)) {
                the_score_ptr = &scores[j];
            } else {
                break;
            }

            const auto &the_score = *the_score_ptr;

            const auto pr = the_score.race;
            const auto pc = the_score.pclass;
            const auto pa = the_score.personality;

            const auto clev = the_score.level;
            const auto mlev = the_score.max_level;
            const auto cdun = the_score.dungeon_level;
            const auto mdun = the_score.max_dungeon_level;

            const auto user = the_score.uid;
            const auto gold = the_score.gold;
            const auto aged = the_score.turns;
            auto when = HighScore::to_sys(the_score.date);
            const auto name = HighScore::to_sys(the_score.name);
            const auto died_from = shorten_died_from(HighScore::to_sys(the_score.died_from));

            std::string out_val;
            const auto pclass = i2enum<PlayerClassType>(pc);
#ifdef JP
            out_val = fmt::format("{:3}.{:>9}  {}{}{} - {}{} (レベル {})", place, the_score.points, personality_info[pa].title, (personality_info[pa].no ? "の" : ""), name, race_info[pr].title, class_info.at(pclass).title, clev);

#else
            out_val = fmt::format("{:3}.{:>9}  {} {} the {} {}, Level {}", place, the_score.points, personality_info[pa].title, name, race_info[pr].title, class_info.at(pclass).title, clev);
#endif
            if (mlev > clev) {
                out_val.append(fmt::format(_(" (最高{})", " (Max {})"), mlev));
            }

            c_put_str(attr, out_val, n * 4 + 2, 0);
#ifdef JP
            if (mdun != 0) {
                out_val = fmt::format("    最高{:3}階", mdun);
            } else {
                out_val = "             ";
            }

            /* 死亡原因をオリジナルより細かく表示 */
            if (the_score.died_from == "yet") {
                out_val.append(fmt::format("  まだ生きている ({}{})", cdun, "階"));
            } else if (the_score.died_from == "ripe") {
                out_val.append(fmt::format("  勝利の後に引退 ({}{})", cdun, "階"));
            } else if (the_score.died_from == "Seppuku") {
                out_val.append(fmt::format("  勝利の後に切腹 ({}{})", cdun, "階"));
            } else {
                if (!cdun) {
                    out_val.append(fmt::format("  地上で{}に殺された", died_from));
                } else {
                    out_val.append(fmt::format("  {}階で{}に殺された", cdun, died_from));
                }
            }
#else
            if (!cdun) {
                out_val = fmt::format("               Killed by {} on the surface", died_from);
            } else {
                out_val = fmt::format("               Killed by {} on {} {}", died_from, "Dungeon Level", cdun);
            }

            if (mdun > cdun) {
                out_val.append(fmt::format(" (Max {})", mdun));
            }
#endif
            c_put_str(attr, out_val, n * 4 + 3, 0);
#ifdef JP
            /* 日付を 19yy/mm/dd の形式に変更する */
            if ((when.length() == 8) && (when[2] == '/') && (when[5] == '/')) {
                when = fmt::format("{}{}/{}", 19 + (when[6] < '8'), when.substr(6), when.substr(0, 5));
            }

            out_val = fmt::format("        (ユーザー:{}, 日付:{}, 所持金:{}, ターン:{})", user, when, gold, aged);
#else
            out_val = fmt::format("               (User {}, Date {}, Gold {}, Turn {}).", user, when, gold, aged);
#endif

            c_put_str(attr, out_val, n * 4 + 4, 0);
        }

        constexpr auto prompt = _("[ ESCで中断, その他のキーで続けます ]", "[Press ESC to quit, any other key to continue.]");
        prt(prompt, MAIN_TERM_MIN_ROWS - 1, _(21, 17));
        const auto key = input_key_with_context(InputKind::ANY_KEY, prompt);
        prt("", MAIN_TERM_MIN_ROWS - 1, 0);
        if (key == ESCAPE) {
            break;
        }
    }
}

#ifndef _WIN32
/*!
 * @brief スコア表示処理メインルーチン / Display the scores in a given range and quit.
 * @param from 順位先頭
 * @param to 順位末尾
 * @details
 * スコア表示した後、或いは表示に失敗したら終了する (Windows版は表示処理の中で終了することはない)
 * main.cpp からしか呼ばれていない。暫定でプリプロで包んでおく
 * 取り敢えずWindows版はオーバーロード解決できている。他は不明
 */
void display_scores(int from, int to)
{
    // ゲームの初期化より前に呼ばれるので、scores.raw からの移し替えをここでも行う
    const auto scores = prepare_score_file() ? read_score_file() : tl::nullopt;
    if (!scores) {
        quit(_("スコア・ファイルが使用できません。", "Score file unavailable."));
    }

    term_clear();
    display_scores(*scores, from, to, -1, nullptr);
    quit("");
}
#endif
