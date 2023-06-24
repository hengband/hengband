/*!
 * @file report.c
 * @brief スコアサーバ転送機能の実装
 * @date 2014/07/14
 * @author Hengband Team
 */

#include "io/report.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "core/turn-compensator.h"
#include "core/visuals-reseter.h"
#include "game-option/special-options.h"
#include "io-dump/character-dump.h"
#include "io/input-key-acceptor.h"
#include "mind/mind-elementalist.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/player-personality.h"
#include "player/player-status.h"
#include "realm/realm-names-table.h"
#include "system/angband-version.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <algorithm>
#include <fstream>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#ifdef WORLD_SCORE
#ifdef WINDOWS
#define CURL_STATICLIB
#endif
#include <curl/curl.h>

concptr screen_dump = nullptr;

/*
 * internet resource value
 */
#define HTTP_TIMEOUT 30 /*!< デフォルトのタイムアウト時間(秒) / Timeout length (second) */

#ifdef JP
#define SCORE_PATH "http://mars.kmc.gr.jp/~dis/heng_score/register_score.php" /*!< スコア開示URL */
#else
#define SCORE_PATH "http://moon.kmc.gr.jp/hengband/hengscore-en/score.cgi" /*!< スコア開示URL */
#endif

size_t read_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    auto &data = *static_cast<std::span<const char> *>(userdata);
    const auto copy_size = std::min<size_t>(size * nitems, data.size());

    std::copy_n(data.begin(), copy_size, buffer);
    data = data.subspan(copy_size);

    return copy_size;
}

/*!
 * @brief HTTPによるダンプ内容伝送
 * @param url 伝送先URL
 * @param buf 伝送内容バッファ
 * @return 送信に成功した場合TRUE、失敗した場合FALSE
 */
static bool http_post(concptr url, const std::vector<char> &buf)
{
    bool succeeded = false;
    CURL *curl = curl_easy_init();
    if (curl == nullptr) {
        return false;
    }

    struct curl_slist *slist = nullptr;
    slist = curl_slist_append(slist,
#ifdef JP
#ifdef SJIS
        "Content-Type: text/plain; charset=SHIFT_JIS"
#endif
#ifdef EUC
        "Content-Type: text/plain; charset=EUC-JP"
#endif
#else
        "Content-Type: text/plain; charset=ASCII"
#endif
    );

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);

    char user_agent[64];
    snprintf(user_agent, sizeof(user_agent), "Hengband %d.%d.%d", H_VER_MAJOR, H_VER_MINOR, H_VER_PATCH);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);

    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, HTTP_TIMEOUT);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, HTTP_TIMEOUT);

    curl_easy_setopt(curl, CURLOPT_POST, 1);

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10);
    curl_easy_setopt(curl, CURLOPT_POSTREDIR, CURL_REDIR_POST_ALL);

    std::span<const char> data(buf);
    curl_easy_setopt(curl, CURLOPT_READDATA, &data);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());

    if (curl_easy_perform(curl) == CURLE_OK) {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code == 200) {
            succeeded = true;
        }
    }

    curl_slist_free_all(slist);
    curl_easy_cleanup(curl);

    return succeeded;
}

/*!
 * @brief キャラクタダンプを引数で指定した出力ストリームに書き込む
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param stream 書き込む出力ストリーム
 * @return エラーコード
 */
static errr make_dump(PlayerType *player_ptr, std::ostream &stream)
{
    FILE *fff;
    GAME_TEXT file_name[1024];

    /* Open a new file */
    fff = angband_fopen_temp(file_name, 1024);
    if (!fff) {
#ifdef JP
        msg_format("一時ファイル %s を作成できませんでした。", file_name);
#else
        msg_format("Failed to create temporary file %s.", file_name);
#endif
        msg_print(nullptr);
        return 1;
    }

    /* 一旦一時ファイルを作る。通常のダンプ出力と共通化するため。 */
    make_character_dump(player_ptr, fff);
    angband_fclose(fff);

    // 一時ファイルを削除する前に閉じるためブロックにする
    {
        std::ifstream ifs(file_name);
        stream << ifs.rdbuf();
    }

    fd_kill(file_name);

    /* Success */
    return 0;
}

/*!
 * @brief スクリーンダンプを作成する/ Make screen dump to buffer
 * @return 作成したスクリーンダンプの参照ポインタ
 */
