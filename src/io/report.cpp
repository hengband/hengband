/*!
 * @file report.c
 * @brief スコアサーバ転送機能の実装
 * @date 2014/07/14
 * @author Hengband Team
 */

#include "io/report.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "core/stuff-handler.h"
#include "core/turn-compensator.h"
#include "core/visuals-reseter.h"
#include "dungeon/dungeon.h"
#include "game-option/special-options.h"
#include "io-dump/character-dump.h"
#include "io/inet.h"
#include "io/input-key-acceptor.h"
#include "mind/mind-elementalist.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/player-personality.h"
#include "player/player-status.h"
#include "realm/realm-names-table.h"
#include "system/angband-version.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "world/world.h"

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

/*
 * simple buffer library
 */
typedef struct {
    size_t max_size;
    size_t size;
    size_t read_head;
    char *data;
} BUF;

#define BUFSIZE (65536) /*!< スコアサーバ転送バッファサイズ */

/*!
 * @brief 転送用バッファの確保
 * @return 確保したバッファの参照ポインタ
 */
static BUF *buf_new(void)
{
    BUF *p;
    p = static_cast<BUF *>(malloc(sizeof(BUF)));
    if (!p)
        return nullptr;

    p->size = 0;
    p->max_size = BUFSIZE;
    p->data = static_cast<char *>(malloc(BUFSIZE));
    if (!p->data) {
        free(p);
        return nullptr;
    }

    return p;
}

/*!
 * @brief 転送用バッファの解放
 * @param b 解放するバッファの参照ポインタ
 */
static void buf_delete(BUF *b)
{
    free(b->data);
    free(b);
}

/*!
 * @brief 転送用バッファにデータを追加する
 * @param buf 追加先バッファの参照ポインタ
 * @param data 追加元データ
 * @param size 追加サイズ
 * @return 追加後のバッファ容量
 */
static int buf_append(BUF *buf, concptr data, size_t size)
{
    while (buf->size + size > buf->max_size) {
        char *tmp;
        if ((tmp = static_cast<char *>(malloc(buf->max_size * 2))) == nullptr)
            return -1;

        memcpy(tmp, buf->data, buf->max_size);
        free(buf->data);

        buf->data = tmp;

        buf->max_size *= 2;
    }
    memcpy(buf->data + buf->size, data, size);
    buf->size += size;

    return buf->size;
}

/*!
 * @brief 転送用バッファにフォーマット指定した文字列データを追加する
 * @param buf 追加先バッファの参照ポインタ
 * @param fmt 文字列フォーマット
 * @return 追加後のバッファ容量
 */
static int buf_sprintf(BUF *buf, concptr fmt, ...)
{
    int ret;
    char tmpbuf[8192];
    va_list ap;

    va_start(ap, fmt);
#if defined(HAVE_VSNPRINTF)
    ret = vsnprintf(tmpbuf, sizeof(tmpbuf), fmt, ap);
#else
    ret = vsprintf(tmpbuf, fmt, ap);
#endif
    va_end(ap);

    if (ret < 0)
        return -1;

    ret = buf_append(buf, tmpbuf, strlen(tmpbuf));
    return ret;
}

size_t read_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    BUF *buf = static_cast<BUF *>(userdata);
    const size_t remain = buf->size - buf->read_head;
    const size_t copy_size = std::min<size_t>(size * nitems, remain);

    strncpy(buffer, buf->data + buf->read_head, copy_size);
    buf->read_head += copy_size;

    return copy_size;
}

/*!
 * @brief HTTPによるダンプ内容伝送
 * @param url 伝送先URL
 * @param buf 伝送内容バッファ
 * @return 送信に成功した場合TRUE、失敗した場合FALSE
 */
static bool http_post(concptr url, BUF *buf)
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
    snprintf(user_agent, sizeof(user_agent), "Hengband %d.%d.%d", FAKE_VER_MAJOR - 10, FAKE_VER_MINOR, FAKE_VER_PATCH);
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

    buf->read_head = 0;
    curl_easy_setopt(curl, CURLOPT_READDATA, buf);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, buf->size);

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
 * @brief キャラクタダンプを作って BUFに保存
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dumpbuf 伝送内容バッファ
 * @return エラーコード
 */
