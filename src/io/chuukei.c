/*!
    @file chuukei.c
    @brief 中継機能の実装
    @date 2014/01/02
    @author
    2014 Deskull rearranged comment for Doxygen.
 */

#include "io/chuukei.h"
#include "cmd-io/cmd-dump.h"
#include "cmd-visual/cmd-draw.h"
#include "core/asking-player.h"
#include "io/files-util.h"
#include "io/inet.h"
#include "io/signal-handlers.h"
#include "term/gameterm.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#ifdef JP
#include "locale/japanese.h"
#endif

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef WINDOWS
#include <windows.h>
#endif

#ifdef CHUUKEI
#if defined(WINDOWS)
#include <winsock.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#include <setjmp.h>
#include <signal.h>
#endif

#define MAX_HOSTNAME 256
#endif

#define RINGBUF_SIZE 1024 * 1024
#define FRESH_QUEUE_SIZE 4096
#ifdef WINDOWS
#define WAIT 100
#else
#define WAIT 100 * 1000 /* ブラウズ側のウエイト(us単位) */
#endif
#define DEFAULT_DELAY 50
#define RECVBUF_SIZE 1024

#ifdef CHUUKEI
bool chuukei_server;
bool chuukei_client;
char *server_name;
int server_port;
#endif

static long epoch_time; /* バッファ開始時刻 */
static int browse_delay; /* 表示するまでの時間(100ms単位)(この間にラグを吸収する) */
#ifdef CHUUKEI
static int sd; /* ソケットのファイルディスクリプタ */
static long time_diff; /* プレイ側との時間のずれ(これを見ながらディレイを調整していく) */
static int server_port;
static GAME_TEXT server_name[MAX_HOSTNAME];
#endif

static int movie_fd;
static int movie_mode;

#ifdef CHUUKEI
#ifdef WINDOWS
#define close closesocket
#endif
#endif

/* 描画する時刻を覚えておくキュー構造体 */
static struct {
    int time[FRESH_QUEUE_SIZE];
    int next;
    int tail;
} fresh_queue;

/* リングバッファ構造体 */
static struct {
    char *buf;
    int wptr;
    int rptr;
    int inlen;
} ring;

/*
 * Original hooks
 */
static errr (*old_xtra_hook)(int n, int v);
static errr (*old_curs_hook)(int x, int y);
static errr (*old_bigcurs_hook)(int x, int y);
static errr (*old_wipe_hook)(int x, int y, int n);
static errr (*old_text_hook)(int x, int y, int n, TERM_COLOR a, concptr s);

static void disable_chuukei_server(void)
{
    term_type *t = angband_term[0];
#ifdef CHUUKEI
    chuukei_server = FALSE;
#endif /* CHUUKEI */
    t->xtra_hook = old_xtra_hook;
    t->curs_hook = old_curs_hook;
    t->bigcurs_hook = old_bigcurs_hook;
    t->wipe_hook = old_wipe_hook;
    t->text_hook = old_text_hook;
}

/* ANSI Cによればstatic変数は0で初期化されるが一応初期化する */
static errr init_buffer(void)
{
    fresh_queue.next = fresh_queue.tail = 0;
    ring.wptr = ring.rptr = ring.inlen = 0;
    fresh_queue.time[0] = 0;
    ring.buf = malloc(RINGBUF_SIZE);
    if (ring.buf == NULL)
        return -1;

    return 0;
}

/* 現在の時間を100ms単位で取得する */
static long get_current_time(void)
{
#ifdef WINDOWS
    return timeGetTime() / 100;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (tv.tv_sec * 10 + tv.tv_usec / 100000);
#endif
}