concptr make_screen_dump(PlayerType *player_ptr)
{
    constexpr auto html_head =
        "<html>\n<body text=\"#ffffff\" bgcolor=\"#000000\">\n"
        "<pre>\n";
    constexpr auto html_foot =
        "</pre>\n"
        "</body>\n</html>\n";

    int wid, hgt;
    term_get_size(&wid, &hgt);

    std::stringstream screen_ss;

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    bool old_use_graphics = use_graphics;
    if (old_use_graphics) {
        /* Clear -more- prompt first */
        msg_print(nullptr);

        use_graphics = false;
        reset_visuals(player_ptr);

        static constexpr auto flags = {
            MainWindowRedrawingFlag::WIPE,
            MainWindowRedrawingFlag::BASIC,
            MainWindowRedrawingFlag::EXTRA,
            MainWindowRedrawingFlag::MAP,
            MainWindowRedrawingFlag::EQUIPPY,
        };
        rfu.set_flags(flags);
        handle_stuff(player_ptr);
    }

    screen_ss << html_head;

    /* Dump the screen */
    for (int y = 0; y < hgt; y++) {
        /* Start the row */
        if (y != 0) {
            screen_ss << '\n';
        }

        /* Dump each row */
        TERM_COLOR a = 0, old_a = 0;
        auto c = ' ';
        for (int x = 0; x < wid - 1; x++) {
            int rv, gv, bv;
            concptr cc = nullptr;
            /* Get the attr/char */
            (void)(term_what(x, y, &a, &c));

            switch (c) {
            case '&':
                cc = "&amp;";
                break;
            case '<':
                cc = "&lt;";
                break;
            case '>':
                cc = "&gt;";
                break;
            case '"':
                cc = "&quot;";
                break;
            case '\'':
                cc = "&#39;";
                break;
#ifdef WINDOWS
            case 0x1f:
                c = '.';
                break;
            case 0x7f:
                c = (a == 0x09) ? '%' : '#';
                break;
#endif
            }

            a = a & 0x0F;
            if ((y == 0 && x == 0) || a != old_a) {
                rv = angband_color_table[a][1];
                gv = angband_color_table[a][2];
                bv = angband_color_table[a][3];
                screen_ss << format("%s<font color=\"#%02x%02x%02x\">", ((y == 0 && x == 0) ? "" : "</font>"), rv, gv, bv);
                old_a = a;
            }

            if (cc) {
                screen_ss << cc;
            } else {
                screen_ss << c;
            }
        }
    }

    screen_ss << "</font>\n";

    screen_ss << html_foot;

    concptr ret;
    if (const auto screen_dump_size = screen_ss.tellp();
        (0 <= screen_dump_size) && (screen_dump_size < SCREEN_BUF_MAX_SIZE)) {
        ret = string_make(screen_ss.str().data());
    } else {
        ret = nullptr;
    }

    if (!old_use_graphics) {
        return ret;
    }

    use_graphics = true;
    reset_visuals(player_ptr);
    static constexpr auto flags = {
        MainWindowRedrawingFlag::WIPE,
        MainWindowRedrawingFlag::BASIC,
        MainWindowRedrawingFlag::EXTRA,
        MainWindowRedrawingFlag::MAP,
        MainWindowRedrawingFlag::EQUIPPY,
    };
    rfu.set_flags(flags);
    handle_stuff(player_ptr);
    return ret;
}

/*!
 * @brief スコア転送処理のメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 正常にスコアを送信できたらtrue、失敗時に送信を中止したらfalse
 */
bool report_score(PlayerType *player_ptr)
{
    std::stringstream score_ss;
    std::string personality_desc = ap_ptr->title;
    personality_desc.append(_(ap_ptr->no ? "の" : "", " "));

    auto realm1_name = PlayerClass(player_ptr).equals(PlayerClassType::ELEMENTALIST) ? get_element_title(player_ptr->element) : realm_names[player_ptr->realm1];
    score_ss << format("name: %s\n", player_ptr->name)
             << format("version: %s\n", get_version().data())
             << format("score: %ld\n", calc_score(player_ptr))
             << format("level: %d\n", player_ptr->lev)
             << format("depth: %d\n", player_ptr->current_floor_ptr->dun_level)
             << format("maxlv: %d\n", player_ptr->max_plv)
             << format("maxdp: %d\n", max_dlv[DUNGEON_ANGBAND])
             << format("au: %d\n", player_ptr->au)
             << format("turns: %d\n", turn_real(player_ptr, w_ptr->game_turn))
             << format("sex: %d\n", player_ptr->psex)
             << format("race: %s\n", rp_ptr->title)
             << format("class: %s\n", cp_ptr->title)
             << format("seikaku: %s\n", personality_desc.data())
             << format("realm1: %s\n", realm1_name)
             << format("realm2: %s\n", realm_names[player_ptr->realm2])
             << format("killer: %s\n", player_ptr->died_from.data())
             << "-----charcter dump-----\n";

    make_dump(player_ptr, score_ss);
    if (screen_dump) {
        score_ss << "-----screen shot-----\n"
                 << screen_dump;
    }

    const auto score = score_ss.str();
    const std::vector<char> score_data(score.begin(), score.end());

    term_clear();
    while (true) {
        term_fresh();
        prt(_("スコア送信中...", "Sending the score..."), 0, 0);
        term_fresh();
        if (http_post(SCORE_PATH, score_data)) {
            return true;
        }

        prt(_("スコア・サーバへの送信に失敗しました。", "Failed to send to the score server."), 0, 0);
        (void)inkey();
        if (get_check_strict(player_ptr, _("もう一度接続を試みますか? ", "Try again? "), CHECK_NO_HISTORY)) {
            continue;
        }

        return false;
    }
}
#else
concptr screen_dump = nullptr;
#endif /* WORLD_SCORE */
