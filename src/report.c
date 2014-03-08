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

/*
 * internet resource value
 */
#define HTTP_PROXY ""                   /* Default proxy url */
#define HTTP_PROXY_PORT 0               /* Default proxy port */
#define HTTP_TIMEOUT    20              /* Timeout length (second) */
#define SCORE_SERVER "moon.kmc.gr.jp"    /* Default score server url */
#define SCORE_PORT 80                   /* Default score server port */

#ifdef JP
#define SCORE_PATH "http://moon.kmc.gr.jp/hengband/hengscore/score.cgi"
#else
#define SCORE_PATH "http://moon.kmc.gr.jp/hengband/hengscore-en/score.cgi"
#endif

/* for debug */
#if 0
#define SCORE_PATH "http://moon.kmc.gr.jp/hengband/scoretest/score.cgi"
#endif

/*
 * simple buffer library
 */

typedef struct {
	size_t max_size;
	size_t size;
	char *data;
} BUF;

#define	BUFSIZE	(65536)

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

static void buf_delete(BUF *b)
{
	free(b->data);
	free(b);
}

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
	char	tmpbuf[8192];
	va_list	ap;

	va_start(ap, fmt);
#if defined(HAVE_VSNPRINTF)
	ret = vsnprintf(tmpbuf, sizeof(tmpbuf), fmt, ap);
#else
	ret = vsprintf(tmpbuf, fmt, ap);
#endif
	va_end(ap);

	if (ret < 0) return -1;

#if ('\r' == 0x0a && '\n' == 0x0d)
	{
		/*
		 * Originally '\r'= CR (= 0x0d) and '\n'= LF (= 0x0a)
		 * But for MPW (Macintosh Programers Workbench), these
		 * are reversed so that '\r'=LF and '\n'=CR unless the
		 * -noMapCR option is not defined.
		 *
		 * We need to swap back these here since the score
		 * dump text should be written using LF as the end of
		 * line.
		 */
		char *ptr;
		for (ptr = tmpbuf; *ptr; ptr++)
		{
			if (0x0d == *ptr) *ptr = 0x0a;
		}
	}
#endif

	ret = buf_append(buf, tmpbuf, strlen(tmpbuf));

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

	ret = my_strstr(buf->data, str);

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

static void http_post(int sd, cptr url, BUF *buf)
{
	BUF *output;

	output = buf_new();
	buf_sprintf(output, "POST %s HTTP/1.0\n", url);
	buf_sprintf(output, "User-Agent: Hengband %d.%d.%d\n",
		    FAKE_VER_MAJOR-10, FAKE_VER_MINOR, FAKE_VER_PATCH);

	buf_sprintf(output, "Content-Length: %d\n", buf->size);
	buf_sprintf(output, "Content-Encoding: binary\n");
	buf_sprintf(output, "Content-Type: application/octet-stream\n");
	buf_sprintf(output, "\n");
	buf_append(output, buf->data, buf->size);

	soc_write(sd, output->data, output->size);
}


/* キャラクタダンプを作って BUFに保存 */
static errr make_dump(BUF* dumpbuf)
{
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
		(void)buf_sprintf(dumpbuf, "%s", buf);
	}

	/* Close the file */
	my_fclose(fff);

	/* Remove the file */
	fd_kill(file_name);

	/* Success */
	return (0);
}

/*
 * Make screen dump to buffer
 */
cptr make_screen_dump(void)
{
	BUF *screen_buf;
	int y, x, i;
	cptr ret;

	byte a = 0, old_a = 0;
	char c = ' ';

	static cptr html_head[] = {
		"<html>\n<body text=\"#ffffff\" bgcolor=\"#000000\">\n",
		"<pre>",
		0,
	};
	static cptr html_foot[] = {
		"</pre>\n",
		"</body>\n</html>\n",
		0,
	};

	bool old_use_graphics = use_graphics;

	int wid, hgt;

	Term_get_size(&wid, &hgt);

	/* Alloc buffer */
	screen_buf = buf_new();
	if (screen_buf == NULL) return (NULL);

	if (old_use_graphics)
	{
		/* Clear -more- prompt first */
		msg_print(NULL);

		use_graphics = FALSE;
		reset_visuals();

		/* Redraw everything */
		p_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY);

		/* Hack -- update */
		handle_stuff();
	}

	for (i = 0; html_head[i]; i++)
		buf_sprintf(screen_buf, html_head[i]);

	/* Dump the screen */
	for (y = 0; y < hgt; y++)
	{
		/* Start the row */
		if (y != 0)
			buf_sprintf(screen_buf, "\n");

		/* Dump each row */
		for (x = 0; x < wid - 1; x++)
		{
			int rv, gv, bv;
			cptr cc = NULL;
			/* Get the attr/char */
			(void)(Term_what(x, y, &a, &c));

			switch (c)
			{
			case '&': cc = "&amp;"; break;
			case '<': cc = "&lt;"; break;
			case '>': cc = "&gt;"; break;
#ifdef WINDOWS
			case 0x1f: c = '.'; break;
			case 0x7f: c = (a == 0x09) ? '%' : '#'; break;
#endif
			}

			a = a & 0x0F;
			if ((y == 0 && x == 0) || a != old_a) {
				rv = angband_color_table[a][1];
				gv = angband_color_table[a][2];
				bv = angband_color_table[a][3];
				buf_sprintf(screen_buf, "%s<font color=\"#%02x%02x%02x\">", 
					    ((y == 0 && x == 0) ? "" : "</font>"), rv, gv, bv);
				old_a = a;
			}
			if (cc)
				buf_sprintf(screen_buf, "%s", cc);
			else
				buf_sprintf(screen_buf, "%c", c);
		}
	}
	buf_sprintf(screen_buf, "</font>");

	for (i = 0; html_foot[i]; i++)
		buf_sprintf(screen_buf, html_foot[i]);

	/* Screen dump size is too big ? */
	if (screen_buf->size + 1> SCREEN_BUF_SIZE)
	{
		ret = NULL;
	}
	else
	{
		/* Terminate string */
		buf_append(screen_buf, "", 1);

		ret = string_make(screen_buf->data);
	}

	/* Free buffer */
	buf_delete(screen_buf);

	if (old_use_graphics)
	{
		use_graphics = TRUE;
		reset_visuals();

		/* Redraw everything */
		p_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY);

		/* Hack -- update */
		handle_stuff();
	}

	return ret;
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
	buf_sprintf(score, "killer: %s\n", p_ptr->died_from);
	buf_sprintf(score, "-----charcter dump-----\n");

	make_dump(score);

	if (screen_dump)
	{
		buf_sprintf(score, "-----screen shot-----\n");
		buf_append(score, screen_dump, strlen(screen_dump));
	}
	
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
		
		/* プロキシを設定する */
		set_proxy(HTTP_PROXY, HTTP_PROXY_PORT);

		/* Connect to the score server */
		sd = connect_server(HTTP_TIMEOUT, SCORE_SERVER, SCORE_PORT);


		if (!(sd < 0)) break;
#ifdef JP
		sprintf(buff, "スコア・サーバへの接続に失敗しました。(%s)", soc_err());
#else
		sprintf(buff, "Failed to connect to the score server.(%s)", soc_err());
#endif
		prt(buff, 0, 0);
		(void)inkey();
		
#ifdef JP
		if (!get_check_strict("もう一度接続を試みますか? ", CHECK_NO_HISTORY))
#else
		if (!get_check_strict("Try again? ", CHECK_NO_HISTORY))
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
