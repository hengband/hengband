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
#include "io/signal-handlers.h"
#include "locale/japanese.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/z-form.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include <algorithm>
#include <sstream>
#include <vector>

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
/* 「n」、「t」、および「w」コマンドでは、長さが「signed char」に配置されるときに負の値を回避するために、これよりも長い長さを使用しないでください。 */
static constexpr auto SPLIT_MAX = 127;

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
    std::vector<char> buf{};
    int wptr = 0;
    int rptr = 0;
    int inlen = 0;
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
    term_type *t = angband_terms[0];
    t->xtra_hook = old_xtra_hook;
    t->curs_hook = old_curs_hook;
    t->bigcurs_hook = old_bigcurs_hook;
    t->wipe_hook = old_wipe_hook;
    t->text_hook = old_text_hook;
}

/* ANSI Cによればstatic変数は0で初期化されるが一応初期化する */
static void init_buffer(void)
{
    fresh_queue.next = fresh_queue.tail = 0;
    ring.wptr = ring.rptr = ring.inlen = 0;
    fresh_queue.time[0] = 0;
    ring.buf.resize(RINGBUF_SIZE);
}

/* 現在の時間を100ms単位で取得する */
static long get_current_time(void)
{
#ifdef WINDOWS
    return timeGetTime() / 100;
#else
    struct timeval tv;
    gettimeofday(&tv, nullptr);

    return tv.tv_sec * 10 + tv.tv_usec / 100000;
#endif
}

/*!
 * @brief リングバッファにヘッダとペイロードを追加する
 * @param header ヘッダ
 * @param payload ペイロード (オプション)
 * @return エラーコード
 */
static errr insert_ringbuf(std::string_view header, std::string_view payload = "")
{
    if (movie_mode) {
        fd_write(movie_fd, header.data(), header.length());
        if (!payload.empty()) {
            fd_write(movie_fd, payload.data(), payload.length());
        }
        fd_write(movie_fd, "", 1);
        return 0;
    }

    /* バッファをオーバー */
    auto all_length = header.length() + payload.length();
    if (ring.inlen + all_length + 1 >= RINGBUF_SIZE) {
        return -1;
    }

    /* バッファの終端までに収まる */
    if (ring.wptr + all_length + 1 < RINGBUF_SIZE) {
        std::copy_n(header.begin(), header.length(), ring.buf.begin() + ring.wptr);
        if (!payload.empty()) {
            std::copy_n(payload.begin(), payload.length(), ring.buf.begin() + ring.wptr + header.length());
        }
        ring.buf[ring.wptr + all_length] = '\0';
        ring.wptr += all_length + 1;
    }
    /* バッファの終端までに収まらない(ピッタリ収まる場合も含む) */
    else {
        int head = RINGBUF_SIZE - ring.wptr; /* 前半 */
        int tail = all_length - head; /* 後半 */

        if ((int)header.length() <= head) {
            std::copy_n(header.begin(), header.length(), ring.buf.begin() + ring.wptr);
            head -= header.length();
            if (head > 0) {
                std::copy_n(payload.begin(), head, ring.buf.begin() + ring.wptr + header.length());
            }
            std::copy_n(payload.data() + head, tail, ring.buf.begin());
        } else {
            std::copy_n(header.begin(), head, ring.buf.begin() + ring.wptr);
            int part = header.length() - head;
            std::copy_n(header.data() + head, part, ring.buf.begin());
            if (tail > part) {
                std::copy_n(payload.begin(), tail - part, ring.buf.begin() + part);
            }
        }
        ring.buf[tail] = '\0';
        ring.wptr = tail + 1;
    }

    ring.inlen += all_length + 1;

    /* Success */
    return 0;
}

/* strが同じ文字の繰り返しかどうか調べる */
static bool string_is_repeat(concptr str, int len)
{
    char c = str[0];
    int i;

    if (len < 2) {
        return false;
    }
#ifdef JP
    if (iskanji(c)) {
        return false;
    }
#endif

    for (i = 1; i < len; i++) {
#ifdef JP
        if (c != str[i] || iskanji(str[i])) {
            return false;
        }
#else
        if (c != str[i]) {
            return false;
        }
#endif
    }

    return true;
}

#ifdef JP
/* マルチバイト文字を分割せずに str を分割する長さを len 以下で返す */
static int find_split(concptr str, int len)
{
    /*
     * SJIS が定義されている場合でも、str は常に EUC-JP としてエンコードされます。
     * 他の場所で行われているような 3 バイトのエンコーディングは想定していません。
     */
    int last_split = 0, i = 0;
    while (i < len) {
        if (iseuckanji(str[i])) {
            if (i < len - 1) {
                last_split = i + 2;
            }
            i += 2;
        } else {
            last_split = i + 1;
            ++i;
        }
    }
    return last_split;
}
#endif

