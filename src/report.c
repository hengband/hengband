/* File: report.c */

#define _GNU_SOURCE
#include "angband.h"

#ifdef WORLD_SCORE

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

#if defined(WINDOWS)
#include <winsock.h>
#elif defined(MACINTOSH)
#include <OpenTransport.h>
#include <OpenTptInternet.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>

#include <setjmp.h>
#include <signal.h>
#endif

#ifdef JP
#define SCORE_PATH "http://www.kmc.gr.jp/~habu/local/hengscore/score.cgi"
#else
#define SCORE_PATH "http://www.kmc.gr.jp/~habu/local/hengscore-en/score.cgi"
#endif

/* for debug */
/*#define SCORE_PATH "http://www.kmc.gr.jp/~habu/local/scoretest/score.cgi" */

/*
 * simple buffer library
 */

typedef struct {
	size_t max_size;
	size_t size;
	char *data;
} BUF;

#define	BUFSIZE	(65536)

#if defined(WINDOWS) || defined(SUNOS4) || defined(MACINTOSH) || defined(SGI)
#define vasprintf	Vasprintf
#endif

#ifdef SUNOS4
static int Vasprintf(char **buf, const char *fmt, va_list ap)
{
	int ret;

	*buf = malloc(BUFSIZE);

	ret = vsnprintf(*buf, BUFSIZE, fmt, ap);

	return ret;
}
#endif

#if defined(WINDOWS) || defined(MACINTOSH) || defined(SGI)
static int Vasprintf(char **buf, const char *fmt, va_list ap)
{
	int ret;
	*buf = malloc(BUFSIZE * 4);

	ret = vsprintf(*buf, fmt, ap);

	return ret;
}
#endif

static BUF* buf_new(void)
{
	BUF *p;

	if ((p = malloc(sizeof(BUF))) == NULL)
		return NULL;

	p->size = 0;
	p->max_size = BUFSIZE;
	if ((p->data = malloc(BUFSIZE)) == NULL)
	{
		free(p);
		return NULL;
	}
	return p;
}

#if 0
static void buf_delete(BUF *b)
{
	free(b->data);
	free(b);
}
#endif

static int buf_append(BUF *buf, const char *data, size_t size)
{
	while (buf->size + size > buf->max_size)
	{
		char *tmp;
		if ((tmp = malloc(buf->max_size * 2)) == NULL) return -1;

		memcpy(tmp, buf->data, buf->max_size);
		free(buf->data);

		buf->data = tmp;

		buf->max_size *= 2;
	}
	memcpy(buf->data + buf->size, data, size);
	buf->size += size;

	return buf->size;
}

static int buf_sprintf(BUF *buf, const char *fmt, ...)
{
	int		ret;
	char	*tmpbuf;
	va_list	ap;

	va_start(ap, fmt);
	vasprintf(&tmpbuf, fmt, ap);
	va_end(ap);

	if(!tmpbuf) return -1;

	ret = buf_append(buf, tmpbuf, strlen(tmpbuf));

	free(tmpbuf);

	return ret;
}

#if 0
static int buf_read(BUF *buf, int fd)
{
	int len;
#ifndef MACINTOSH
	char tmp[BUFSIZE];
#else
	char *tmp;
	
	tmp = calloc( BUFSIZE , sizeof(char) );
#endif

	while ((len = read(fd, tmp, BUFSIZE)) > 0)
		buf_append(buf, tmp, len);

	return buf->size;
}
#endif

#if 0
static int buf_write(BUF *buf, int fd)
{
	write(fd, buf->data, buf->size);

	return buf->size;
}

static int buf_search(BUF *buf, const char *str)
{
	char *ret;

	ret = strstr(buf->data, str);

	if (!ret) return -1;

	return ret - buf->data;
}

static BUF * buf_subbuf(BUF *buf, int pos1, size_t sz)
{
	BUF *ret;

	if (pos1 < 0) return NULL;

	ret = buf_new();

	if (sz <= 0) sz = buf->size - pos1;

	buf_append(ret, buf->data + pos1, sz);

	return ret;
}
#endif

static void http_post(int sd, char *url, BUF *buf)
{
	BUF *output;

	output = buf_new();
	buf_sprintf(output, "POST %s HTTP/1.0\r\n", url);
	buf_sprintf(output, "User-Agent: Hengband %d.%d.%d\r\n",
		    FAKE_VER_MAJOR-10, FAKE_VER_MINOR, FAKE_VER_PATCH);

	buf_sprintf(output, "Content-Length: %d\r\n", buf->size);
	buf_sprintf(output, "Content-Encoding: binary\r\n");
	buf_sprintf(output, "Content-Type: application/octet-stream\r\n");
	buf_sprintf(output, "\r\n");
	buf_append(output, buf->data, buf->size);

	soc_write(sd, output->data, output->size);
}