/* リングバッファ構造体に buf の内容を加える */
static errr insert_ringbuf(char *buf)
{
    int len;
    len = strlen(buf) + 1; /* +1は終端文字分 */

    if (movie_mode) {
        fd_write(movie_fd, buf, len);
#ifdef CHUUKEI
        if (!chuukei_server)
            return 0;
#else
        return 0;
#endif
    }

    /* バッファをオーバー */
    if (ring.inlen + len >= RINGBUF_SIZE) {
#ifdef CHUUKEI
        if (chuukei_server)
            disable_chuukei_server();
        else
            chuukei_client = FALSE;

        prt("送受信バッファが溢れました。サーバとの接続を切断します。", 0, 0);
        inkey();

        close(sd);
#endif
        return -1;
    }

    /* バッファの終端までに収まる */
    if (ring.wptr + len < RINGBUF_SIZE) {
        memcpy(ring.buf + ring.wptr, buf, len);
        ring.wptr += len;
    }
    /* バッファの終端までに収まらない(ピッタリ収まる場合も含む) */
    else {
        int head = RINGBUF_SIZE - ring.wptr; /* 前半 */
        int tail = len - head; /* 後半 */

        memcpy(ring.buf + ring.wptr, buf, head);
        memcpy(ring.buf, buf + head, tail);
        ring.wptr = tail;
    }

    ring.inlen += len;

    /* Success */
    return 0;
}

#ifdef CHUUKEI
void flush_ringbuf(void)
{
    fd_set fdset;
    struct timeval tv;

    if (!chuukei_server)
        return;

    if (ring.inlen == 0)
        return;

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    FD_ZERO(&fdset);
    FD_SET(sd, &fdset);

    while (TRUE) {
        fd_set tmp_fdset;
        int result;

        tmp_fdset = fdset;

        /* ソケットにデータを書き込めるかどうか調べる */
        select(sd + 1, (fd_set *)NULL, &tmp_fdset, (fd_set *)NULL, &tv);

        /* 書き込めなければ戻る */
        if (FD_ISSET(sd, &tmp_fdset) == 0)
            break;

        result = send(sd, ring.buf + ring.rptr, ((ring.wptr > ring.rptr) ? ring.wptr : RINGBUF_SIZE) - ring.rptr, 0);

        if (result <= 0) {
            /* サーバとの接続断？ */
            if (chuukei_server)
                disable_chuukei_server();

            prt("サーバとの接続が切断されました。", 0, 0);
            inkey();
            close(sd);

            return;
        }

        ring.rptr += result;
        ring.inlen -= result;

        if (ring.rptr == RINGBUF_SIZE)
            ring.rptr = 0;
        if (ring.inlen == 0)
            break;
    }
}

static int read_chuukei_prf(concptr prf_name)
{
    char buf[1024];
    FILE *fp;

    path_build(buf, sizeof(buf), ANGBAND_DIR_XTRA, prf_name);
    fp = angband_fopen(buf, "r");

    if (!fp)
        return -1;

    /* 初期化 */
    server_port = -1;
    server_name[0] = 0;
    browse_delay = DEFAULT_DELAY;

    while (0 == angband_fgets(fp, buf, sizeof(buf))) {
        /* サーバ名 */
        if (!strncmp(buf, "server:", 7)) {
            strncpy(server_name, buf + 7, MAX_HOSTNAME - 1);
            server_name[MAX_HOSTNAME - 1] = '\0';
        }

        /* ポート番号 */
        if (!strncmp(buf, "port:", 5)) {
            server_port = atoi(buf + 5);
        }

        /* ディレイ */
        if (!strncmp(buf, "delay:", 6)) {
            browse_delay = atoi(buf + 6);
        }
    }

    angband_fclose(fp);

    /* prfファイルが完全でない */
    if (server_port == -1 || server_name[0] == 0)
        return -1;

    return 0;
}

