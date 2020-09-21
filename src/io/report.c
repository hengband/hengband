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
#include "player/player-class.h"
#include "player/player-personality.h"
#include "player/player-race.h"
#include "realm/realm-names-table.h"
#include "system/angband-version.h"
#include "system/floor-type-definition.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "world/world.h"

#ifdef WORLD_SCORE
#ifdef WINDOWS
#include <winsock.h>
#else
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#include <setjmp.h>
#include <signal.h>
#endif

concptr screen_dump = NULL;

/*
 * internet resource value
 */
#define HTTP_PROXY "" /*!< デフォルトのプロキシURL / Default proxy url */
#define HTTP_PROXY_PORT 0 /*!< デフォルトのプロキシポート / Default proxy port */
#define HTTP_TIMEOUT 20 /*!< デフォルトのタイムアウト時間(秒) / Timeout length (second) */
#define SCORE_SERVER "hengband.osdn.jp" /*!< デフォルトのスコアサーバURL / Default score server url */
#define SCORE_PORT 80 /*!< デフォルトのスコアサーバポート / Default score server port */

#ifdef JP
#define SCORE_PATH "http://hengband.osdn.jp/score/register_score.php" /*!< スコア開示URL */
#else
#define SCORE_PATH "http://moon.kmc.gr.jp/hengband/hengscore-en/score.cgi" /*!< スコア開示URL */
#endif

/*
 * simple buffer library
 */
typedef struct {
    size_t max_size;
    size_t size;
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
    p = malloc(sizeof(BUF));
    if (!p)
        return NULL;

    p->size = 0;
    p->max_size = BUFSIZE;
    p->data = malloc(BUFSIZE);
    if (!p->data) {
        free(p);
        return NULL;
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
        if ((tmp = malloc(buf->max_size * 2)) == NULL)
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

/*!
 * @brief HTTPによるダンプ内容伝送
 * @param sd ソケットID
 * @param url 伝送先URL
 * @param buf 伝送内容バッファ
 * @return なし
 */
static bool http_post(int sd, concptr url, BUF *buf)
{
    BUF *output;
    char response_buf[1024] = "";
    concptr HTTP_RESPONSE_CODE_OK = "HTTP/1.1 200 OK";

    output = buf_new();
    buf_sprintf(output, "POST %s HTTP/1.0\r\n", url);
    buf_sprintf(output, "User-Agent: Hengband %d.%d.%d\r\n", FAKE_VER_MAJOR - 10, FAKE_VER_MINOR, FAKE_VER_PATCH);

    buf_sprintf(output, "Content-Length: %d\r\n", buf->size);
    buf_sprintf(output, "Content-Encoding: binary\r\n");
#ifdef JP
#ifdef SJIS
    buf_sprintf(output, "Content-Type: text/plain; charset=SHIFT_JIS\r\n");
#endif
#ifdef EUC
    buf_sprintf(output, "Content-Type: text/plain; charset=EUC-JP\r\n");
#endif
#else
    buf_sprintf(output, "Content-Type: text/plain; charset=ASCII\r\n");
#endif
    buf_sprintf(output, "\r\n");
    buf_append(output, buf->data, buf->size);

    soc_write(sd, output->data, output->size);

    soc_read(sd, response_buf, sizeof(response_buf));

    return strncmp(response_buf, HTTP_RESPONSE_CODE_OK, strlen(HTTP_RESPONSE_CODE_OK)) == 0;
}

/*!
 * @brief キャラクタダンプを作って BUFに保存
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param dumpbuf 伝送内容バッファ
 * @return エラーコード
 */
static errr make_dump(player_type *creature_ptr, BUF *dumpbuf, void (*update_playtime)(void), display_player_pf display_player)
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
        msg_print(NULL);
        return 1;
    }

    /* 一旦一時ファイルを作る。通常のダンプ出力と共通化するため。 */
    make_character_dump(creature_ptr, fff, update_playtime, display_player);
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
concptr make_screen_dump(player_type *creature_ptr, void (*process_autopick_file_command)(char *))
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
    if (screen_buf == NULL)
        return (NULL);

    bool old_use_graphics = use_graphics;
    if (old_use_graphics) {
        /* Clear -more- prompt first */
        msg_print(NULL);

        use_graphics = FALSE;
        reset_visuals(creature_ptr, process_autopick_file_command);

        creature_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY);
        handle_stuff(creature_ptr);
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
            concptr cc = NULL;
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
        ret = NULL;
    } else {
        /* Terminate string */
        buf_append(screen_buf, "", 1);

        ret = string_make(screen_buf->data);
    }

    /* Free buffer */
    buf_delete(screen_buf);

    if (!old_use_graphics)
        return ret;

    use_graphics = TRUE;
    reset_visuals(creature_ptr, process_autopick_file_command);

    creature_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY);
    handle_stuff(creature_ptr);
    return ret;
}

/*!
 * todo メッセージは言語選択の関数マクロで何とかならんか？
 * @brief スコア転送処理のメインルーチン
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return 正常終了の時0、異常があったら1
 */