/* キャラクタダンプを作って BUFに保存 */
static errr make_dump(BUF* dumpbuf)
{
	errr make_character_dump(FILE *fff);

	char		buf[1024];
	FILE *fff;
	char file_name[1024];

	/* Open a new file */
	fff = my_fopen_temp(file_name, 1024);
	if (!fff)
	{
#ifdef JP
		msg_format("一時ファイル %s を作成できませんでした。", file_name);
#else
		msg_format("Failed to create temporary file %s.", file_name);
#endif
		msg_print(NULL);
		return 1;
	}

	/* 一旦一時ファイルを作る。通常のダンプ出力と共通化するため。 */
	(void)make_character_dump(fff);

	/* Close the file */
	my_fclose(fff);

	/* Open for read */
	fff = my_fopen(file_name, "r");

	while (fgets(buf, 1024, fff))
	{
		(void)buf_append(dumpbuf, buf, strlen(buf));
	}

	/* Remove the file */
	fd_kill(file_name);

	/* Success */
	return (0);
}

errr report_score(void)
{
#ifdef MACINTOSH
	OSStatus err;
#else
	errr err = 0;
#endif

#ifdef WINDOWS
	WSADATA wsaData;
	WORD wVersionRequested =(WORD) (( 1) |  ( 1 << 8));
#endif

	BUF *score;
	int sd;
	char seikakutmp[128];

	score = buf_new();

#ifdef JP
	sprintf(seikakutmp, "%s%s", ap_ptr->title, (ap_ptr->no ? "の" : ""));
#else
	sprintf(seikakutmp, "%s ", ap_ptr->title);
#endif

	buf_sprintf(score, "name: %s\n", player_name);
#ifdef JP
	buf_sprintf(score, "version: 変愚蛮怒 %d.%d.%d\n",
		    FAKE_VER_MAJOR-10, FAKE_VER_MINOR, FAKE_VER_PATCH);
#else
	buf_sprintf(score, "version: Hengband %d.%d.%d\n",
		    FAKE_VER_MAJOR-10, FAKE_VER_MINOR, FAKE_VER_PATCH);
#endif
	buf_sprintf(score, "score: %d\n", total_points());
	buf_sprintf(score, "level: %d\n", p_ptr->lev);
	buf_sprintf(score, "depth: %d\n", dun_level);
	buf_sprintf(score, "maxlv: %d\n", p_ptr->max_plv);
	buf_sprintf(score, "maxdp: %d\n", max_dlv[DUNGEON_ANGBAND]);
	buf_sprintf(score, "au: %d\n", p_ptr->au);
	buf_sprintf(score, "turns: %d\n", turn_real(turn));
	buf_sprintf(score, "sex: %d\n", p_ptr->psex);
	buf_sprintf(score, "race: %s\n", rp_ptr->title);
	buf_sprintf(score, "class: %s\n", cp_ptr->title);
	buf_sprintf(score, "seikaku: %s\n", seikakutmp);
	buf_sprintf(score, "realm1: %s\n", realm_names[p_ptr->realm1]);
	buf_sprintf(score, "realm2: %s\n", realm_names[p_ptr->realm2]);
	buf_sprintf(score, "killer: %s\n", died_from);
	buf_sprintf(score, "-----charcter dump-----\n");

	make_dump(score);

#ifdef WINDOWS
	if (WSAStartup(wVersionRequested, &wsaData))
	{
		msg_print("Report: WSAStartup failed.");
		goto report_end;
	}
#endif

#ifdef MACINTOSH
#if TARGET_API_MAC_CARBON
	err = InitOpenTransportInContext(kInitOTForApplicationMask, NULL);
#else
	err = InitOpenTransport();
#endif
	if (err != noErr)
	{
		msg_print("Report: OpenTransport failed.");
		return 1;
	}
#endif

	Term_clear();

	while (1)
	{
		char buff[160];
#ifdef JP
		prt("接続中...", 0, 0);
#else
		prt("connecting...", 0, 0);
#endif
		Term_fresh();
		
		sd = connect_scoreserver();
		if (!(sd < 0)) break;
#ifdef JP
		sprintf(buff, "スコア・サーバへの接続に失敗しました。(%s)", soc_err());
#else
		sprintf(buff, "Failed to connect to the score server.(%s)", soc_err());
#endif
		prt(buff, 0, 0);
		(void)inkey();
		
#ifdef JP
		if (!get_check("もう一度接続を試みますか? "))
#else
		if (!get_check("Try again? "))
#endif
		{
			err = 1;
			goto report_end;
		}
	}
#ifdef JP
	prt("スコア送信中...", 0, 0);
#else
	prt("Sending the score...", 0, 0);
#endif
	Term_fresh();
	http_post(sd, SCORE_PATH, score);

	disconnect_server(sd);
 report_end:
#ifdef WINDOWS
	WSACleanup();
#endif

#ifdef MACINTOSH
#if TARGET_API_MAC_CARBON
	CloseOpenTransportInContext(NULL);
#else
	CloseOpenTransport();
#endif
#endif

	return err;
}

#endif /* WORLD_SCORE */
