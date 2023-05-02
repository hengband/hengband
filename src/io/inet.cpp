/*
 * @brief ネットワーク機能処理
 * @date 2002/01/12
 * @author mogami
 */

#include "io/inet.h"
#include "io/files-util.h"
#include "util/angband-files.h"

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

#include <stdlib.h>

static concptr errstr;
static char *proxy;
static int proxy_port;

/* プロキシサーバのアドレスををファイルから読んで設定する */
void set_proxy(char *default_url, int default_port)
{
    const auto &path = path_build(ANGBAND_DIR_PREF, "proxy.prf");
    auto *fp = angband_fopen(path, FileOpenMode::READ);
    if (!fp) {
        proxy = default_url;
        proxy_port = default_port;
        return;
    }

    char buf[1024]{};
    while (angband_fgets(fp, buf, sizeof(buf)) == 0) {
        if (buf[0] != '#' && buf[0] != '\0') {
            break;
        }
    }

    angband_fclose(fp);
    auto *s = buf;

    /* "http://" から始まっている場合はその部分をカットする。 */
#if defined(WINDOWS)
    if (!strnicmp(s, "http://", 7)) {
        s += 7;
    }
#else
    if (!strncasecmp(s, "http://", 7)) {
        s += 7;
    }
#endif

    /* 文字列の長さを調べ、必要なメモリを確保 */
    auto len = strlen(s);
    proxy = static_cast<char *>(malloc(len + 1));

    /* ポート番号があるかどうかを調べ、あればproxy_portに設定。 */
    --len;
    while (len > 0 && isdigit((unsigned char)s[len])) {
        --len;
    }
    if (len > 0 && s[len] == ':' && s[len + 1] != '\0') {
        s[len] = '\0';
        strcpy(proxy, s);
        proxy_port = atoi(s + (len + 1));
    } else {
        strcpy(proxy, s);
        proxy_port = default_port;
    }

    /* プロキシのアドレスをproxyにコピー */
    strcpy(proxy, s);

    if (proxy_port == 0) {
        proxy_port = 80;
    }
}

/* ソケットにバッファの内容を書き込む */
int soc_write(int sd, char *buf, size_t sz)
{
    int nleft, nwritten;

    nleft = sz;

    while (nleft > 0) {
        nwritten = send(sd, buf, nleft, 0);
        if (nwritten <= 0) {
            return nwritten;
        }
        nleft -= nwritten;
        buf += nwritten;
    }

    return sz;
}

int soc_read(int sd, char *buf, size_t sz)
{
    int nleft, nread = 0;

    nleft = sz;

    while (nleft > 0) {
        int n;
        n = recv(sd, buf + nread, nleft, 0);
        if (n <= 0) {
            return nread;
        }
        nleft -= n;
        nread += n;
    }

    return nread;
}

#if !defined(WINDOWS)
static sigjmp_buf env;
static void (*sig_int_saved)(int);
static void (*sig_alm_saved)(int);
#endif

static void restore_signal(void)
{
#if !defined(WINDOWS)
    struct itimerval val0;

    /* itimerリセット用 */
    val0.it_interval.tv_sec = 0;
    val0.it_interval.tv_usec = 0;
    val0.it_value.tv_sec = 0;
    val0.it_value.tv_usec = 0;

    /* アラーム解除 */
    setitimer(ITIMER_REAL, &val0, nullptr);
    signal(SIGALRM, sig_alm_saved);
    signal(SIGINT, sig_int_saved);
#endif
}

#if !defined(WINDOWS)
static void interrupt_report(int sig)
{
    restore_signal();
    siglongjmp(env, sig);
}
#endif

/* サーバにコネクトする関数。 */
int connect_server(int timeout, concptr host, int port)
{
    int sd;
    struct sockaddr_in to;
    struct hostent *hp;

#ifndef WINDOWS
    struct itimerval val;
    int ret;

    /* itimer設定用 */
    val.it_interval.tv_sec = 0;
    val.it_interval.tv_usec = 0;
    val.it_value.tv_sec = timeout;
    val.it_value.tv_usec = 0;

    /* タイムアウト、もしくは中断した時の処理。 */
    if ((ret = sigsetjmp(env, 1)) != 0) {
#ifdef JP
        if (ret == SIGALRM) {
            errstr = "エラー: タイムアウト";
        } else {
            errstr = "エラー: インタラプト";
        }
#else
        if (ret == SIGALRM) {
            errstr = "Error : time out";
        } else {
            errstr = "Error : interupted";
        }
#endif
        return -1;
    }
    sig_int_saved = signal(SIGINT, interrupt_report);
    sig_alm_saved = signal(SIGALRM, interrupt_report);

    /* タイムアウトの時間を設定 */
    setitimer(ITIMER_REAL, &val, nullptr);
#else
    /* Unused in Windows */
    (void)timeout;
#endif

    /* プロキシが設定されていればプロキシに繋ぐ */
    if (proxy && proxy[0]) {
        if ((hp = gethostbyname(proxy)) == nullptr) {
#ifdef JP
            errstr = "エラー: プロキシのアドレスが不正です";
#else
            errstr = "Error : wrong proxy address";
#endif

            restore_signal();

            return -1;
        }
    } else if ((hp = gethostbyname(host)) == nullptr) {
#ifdef JP
        errstr = "エラー: サーバのアドレスが不正です";
#else
        errstr = "Error : wrong server address";
#endif

        restore_signal();

        return -1;
    }

    memset(&to, 0, sizeof(to));
    memcpy(&to.sin_addr, hp->h_addr_list[0], hp->h_length);

    to.sin_family = AF_INET;

    if (proxy && proxy[0] && proxy_port) {
        to.sin_port = htons(static_cast<ushort>(proxy_port));
    } else {
        to.sin_port = htons(static_cast<ushort>(port));
    }

#ifndef WINDOWS
    if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
#else
    if ((sd = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
#endif
    {
#ifdef JP
        errstr = "エラー: ソケットを生成できません";
#else
        errstr = "Error : cannot create socket.";
#endif
        restore_signal();
        return -1;
    }

    if (connect(sd, (struct sockaddr *)&to, sizeof(to)) < 0) {
#ifdef JP
        errstr = "エラー: サーバに接続できません";
#else
        errstr = "Error : failed to connect server";
#endif
        restore_signal();
#ifndef WINDOWS
        close(sd);
#else
        closesocket(sd);
#endif
        return -1;
    }

    restore_signal();

    return sd;
}

int disconnect_server(int sd)
{
#if defined(WINDOWS)
    return closesocket(sd);
#else
    return close(sd);
#endif
}

concptr soc_err()
{
    return errstr;
}

#endif /* WORLD_SCORE */