static errr make_dump(PlayerType *player_ptr, BUF *dumpbuf)
{
    char buf[1024];
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

    /* Open for read */
    fff = angband_fopen(file_name, "r");

    while (fgets(buf, 1024, fff)) {
        (void)buf_sprintf(dumpbuf, "%s", buf);
    }
    angband_fclose(fff);
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
    static concptr html_head[] = {
        "<html>\n<body text=\"#ffffff\" bgcolor=\"#000000\">\n",
        "<pre>",
        0,
    };
    static concptr html_foot[] = {
        "</pre>\n",
        "</body>\n</html>\n",
        0,
    };

    int wid, hgt;
    term_get_size(&wid, &hgt);

    /* Alloc buffer */
    BUF *screen_buf;
    screen_buf = buf_new();
    if (screen_buf == nullptr)
        return nullptr;

    bool old_use_graphics = use_graphics;
    if (old_use_graphics) {
        /* Clear -more- prompt first */
        msg_print(nullptr);

        use_graphics = false;
        reset_visuals(player_ptr);

        player_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY);
        handle_stuff(player_ptr);
    }

    for (int i = 0; html_head[i]; i++)
        buf_sprintf(screen_buf, html_head[i]);

    /* Dump the screen */
    for (int y = 0; y < hgt; y++) {
        /* Start the row */
        if (y != 0)
            buf_sprintf(screen_buf, "\n");

        /* Dump each row */
        TERM_COLOR a = 0, old_a = 0;
        SYMBOL_CODE c = ' ';
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
                buf_sprintf(screen_buf, "%s<font color=\"#%02x%02x%02x\">", ((y == 0 && x == 0) ? "" : "</font>"), rv, gv, bv);
                old_a = a;
            }

            if (cc)
                buf_sprintf(screen_buf, "%s", cc);
            else
                buf_sprintf(screen_buf, "%c", c);
        }
    }

    buf_sprintf(screen_buf, "</font>");

    for (int i = 0; html_foot[i]; i++)
        buf_sprintf(screen_buf, html_foot[i]);

    /* Screen dump size is too big ? */
    concptr ret;
    if (screen_buf->size + 1 > SCREEN_BUF_MAX_SIZE) {
        ret = nullptr;
    } else {
        /* Terminate string */
        buf_append(screen_buf, "", 1);

        ret = string_make(screen_buf->data);
    }

    /* Free buffer */
    buf_delete(screen_buf);

    if (!old_use_graphics)
        return ret;

    use_graphics = true;
    reset_visuals(player_ptr);

    player_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY);
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
    auto *score = buf_new();
    char personality_desc[128];
    char title[128];
    put_version(title);
#ifdef JP
    sprintf(personality_desc, "%s%s", ap_ptr->title, (ap_ptr->no ? "の" : ""));
#else
    sprintf(personality_desc, "%s ", ap_ptr->title);
#endif

    auto realm1_name = player_ptr->pclass == PlayerClassType::ELEMENTALIST ? get_element_title(player_ptr->element) : realm_names[player_ptr->realm1];
    buf_sprintf(score, "name: %s\n", player_ptr->name);
    buf_sprintf(score, "version: %s\n", title);
    buf_sprintf(score, "score: %d\n", calc_score(player_ptr));
    buf_sprintf(score, "level: %d\n", player_ptr->lev);
    buf_sprintf(score, "depth: %d\n", player_ptr->current_floor_ptr->dun_level);
    buf_sprintf(score, "maxlv: %d\n", player_ptr->max_plv);
    buf_sprintf(score, "maxdp: %d\n", max_dlv[DUNGEON_ANGBAND]);
    buf_sprintf(score, "au: %d\n", player_ptr->au);
    buf_sprintf(score, "turns: %d\n", turn_real(player_ptr, w_ptr->game_turn));
    buf_sprintf(score, "sex: %d\n", player_ptr->psex);
    buf_sprintf(score, "race: %s\n", rp_ptr->title);
    buf_sprintf(score, "class: %s\n", cp_ptr->title);
    buf_sprintf(score, "seikaku: %s\n", personality_desc);
    buf_sprintf(score, "realm1: %s\n", realm1_name);
    buf_sprintf(score, "realm2: %s\n", realm_names[player_ptr->realm2]);
    buf_sprintf(score, "killer: %s\n", player_ptr->died_from);
    buf_sprintf(score, "-----charcter dump-----\n");

    make_dump(player_ptr, score);
    if (screen_dump) {
        buf_sprintf(score, "-----screen shot-----\n");
        buf_append(score, screen_dump, strlen(screen_dump));
    }

    term_clear();
    while (true) {
        term_fresh();
        prt(_("スコア送信中...", "Sending the score..."), 0, 0);
        term_fresh();
        if (http_post(SCORE_PATH, score)) {
            buf_delete(score);
            return true;
        }

        prt(_("スコア・サーバへの送信に失敗しました。", "Failed to send to the score server."), 0, 0);
        (void)inkey();
        if (get_check_strict(player_ptr, _("もう一度接続を試みますか? ", "Try again? "), CHECK_NO_HISTORY)) {
            continue;
        }

        buf_delete(score);
        return false;
    }
}
#else
concptr screen_dump = nullptr;
#endif /* WORLD_SCORE */