static errr send_text_to_chuukei_server(TERM_LEN x, TERM_LEN y, int len, TERM_COLOR col, concptr str)
{
    if (len == 1) {
        insert_ringbuf(format("s%c%c%c%c", x + 1, y + 1, col, *str));
        return (*old_text_hook)(x, y, len, col, str);
    }

    if (string_is_repeat(str, len)) {
        while (len > SPLIT_MAX) {
            insert_ringbuf(format("n%c%c%c%c%c", x + 1, y + 1, SPLIT_MAX, col, *str));
            x += SPLIT_MAX;
            len -= SPLIT_MAX;
        }

        std::string formatted_text;
        if (len > 1) {
            formatted_text = format("n%c%c%c%c%c", x + 1, y + 1, len, col, *str);
        } else {
            formatted_text = format("s%c%c%c%c", x + 1, y + 1, col, *str);
        }

        insert_ringbuf(formatted_text);
        return (*old_text_hook)(x, y, len, col, str);
    }

#if defined(SJIS) && defined(JP)
    std::string buffer = str; // strは書き換わって欲しくないのでコピーする.
    auto *payload = buffer.data();
    sjis2euc(payload);
#else
    const auto *payload = str;
#endif
    while (len > SPLIT_MAX) {
        auto split_len = _(find_split(payload, SPLIT_MAX), SPLIT_MAX);
        insert_ringbuf(format("t%c%c%c%c", x + 1, y + 1, split_len, col), std::string_view(payload, split_len));
        x += split_len;
        len -= split_len;
        payload += split_len;
    }

    insert_ringbuf(format("t%c%c%c%c", x + 1, y + 1, len, col), std::string_view(payload, len));
    return (*old_text_hook)(x, y, len, col, str);
}

static errr send_wipe_to_chuukei_server(int x, int y, int len)
{
    while (len > SPLIT_MAX) {
        insert_ringbuf(format("w%c%c%c", x + 1, y + 1, SPLIT_MAX));
        x += SPLIT_MAX;
        len -= SPLIT_MAX;
    }
    insert_ringbuf(format("w%c%c%c", x + 1, y + 1, len));

    return (*old_wipe_hook)(x, y, len);
}

static errr send_xtra_to_chuukei_server(int n, int v)
{
    if (n == TERM_XTRA_CLEAR || n == TERM_XTRA_FRESH || n == TERM_XTRA_SHAPE) {
        insert_ringbuf(format("x%c", n + 1));

        if (n == TERM_XTRA_FRESH) {
            insert_ringbuf("d", std::to_string(get_current_time() - epoch_time));
        }
    }

    /* Verify the hook */
    if (!old_xtra_hook) {
        return -1;
    }

    return (*old_xtra_hook)(n, v);
}

static errr send_curs_to_chuukei_server(int x, int y)
{
    insert_ringbuf(format("c%c%c", x + 1, y + 1));

    return (*old_curs_hook)(x, y);
}

static errr send_bigcurs_to_chuukei_server(int x, int y)
{
    insert_ringbuf(format("C%c%c", x + 1, y + 1));

    return (*old_bigcurs_hook)(x, y);
}

/*
 * Prepare z-term hooks to call send_*_to_chuukei_server()'s
 */
void prepare_chuukei_hooks(void)
{
    term_type *t0 = angband_terms[0];

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
    TermCenteredOffsetSetter tcos(std::nullopt, std::nullopt);

    if (movie_mode) {
        movie_mode = 0;
        disable_chuukei_server();
        fd_close(movie_fd);
        msg_print(_("録画を終了しました。", "Stopped recording."));
        return;
    }

    std::stringstream ss;
    ss << player_ptr->base_name << ".amv";
    auto initial_movie_filename = ss.str();
    constexpr auto prompt = _("ムービー記録ファイル: ", "Movie file name: ");
    const auto movie_filename = input_string(prompt, 80, initial_movie_filename.data());
    if (!movie_filename.has_value()) {
        return;
    }

    const auto &path = path_build(ANGBAND_DIR_USER, movie_filename.value());
    auto fd = fd_open(path, O_RDONLY);
    if (fd >= 0) {
        const auto &filename = path.string();
        (void)fd_close(fd);
        std::string query = _("現存するファイルに上書きしますか? (", "Replace existing file ");
        query.append(filename);
        query.append(_(")", "? "));
        if (!input_check(query)) {
            return;
        }

        movie_fd = fd_open(path, O_WRONLY | O_TRUNC);
    } else {
        movie_fd = fd_make(path);
    }

    if (!movie_fd) {
        msg_print(_("ファイルを開けません！", "Can not open file."));
        return;
    }

    movie_mode = 1;
    prepare_chuukei_hooks();
    do_cmd_redraw(player_ptr);
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
    int i, start;

    recv_bytes = read(movie_fd, recv_buf + remain_bytes, RECVBUF_SIZE - remain_bytes);

    if (recv_bytes <= 0) {
        return -1;
    }

    /* 前回残ったデータ量に今回読んだデータ量を追加 */
    remain_bytes += recv_bytes;

    for (i = 0, start = 0; i < remain_bytes; i++) {
        /* データのくぎり('\0')を探す */
        if (recv_buf[i] == '\0') {
            /* 'd'で始まるデータ(タイムスタンプ)の場合は
               描画キューに保存する処理を呼ぶ */
            if ((recv_buf[start] == 'd') && (handle_movie_timestamp_data(atoi(recv_buf + start + 1)) < 0)) {
                return -1;
            }

            /* 受信データを保存 */
            if (insert_ringbuf(std::string_view(recv_buf + start, i - start)) < 0) {
                return -1;
            }

            start = i + 1;
        }
    }
    if (start > 0) {
        if (remain_bytes >= start) {
            memmove(recv_buf, recv_buf + start, remain_bytes - start);
            remain_bytes -= start;
        } else {
            remain_bytes = 0;
        }
    }

    return 0;
}