errr report_score(player_type *creature_ptr, void (*update_playtime)(void), display_player_pf display_player)
{
#ifdef WINDOWS
    WSADATA wsaData;
    WORD wVersionRequested = (WORD)((1) | (1 << 8));
#endif

    BUF *score;
    score = buf_new();

    char seikakutmp[128];
#ifdef JP
    sprintf(seikakutmp, "%s%s", ap_ptr->title, (ap_ptr->no ? "の" : ""));
#else
    sprintf(seikakutmp, "%s ", ap_ptr->title);
#endif

    buf_sprintf(score, "name: %s\n", creature_ptr->name);
#ifdef JP
    buf_sprintf(score, "version: 変愚蛮怒 %d.%d.%d\n", FAKE_VER_MAJOR - 10, FAKE_VER_MINOR, FAKE_VER_PATCH);
#else
    buf_sprintf(score, "version: Hengband %d.%d.%d\n", FAKE_VER_MAJOR - 10, FAKE_VER_MINOR, FAKE_VER_PATCH);
#endif
    buf_sprintf(score, "score: %d\n", calc_score(creature_ptr));
    buf_sprintf(score, "level: %d\n", creature_ptr->lev);
    buf_sprintf(score, "depth: %d\n", creature_ptr->current_floor_ptr->dun_level);
    buf_sprintf(score, "maxlv: %d\n", creature_ptr->max_plv);
    buf_sprintf(score, "maxdp: %d\n", max_dlv[DUNGEON_ANGBAND]);
    buf_sprintf(score, "au: %d\n", creature_ptr->au);
    buf_sprintf(score, "turns: %d\n", turn_real(creature_ptr, current_world_ptr->game_turn));
    buf_sprintf(score, "sex: %d\n", creature_ptr->psex);
    buf_sprintf(score, "race: %s\n", rp_ptr->title);
    buf_sprintf(score, "class: %s\n", cp_ptr->title);
    buf_sprintf(score, "seikaku: %s\n", seikakutmp);
    buf_sprintf(score, "realm1: %s\n", realm_names[creature_ptr->realm1]);
    buf_sprintf(score, "realm2: %s\n", realm_names[creature_ptr->realm2]);
    buf_sprintf(score, "killer: %s\n", creature_ptr->died_from);
    buf_sprintf(score, "-----charcter dump-----\n");

    make_dump(creature_ptr, score, update_playtime, display_player);

    if (screen_dump) {
        buf_sprintf(score, "-----screen shot-----\n");
        buf_append(score, screen_dump, strlen(screen_dump));
    }

#ifdef WINDOWS
    if (WSAStartup(wVersionRequested, &wsaData)) {
        msg_print("Report: WSAStartup failed.");
#ifdef WINDOWS
        WSACleanup();
#endif
        return 1;
    }
#endif

    term_clear();

    int sd;
    while (TRUE) {
        char buff[160];
#ifdef JP
        prt("接続中...", 0, 0);
#else
        prt("connecting...", 0, 0);
#endif
        term_fresh();

        /* プロキシを設定する */
        set_proxy(HTTP_PROXY, HTTP_PROXY_PORT);

        /* Connect to the score server */
        sd = connect_server(HTTP_TIMEOUT, SCORE_SERVER, SCORE_PORT);

        if (sd < 0) {
#ifdef JP
            sprintf(buff, "スコア・サーバへの接続に失敗しました。(%s)", soc_err());
#else
            sprintf(buff, "Failed to connect to the score server.(%s)", soc_err());
#endif
            prt(buff, 0, 0);
            (void)inkey();

#ifdef JP
            if (!get_check_strict(creature_ptr, "もう一度接続を試みますか? ", CHECK_NO_HISTORY))
#else
            if (!get_check_strict(creature_ptr, "Try again? ", CHECK_NO_HISTORY))
#endif
            {
#ifdef WINDOWS
                WSACleanup();
#endif
                return 1;
            }

            continue;
        }

#ifdef JP
        prt("スコア送信中...", 0, 0);
#else
        prt("Sending the score...", 0, 0);
#endif
        term_fresh();

        if (!http_post(sd, SCORE_PATH, score)) {
            disconnect_server(sd);
#ifdef JP
            sprintf(buff, "スコア・サーバへの送信に失敗しました。");
#else
            sprintf(buff, "Failed to send to the score server.");
#endif
            prt(buff, 0, 0);
            (void)inkey();

#ifdef JP
            if (!get_check_strict(creature_ptr, "もう一度接続を試みますか? ", CHECK_NO_HISTORY))
#else
            if (!get_check_strict(creature_ptr, "Try again? ", CHECK_NO_HISTORY))
#endif
            {
#ifdef WINDOWS
                WSACleanup();
#endif
                return 1;
            }

            continue;
        }

        disconnect_server(sd);
        break;
    }

#ifdef WINDOWS
    WSACleanup();
#endif

    return 0;
}
#endif /* WORLD_SCORE */