int connect_chuukei_server(char *prf_name)
{
#ifdef WINDOWS
    WSADATA wsaData;
    WORD wVersionRequested = (WORD)((1) | (1 << 8));
#endif

    struct sockaddr_in ask;
    struct hostent *hp;

    if (read_chuukei_prf(prf_name) < 0) {
        printf("Wrong prf file\n");
        return -1;
    }

    if (init_buffer() < 0) {
        printf("Malloc error\n");
        return -1;
    }

#ifdef WINDOWS
    if (WSAStartup(wVersionRequested, &wsaData)) {
        msg_print("Report: WSAStartup failed.");
        return -1;
    }
#endif

    printf("server = %s\nport = %d\n", server_name, server_port);

    if ((hp = gethostbyname(server_name)) != NULL) {
        memset(&ask, 0, sizeof(ask));
        memcpy(&ask.sin_addr, hp->h_addr_list[0], hp->h_length);
    } else {
        if ((ask.sin_addr.s_addr = inet_addr(server_name)) == 0) {
            printf("Bad hostname\n");
            return -1;
        }
    }

    ask.sin_family = AF_INET;
    ask.sin_port = htons((unsigned short)server_port);

#ifndef WINDOWS
    if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
#else
    if ((sd = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
#endif
    {
        printf("Can't create socket\n");
        return -1;
    }

    if (connect(sd, (struct sockaddr *)&ask, sizeof(ask)) < 0) {
        close(sd);
        printf("Can't connect %s port %d\n", server_name, server_port);
        return -1;
    }

    return 0;
}
#endif /* CHUUKEI */

/* strが同じ文字の繰り返しかどうか調べる */
static bool string_is_repeat(char *str, int len)
{
    char c = str[0];
    int i;

    if (len < 2)
        return FALSE;
#ifdef JP
    if (iskanji(c))
        return FALSE;
#endif

    for (i = 1; i < len; i++) {
#ifdef JP
        if (c != str[i] || iskanji(str[i]))
            return FALSE;
#else
        if (c != str[i])
            return FALSE;
#endif
    }

    return TRUE;
}

static errr send_text_to_chuukei_server(TERM_LEN x, TERM_LEN y, int len, TERM_COLOR col, concptr str)
{
    char buf[1024];
    char buf2[1024];

    strncpy(buf2, str, len);
    buf2[len] = '\0';

    if (len == 1) {
        sprintf(buf, "s%c%c%c%c", x + 1, y + 1, col, buf2[0]);
    } else if (string_is_repeat(buf2, len)) {
        int i;
        for (i = len; i > 0; i -= 127) {
            sprintf(buf, "n%c%c%c%c%c", x + 1, y + 1, MIN(i, 127), col, buf2[0]);
        }
    } else {
#if defined(SJIS) && defined(JP)
        sjis2euc(buf2);
#endif
        sprintf(buf, "t%c%c%c%c%s", x + 1, y + 1, len, col, buf2);
    }

    insert_ringbuf(buf);

    return (*old_text_hook)(x, y, len, col, str);
}

static errr send_wipe_to_chuukei_server(int x, int y, int len)
{
    char buf[1024];

    sprintf(buf, "w%c%c%c", x + 1, y + 1, len);

    insert_ringbuf(buf);

    return (*old_wipe_hook)(x, y, len);
}

static errr send_xtra_to_chuukei_server(int n, int v)
{
    char buf[1024];

    if (n == TERM_XTRA_CLEAR || n == TERM_XTRA_FRESH || n == TERM_XTRA_SHAPE) {
        sprintf(buf, "x%c", n + 1);

        insert_ringbuf(buf);

        if (n == TERM_XTRA_FRESH) {
            sprintf(buf, "d%ld", get_current_time() - epoch_time);
            insert_ringbuf(buf);
        }
    }

    /* Verify the hook */
    if (!old_xtra_hook)
        return -1;

    return (*old_xtra_hook)(n, v);
}

static errr send_curs_to_chuukei_server(int x, int y)
{
    char buf[1024];

    sprintf(buf, "c%c%c", x + 1, y + 1);

    insert_ringbuf(buf);

    return (*old_curs_hook)(x, y);
}

static errr send_bigcurs_to_chuukei_server(int x, int y)
{
    char buf[1024];

    sprintf(buf, "C%c%c", x + 1, y + 1);

    insert_ringbuf(buf);

    return (*old_bigcurs_hook)(x, y);
}

/*
 * Prepare z-term hooks to call send_*_to_chuukei_server()'s
 */
void prepare_chuukei_hooks(void)
{
    term_type *t0 = angband_term[0];

    /* Save original z-term hooks */
    old_xtra_hook = t0->xtra_hook;
    old_curs_hook = t0->curs_hook;
    old_bigcurs_hook = t0->bigcurs_hook;
    old_wipe_hook = t0->wipe_hook;
    old_text_hook = t0->text_hook;

    /* Prepare z-term hooks */
    t0->xtra_hook = send_xtra_to_chuukei_server;
    t0->curs_hook = send_curs_to_chuukei_server;
    t0->bigcurs_hook = send_bigcurs_to_chuukei_server;
    t0->wipe_hook = send_wipe_to_chuukei_server;
    t0->text_hook = send_text_to_chuukei_server;
}

/*
 * Prepare z-term hooks to call send_*_to_chuukei_server()'s
 */
void prepare_movie_hooks(player_type *player_ptr)
{
    char buf[1024];
    char tmp[80];

    if (movie_mode) {
        movie_mode = 0;
#ifdef CHUUKEI
        if (!chuukei_server)
            disable_chuukei_server();
#else
        disable_chuukei_server();
#endif
        fd_close(movie_fd);
        msg_print(_("録画を終了しました。", "Stopped recording."));
    } else {
        sprintf(tmp, "%s.amv", player_ptr->base_name);
        if (get_string(_("ムービー記録ファイル: ", "Movie file name: "), tmp, 80)) {
            int fd;

            path_build(buf, sizeof(buf), ANGBAND_DIR_USER, tmp);

            fd = fd_open(buf, O_RDONLY);

            /* Existing file */
            if (fd >= 0) {
                char out_val[160];
                (void)fd_close(fd);

                /* Build query */
                (void)sprintf(out_val, _("現存するファイルに上書きしますか? (%s)", "Replace existing file %s? "), buf);

                /* Ask */
                if (!get_check(out_val))
                    return;

                movie_fd = fd_open(buf, O_WRONLY | O_TRUNC);
            } else {
                movie_fd = fd_make(buf, 0644);
            }

            if (!movie_fd) {
                msg_print(_("ファイルを開けません！", "Can not open file."));
                return;
            }

            movie_mode = 1;
#ifdef CHUUKEI
            if (!chuukei_server)
                prepare_chuukei_hooks();
#else
            prepare_chuukei_hooks();
#endif
            do_cmd_redraw(player_ptr);
        }
    }
}

#ifdef CHUUKEI
static int handle_timestamp_data(int timestamp)
{
    long current_time = get_current_time();

    /* 描画キューは空かどうか？ */
    if (fresh_queue.tail == fresh_queue.next) {
        /* バッファリングし始めの時間を保存しておく */
        epoch_time = current_time;
        epoch_time += browse_delay;
        epoch_time -= timestamp;
        time_diff = current_time - timestamp;
    }

    /* 描画キューに保存し、保存位置を進める */
    fresh_queue.time[fresh_queue.tail] = timestamp;
    fresh_queue.tail++;

    /* キューの最後尾に到達したら先頭に戻す */
    fresh_queue.tail %= FRESH_QUEUE_SIZE;

    if (fresh_queue.tail == fresh_queue.next) {
        /* 描画キュー溢れ */
        prt("描画タイミングキューが溢れました。サーバとの接続を切断します。", 0, 0);
        inkey();
        close(sd);

        return -1;
    }

    /* プレイ側とのディレイを調整 */
    if (time_diff != current_time - timestamp) {
        long old_time_diff = time_diff;
        time_diff = current_time - timestamp;
        epoch_time -= (old_time_diff - time_diff);
    }

    /* Success */
    return 0;
}
#endif /* CHUUKEI */

static int handle_movie_timestamp_data(int timestamp)
{
    static int initialized = FALSE;

    /* 描画キューは空かどうか？ */
    if (!initialized) {
        /* バッファリングし始めの時間を保存しておく */
        epoch_time = get_current_time();
        epoch_time += browse_delay;
        epoch_time -= timestamp;
        // time_diff = current_time - timestamp;
        initialized = TRUE;
    }

    /* 描画キューに保存し、保存位置を進める */
    fresh_queue.time[fresh_queue.tail] = timestamp;
    fresh_queue.tail++;

    /* キューの最後尾に到達したら先頭に戻す */
    fresh_queue.tail %= FRESH_QUEUE_SIZE;

    /* Success */
    return 0;
}

#ifdef CHUUKEI
static int read_sock(void)
{
    static char recv_buf[RECVBUF_SIZE];
    static int remain_bytes = 0;
    int recv_bytes;
    int i;

    /* 前回残ったデータの後につづけて配信サーバからデータ受信 */
    recv_bytes = recv(sd, recv_buf + remain_bytes, RECVBUF_SIZE - remain_bytes, 0);

    if (recv_bytes <= 0)
        return -1;

    /* 前回残ったデータ量に今回読んだデータ量を追加 */
    remain_bytes += recv_bytes;

    for (i = 0; i < remain_bytes; i++) {
        /* データのくぎり('\0')を探す */
        if (recv_buf[i] == '\0') {
            /* 'd'で始まるデータ(タイムスタンプ)の場合は
               描画キューに保存する処理を呼ぶ */
            if ((recv_buf[0] == 'd') && (handle_timestamp_data(atoi(recv_buf + 1)) < 0))
                return -1;

            /* 受信データを保存 */
            if (insert_ringbuf(recv_buf) < 0)
                return -1;

            /* 次のデータ移行をrecv_bufの先頭に移動 */
            memmove(recv_buf, recv_buf + i + 1, remain_bytes - i - 1);

            remain_bytes -= (i + 1);
            i = 0;
        }
    }

    return 0;
}
#endif

static int read_movie_file(void)
{
    static char recv_buf[RECVBUF_SIZE];
    static int remain_bytes = 0;
    int recv_bytes;
    int i;

    recv_bytes = read(movie_fd, recv_buf + remain_bytes, RECVBUF_SIZE - remain_bytes);

    if (recv_bytes <= 0)
        return -1;

    /* 前回残ったデータ量に今回読んだデータ量を追加 */
    remain_bytes += recv_bytes;

    for (i = 0; i < remain_bytes; i++) {
        /* データのくぎり('\0')を探す */
        if (recv_buf[i] == '\0') {
            /* 'd'で始まるデータ(タイムスタンプ)の場合は
               描画キューに保存する処理を呼ぶ */
            if ((recv_buf[0] == 'd') && (handle_movie_timestamp_data(atoi(recv_buf + 1)) < 0))
                return -1;

            /* 受信データを保存 */
            if (insert_ringbuf(recv_buf) < 0)
                return -1;

            /* 次のデータ移行をrecv_bufの先頭に移動 */
            memmove(recv_buf, recv_buf + i + 1, remain_bytes - i - 1);

            remain_bytes -= (i + 1);
            i = 0;
        }
    }

    return 0;
}

#ifndef WINDOWS
/* Win版の床の中点と壁の豆腐をピリオドとシャープにする。*/
static void win2unix(int col, char *buf)
{
    char wall;
    if (col == 9)
        wall = '%';
    else
        wall = '#';

    while (*buf) {
#ifdef JP
        if (iskanji(*buf)) {
            buf += 2;
            continue;
        }
#endif
        if (*buf == 127)
            *buf = wall;
        else if (*buf == 31)
            *buf = '.';
        buf++;
    }
}
#endif

static bool get_nextbuf(char *buf)
{
    char *ptr = buf;

    while (TRUE) {
        *ptr = ring.buf[ring.rptr++];
        ring.inlen--;
        if (ring.rptr == RINGBUF_SIZE)
            ring.rptr = 0;
        if (*ptr++ == '\0')
            break;
    }

    if (buf[0] == 'd')
        return FALSE;

    return TRUE;
}

/* プレイホストのマップが大きいときクライアントのマップもリサイズする */
static void update_term_size(int x, int y, int len)
{
    int ox, oy;
    int nx, ny;
    term_get_size(&ox, &oy);
    nx = ox;
    ny = oy;

    /* 横方向のチェック */
    if (x + len > ox)
        nx = x + len;
    /* 縦方向のチェック */
    if (y + 1 > oy)
        ny = y + 1;

    if (nx != ox || ny != oy)
        term_resize(nx, ny);
}

static bool flush_ringbuf_client(void)
{
    char buf[1024];

    /* 書くデータなし */
    if (fresh_queue.next == fresh_queue.tail)
        return FALSE;

    /* まだ書くべき時でない */
    if (fresh_queue.time[fresh_queue.next] > get_current_time() - epoch_time)
        return FALSE;

    /* 時間情報(区切り)が得られるまで書く */
    while (get_nextbuf(buf)) {
        char id;
        int x, y, len;
        TERM_COLOR col;
        int i;
        unsigned char tmp1, tmp2, tmp3, tmp4;
        char *mesg;

        sscanf(buf, "%c%c%c%c%c", &id, &tmp1, &tmp2, &tmp3, &tmp4);
        x = tmp1 - 1;
        y = tmp2 - 1;
        len = tmp3;
        col = tmp4;
        if (id == 's') {
            col = tmp3;
            mesg = &buf[4];
        } else
            mesg = &buf[5];
#ifndef WINDOWS
        win2unix(col, mesg);
#endif

        switch (id) {
        case 't': /* 通常 */
#if defined(SJIS) && defined(JP)
            euc2sjis(mesg);
#endif
            update_term_size(x, y, len);
            (void)((*angband_term[0]->text_hook)(x, y, len, (byte)col, mesg));
            strncpy(&Term->scr->c[y][x], mesg, len);
            for (i = x; i < x + len; i++) {
                Term->scr->a[y][i] = col;
            }
            break;

        case 'n': /* 繰り返し */
            for (i = 1; i < len; i++) {
                mesg[i] = mesg[0];
            }
            mesg[i] = '\0';
            update_term_size(x, y, len);
            (void)((*angband_term[0]->text_hook)(x, y, len, (byte)col, mesg));
            strncpy(&Term->scr->c[y][x], mesg, len);
            for (i = x; i < x + len; i++) {
                Term->scr->a[y][i] = col;
            }
            break;

        case 's': /* 一文字 */
            update_term_size(x, y, 1);
            (void)((*angband_term[0]->text_hook)(x, y, 1, (byte)col, mesg));
            strncpy(&Term->scr->c[y][x], mesg, 1);
            Term->scr->a[y][x] = col;
            break;

        case 'w':
            update_term_size(x, y, len);
            (void)((*angband_term[0]->wipe_hook)(x, y, len));
            break;

        case 'x':
            if (x == TERM_XTRA_CLEAR)
                term_clear();
            (void)((*angband_term[0]->xtra_hook)(x, 0));
            break;

        case 'c':
            update_term_size(x, y, 1);
            (void)((*angband_term[0]->curs_hook)(x, y));
            break;
        case 'C':
            update_term_size(x, y, 1);
            (void)((*angband_term[0]->bigcurs_hook)(x, y));
            break;
        }
    }

    fresh_queue.next++;
    if (fresh_queue.next == FRESH_QUEUE_SIZE)
        fresh_queue.next = 0;
    return TRUE;
}

#ifdef CHUUKEI
void browse_chuukei()
{
    fd_set fdset;
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = WAIT;

    FD_ZERO(&fdset);
    FD_SET(sd, &fdset);

    term_clear();
    term_fresh();
    term_xtra(TERM_XTRA_REACT, 0);

    while (TRUE) {
        fd_set tmp_fdset;
        struct timeval tmp_tv;

        if (flush_ringbuf_client())
            continue;

        tmp_fdset = fdset;
        tmp_tv = tv;

        /* ソケットにデータが来ているかどうか調べる */
        select(sd + 1, &tmp_fdset, (fd_set *)NULL, (fd_set *)NULL, &tmp_tv);
        if (FD_ISSET(sd, &tmp_fdset) == 0) {
            term_xtra(TERM_XTRA_FLUSH, 0);
            continue;
        }

        if (read_sock() < 0) {
            chuukei_client = FALSE;
        }

        /* 接続が切れた状態で書くべきデータがなくなっていたら終了 */
        if (!chuukei_client && fresh_queue.next == fresh_queue.tail)
            break;
    }
}
#endif /* CHUUKEI */

void prepare_browse_movie_aux(concptr filename)
{
    movie_fd = fd_open(filename, O_RDONLY);

    browsing_movie = TRUE;

    init_buffer();
}

void prepare_browse_movie(concptr filename)
{
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_USER, filename);

    prepare_browse_movie_aux(buf);
}

void browse_movie(void)
{
    term_clear();
    term_fresh();
    term_xtra(TERM_XTRA_REACT, 0);

    while (read_movie_file() == 0) {
        while (fresh_queue.next != fresh_queue.tail) {
            if (!flush_ringbuf_client()) {
                term_xtra(TERM_XTRA_FLUSH, 0);

                /* ソケットにデータが来ているかどうか調べる */
#ifdef WINDOWS
                Sleep(WAIT);
#else
                usleep(WAIT);
#endif
            }
        }
    }
}