#ifndef WINDOWS
/* Win版の床の中点と壁の豆腐をピリオドとシャープにする。*/
static void win2unix(int col, char *buf)
{
    char wall;
    if (col == 9) {
        wall = '%';
    } else {
        wall = '#';
    }

    while (*buf) {
#ifdef JP
        if (iskanji(*buf)) {
            buf += 2;
            continue;
        }
#endif
        if (*buf == 127) {
            *buf = wall;
        } else if (*buf == 31) {
            *buf = '.';
        }
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
        if (ring.rptr == RINGBUF_SIZE) {
            ring.rptr = 0;
        }
        if (*ptr++ == '\0') {
            break;
        }
    }

    if (buf[0] == 'd') {
        return false;
    }

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
    if (x + len > ox) {
        nx = x + len;
    }
    /* 縦方向のチェック */
    if (y + 1 > oy) {
        ny = y + 1;
    }

    if (nx != ox || ny != oy) {
        term_resize(nx, ny);
    }
}

static bool flush_ringbuf_client()
{
    /* 書くデータなし */
    if (fresh_queue.next == fresh_queue.tail) {
        return false;
    }

    /* まだ書くべき時でない */
    if (fresh_queue.time[fresh_queue.next] > get_current_time() - epoch_time) {
        return false;
    }

    /* 時間情報(区切り)が得られるまで書く */
    char buf[1024]{};
    while (get_nextbuf(buf)) {
        auto id = buf[0];
        auto x = static_cast<uint8_t>(buf[1]) - 1;
        auto y = static_cast<uint8_t>(buf[2]) - 1;
        int len = static_cast<uint8_t>(buf[3]);
        uint8_t col = buf[4];
        char *mesg;
        if (id == 's') {
            col = buf[3];
            mesg = &buf[4];
        } else {
            mesg = &buf[5];
        }
#ifndef WINDOWS
        win2unix(col, mesg);
#endif

        switch (id) {
        case 't': /* 通常 */
#if defined(SJIS) && defined(JP)
            euc2sjis(mesg);
#endif
            update_term_size(x, y, len);
            (void)((*angband_terms[0]->text_hook)(x, y, len, (byte)col, mesg));
            std::copy_n(mesg, len, &game_term->scr->c[y][x]);
            for (auto i = x; i < x + len; i++) {
                game_term->scr->a[y][i] = col;
            }

            break;
        case 'n': /* 繰り返し */
            for (auto i = 1; i < len + 1; i++) {
                if (i == len) {
                    mesg[i] = '\0';
                    break;
                }

                mesg[i] = mesg[0];
            }

            update_term_size(x, y, len);
            (void)((*angband_terms[0]->text_hook)(x, y, len, (byte)col, mesg));
            std::copy_n(mesg, len, &game_term->scr->c[y][x]);
            for (auto i = x; i < x + len; i++) {
                game_term->scr->a[y][i] = col;
            }

            break;
        case 's': /* 一文字 */
            update_term_size(x, y, 1);
            (void)((*angband_terms[0]->text_hook)(x, y, 1, (byte)col, mesg));
            std::copy_n(&game_term->scr->c[y][x], 1, mesg);
            game_term->scr->a[y][x] = col;
            break;
        case 'w':
            update_term_size(x, y, len);
            (void)((*angband_terms[0]->wipe_hook)(x, y, len));
            break;
        case 'x':
            if (x == TERM_XTRA_CLEAR) {
                term_clear();
            }

            (void)((*angband_terms[0]->xtra_hook)(x, 0));
            break;
        case 'c':
            update_term_size(x, y, 1);
            (void)((*angband_terms[0]->curs_hook)(x, y));
            break;
        case 'C':
            update_term_size(x, y, 1);
            (void)((*angband_terms[0]->bigcurs_hook)(x, y));
            break;
        }
    }

    fresh_queue.next++;
    if (fresh_queue.next == FRESH_QUEUE_SIZE) {
        fresh_queue.next = 0;
    }
    return true;
}

void prepare_browse_movie_without_path_build(const std::filesystem::path &path)
{
    movie_fd = fd_open(path, O_RDONLY);
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
void prepare_browse_movie_with_path_build(std::string_view filename)
{
    const auto &path = path_build(ANGBAND_DIR_USER, filename);
    movie_fd = fd_open(path, O_RDONLY);
    init_buffer();
}
#endif
