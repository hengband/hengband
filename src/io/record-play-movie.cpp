/*!
 * @brief 録画・再生機能
 * @date 2014/01/02
 * @author 2014 Deskull rearranged comment for Doxygen.
 */

#include "io/record-play-movie.h"
#include "cmd-io/cmd-dump.h"
#include "cmd-visual/cmd-draw.h"
#include "core/asking-player.h"
#include "io/files-util.h"
#include "io/inet.h"
#include "io/signal-handlers.h"
#include "locale/japanese.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include <algorithm>

#ifdef WINDOWS
#include <windows.h>
#define WAIT 100
#else
#include "system/h-basic.h"
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#define WAIT 100 * 1000 /* ブラウズ側のウエイト(us単位) */
#endif

#define RINGBUF_SIZE 1024 * 1024
#define FRESH_QUEUE_SIZE 4096
#define DEFAULT_DELAY 50
#define RECVBUF_SIZE 1024

static long epoch_time; /* バッファ開始時刻 */
static int browse_delay; /* 表示するまでの時間(100ms単位)(この間にラグを吸収する) */
static int movie_fd;
static int movie_mode;

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
    ring.buf = static_cast<char *>(malloc(RINGBUF_SIZE));
    if (ring.buf == nullptr)
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
    gettimeofday(&tv, nullptr);

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
        return 0;
    }

    /* バッファをオーバー */
    if (ring.inlen + len >= RINGBUF_SIZE)
        return -1;

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

/* strが同じ文字の繰り返しかどうか調べる */
static bool string_is_repeat(char *str, int len)
{
    char c = str[0];
    int i;

    if (len < 2)
        return false;
#ifdef JP
    if (iskanji(c))
        return false;
#endif

    for (i = 1; i < len; i++) {
#ifdef JP
        if (c != str[i] || iskanji(str[i]))
            return false;
#else
        if (c != str[i])
            return false;
#endif
    }

    return true;
}

static errr send_text_to_chuukei_server(TERM_LEN x, TERM_LEN y, int len, TERM_COLOR col, concptr str)
{
    char buf[1024 + 32];
    char buf2[1024];

    strncpy(buf2, str, len);
    buf2[len] = '\0';

    if (len == 1) {
        sprintf(buf, "s%c%c%c%c", x + 1, y + 1, col, buf2[0]);
    } else if (string_is_repeat(buf2, len)) {
        int i;
        for (i = len; i > 0; i -= 127) {
            sprintf(buf, "n%c%c%c%c%c", x + 1, y + 1, std::min<int>(i, 127), col, buf2[0]);
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
void prepare_movie_hooks(PlayerType *player_ptr)
{
    char buf[1024];
    char tmp[80];

    if (movie_mode) {
        movie_mode = 0;
        disable_chuukei_server();
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
                char out_val[sizeof(buf) + 128];
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
            prepare_chuukei_hooks();
            do_cmd_redraw(player_ptr);
        }
    }
}

static int handle_movie_timestamp_data(int timestamp)
{
    static int initialized = false;

    /* 描画キューは空かどうか？ */
    if (!initialized) {
        /* バッファリングし始めの時間を保存しておく */
        epoch_time = get_current_time();
        epoch_time += browse_delay;
        epoch_time -= timestamp;
        // time_diff = current_time - timestamp;
        initialized = true;
    }

    /* 描画キューに保存し、保存位置を進める */
    fresh_queue.time[fresh_queue.tail] = timestamp;
    fresh_queue.tail++;

    /* キューの最後尾に到達したら先頭に戻す */
    fresh_queue.tail %= FRESH_QUEUE_SIZE;

    /* Success */
    return 0;
}

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

    while (true) {
        *ptr = ring.buf[ring.rptr++];
        ring.inlen--;
        if (ring.rptr == RINGBUF_SIZE)
            ring.rptr = 0;
        if (*ptr++ == '\0')
            break;
    }

    if (buf[0] == 'd')
        return false;

    return true;
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
        return false;

    /* まだ書くべき時でない */
    if (fresh_queue.time[fresh_queue.next] > get_current_time() - epoch_time)
        return false;

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
            memcpy(&Term->scr->c[y][x], mesg, len);
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
            memcpy(&Term->scr->c[y][x], mesg, len);
            for (i = x; i < x + len; i++) {
                Term->scr->a[y][i] = col;
            }
            break;

        case 's': /* 一文字 */
            update_term_size(x, y, 1);
            (void)((*angband_term[0]->text_hook)(x, y, 1, (byte)col, mesg));
            memcpy(&Term->scr->c[y][x], mesg, 1);
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
    return true;
}

void prepare_browse_movie_without_path_build(concptr filename)
{
    movie_fd = fd_open(filename, O_RDONLY);
    init_buffer();
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

#ifndef WINDOWS
void prepare_browse_movie_with_path_build(concptr filename)
{
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_USER, filename);
    movie_fd = fd_open(buf, O_RDONLY);
    init_buffer();
}
#endif
