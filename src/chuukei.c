/* chuukei.c */

#include "angband.h"

#ifdef CHUUKEI

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

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
#include <arpa/inet.h>

#include <setjmp.h>
#include <signal.h>
#endif

#define MAX_HOSTNAME 256
#define RINGBUF_SIZE 65536*8
#define FRESH_QUEUE_SIZE 1024
#define WAIT 100*1000 /* ブラウズ側のウエイト(us単位) */
#define DEFAULT_DELAY 50

static int sd; /* ソケットのファイルディスクリプタ */
static long epoch_time;  /* 開始時刻 */
static long time_diff;   /* プレイ側との時間のずれ(これを見ながらディレイを調整していく) */
static int browse_delay; /* 表示するまでの時間(100ms単位)(この間にラグを吸収する) */
static int server_port;
static char server_name[MAX_HOSTNAME];


#ifdef WINDOWS
#define close closesocket
#endif

#ifdef MACINTOSH
static InetSvcRef inet_services = nil;
static EndpointRef ep			= kOTInvalidEndpointRef;
#endif

/* 描画する時刻を覚えておくキュー構造体 */
static struct
{
	int time[FRESH_QUEUE_SIZE];
	int next;
	int tail;
}fresh_queue;


/* リングバッファ構造体 */
static struct
{
	char *buf;
	int wptr;
	int rptr;
	int inlen;
}ring;

#ifdef MACINTOSH
int recv(int s, char *buffer, size_t buflen, int flags)
{
	OTFlags 	junkFlags;
	int n = OTRcv(ep, (void *) buffer, buflen, &junkFlags);
	if( n <= 0 )
		return n;
	return n;
}
#endif

/* ANSI Cによればstatic変数は0で初期化されるが一応初期化する */
static errr init_chuukei(void)
{
	fresh_queue.next = fresh_queue.tail = 0;
	ring.wptr = ring.rptr = ring.inlen = 0;
	fresh_queue.time[0] = 0;
	ring.buf = malloc(RINGBUF_SIZE);
	if (ring.buf == NULL) return (-1);

	return (0);
}

/* 現在の時間を100ms単位で取得する */
static long get_current_time(void)
{
#ifdef WINDOWS
	return GetTickCount() / 100;
#elif defined(MACINTOSH)
	return TickCount();
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

	/* バッファをオーバー */
	if (ring.inlen + len >= RINGBUF_SIZE)
	{
		chuukei_server = FALSE;
		chuukei_client = FALSE;

		prt("バッファが溢れました。サーバとの接続を切断します。", 0, 0);
		inkey();

		close(sd);

		return (-1);
	}

	/* バッファの終端までに収まる */
	if (ring.wptr + len < RINGBUF_SIZE)
	{
		memcpy(ring.buf + ring.wptr, buf, len);
		ring.wptr += len;
	}
	/* バッファの終端までに収まらない(ピッタリ収まる場合も含む) */
	else
	{
		int head = RINGBUF_SIZE - ring.wptr;  /* 前半 */
		int tail = len - head;               /* 後半 */

		memcpy(ring.buf + ring.wptr, buf, head);
		memcpy(ring.buf, buf + head, tail);
		ring.wptr = tail;
	}

	ring.inlen += len;

	/* Success */
	return (0);
}

void flush_ringbuf(void)
{
#ifndef MACINTOSH
	fd_set fdset;
	struct timeval tv;
	int writen = 0;

	if (!chuukei_server) return;

	if (ring.inlen == 0) return;

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	FD_ZERO(&fdset);
	FD_SET(sd, &fdset);

	while (1)
	{
		fd_set tmp_fdset;
		struct timeval tmp_tv;
		int result;

		tmp_fdset = fdset;
		tmp_tv = tv;

		/* ソケットにデータを書き込めるかどうか調べる */
		select(sd+1, (fd_set *)NULL, &tmp_fdset, (fd_set *)NULL, &tv);

		/* 書き込めなければ戻る */
		if (FD_ISSET(sd, &tmp_fdset) == 0) break;

		result = send(sd, ring.buf + ring.rptr, ((ring.wptr > ring.rptr ) ? ring.wptr : RINGBUF_SIZE) - ring.rptr, 0);

		if (result <= 0)
		{
			/* サーバとの接続断？ */
			chuukei_server = FALSE;

			prt("サーバとの接続が切断されました。", 0, 0);
			inkey();
			close(sd);

			return;
		}

		ring.rptr += result;
		ring.inlen -= result;
		writen += result;

		if (ring.rptr == RINGBUF_SIZE) ring.rptr = 0;
		if (ring.inlen == 0) break;
	}
#else
	struct timeval tv;
	int writen = 0;

	if (!chuukei_server) return;

	if (ring.inlen == 0) return;

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	while (1)
	{
		struct timeval tmp_tv;
		int result;

		tmp_tv = tv;

		/* ソケットにデータを書き込めるかどうか調べる */
		result = OTSnd(ep, ring.buf + ring.rptr, ((ring.wptr > ring.rptr ) ? ring.wptr : RINGBUF_SIZE) - ring.rptr, 0);

		if (result <= 0)
		{
			/* サーバとの接続断？ */
			chuukei_server = FALSE;

			prt("サーバとの接続が切断されました。", 0, 0);
			inkey();
			close(sd);

			return;
		}

		ring.rptr += result;
		ring.inlen -= result;
		writen += result;

		if (ring.rptr == RINGBUF_SIZE) ring.rptr = 0;
		if (ring.inlen == 0) break;
	}
#endif
}

static int read_chuukei_prf(cptr prf_name)
{
	char buf[1024];
	FILE *fp;

	path_build(buf, sizeof(buf), ANGBAND_DIR_XTRA, prf_name);
	fp = my_fopen(buf, "r");

	if (!fp) return (-1);

	/* 初期化 */
	server_port = -1;
	server_name[0] = 0;
	browse_delay = DEFAULT_DELAY;

	while (0 == my_fgets(fp, buf, sizeof(buf)))
	{
		/* サーバ名 */
		if (!strncmp(buf, "server:", 7))
		{
			strncpy(server_name, buf + 7, MAX_HOSTNAME - 1);
			server_name[MAX_HOSTNAME - 1] = '\0';
		}

		/* ポート番号 */
		if (!strncmp(buf, "port:", 5))
		{
			server_port = atoi(buf + 5);
		}

		/* ディレイ */
		if (!strncmp(buf, "delay:", 6))
		{
			browse_delay = atoi(buf + 6);
		}
	}

	my_fclose(fp);

	/* prfファイルが完全でない */
	if (server_port == -1 || server_name[0] == 0) return (-1);

	return (0);
}

int connect_chuukei_server(char *prf_name)
{
#ifndef MACINTOSH

#ifdef WINDOWS
	WSADATA wsaData;
	WORD wVersionRequested = (WORD) (( 1) |  ( 1 << 8));
#endif

	struct sockaddr_in ask;
	struct hostent *hp;

	if (read_chuukei_prf(prf_name) < 0)
	{
		printf("Wrong prf file\n");
		return (-1);
	}

	if (init_chuukei() < 0)
	{
		printf("Malloc error\n");
		return (-1);
	}

#ifdef WINDOWS
	if (WSAStartup(wVersionRequested, &wsaData))
	{
		msg_print("Report: WSAStartup failed.");
		return (-1);
	}
#endif

	printf("server = %s\nport = %d\n", server_name, server_port);

	if ((hp = gethostbyname(server_name)) != NULL)
	{
		memset(&ask, 0, sizeof(ask));
		memcpy(&ask.sin_addr, hp->h_addr_list[0], hp->h_length);
	}
	else
	{
		if ((ask.sin_addr.s_addr=inet_addr(server_name)) == 0)
		{
			printf("Bad hostname\n");
			return (-1);
		}
	}

	ask.sin_family = AF_INET;
	ask.sin_port = htons((unsigned short)server_port);

#ifndef WINDOWS
	if ((sd=socket(PF_INET,SOCK_STREAM, 0)) < 0)
#else
	if ((sd=socket(PF_INET,SOCK_STREAM, 0)) == INVALID_SOCKET)
#endif
	{
		printf("Can't create socket\n");
		return (-1);
	}

	if (connect(sd, (struct sockaddr *)&ask, sizeof(ask)) < 0)
	{
		close(sd);
		printf("Can't connect %s port %d\n", server_name, server_port);
		return (-1);
	}

	epoch_time = get_current_time();

	return (0);
#else	/* MACINTOSH */
	OSStatus err;
	InetHostInfo 	response;
	InetHost 		host_addr;
	InetAddress 	inAddr;
	TCall 			sndCall;
	Boolean			bind	= false;
	OSStatus 	junk;

	if (read_chuukei_prf(prf_name) < 0){
		printf("Wrong prf file\n");
		return (-1);
	}
	
	init_chuukei();
	
	printf("server = %s\nport = %d\n", server_name, server_port);


#if TARGET_API_MAC_CARBON
	err = InitOpenTransportInContext(kInitOTForApplicationMask, NULL);
#else
	err = InitOpenTransport();
#endif

	memset(&response, 0, sizeof(response));


#if TARGET_API_MAC_CARBON
	inet_services = OTOpenInternetServicesInContext(kDefaultInternetServicesPath, 0, &err, NULL);
#else
	inet_services = OTOpenInternetServices(kDefaultInternetServicesPath, 0, &err);
#endif
	
	if (err == noErr) {
		err = OTInetStringToAddress(inet_services, (char *)server_name, &response);
		
		if (err == noErr) {
			host_addr = response.addrs[0];
		} else {
			printf("Bad hostname\n");
		}
		
#if TARGET_API_MAC_CARBON
		ep = (void *)OTOpenEndpointInContext(OTCreateConfiguration(kTCPName), 0, nil, &err, NULL);
#else
		ep = (void *)OTOpenEndpoint(OTCreateConfiguration(kTCPName), 0, nil, &err);
#endif

		if (err == noErr) {
			err = OTBind(ep, nil, nil);
			bind = (err == noErr);
	    }
	    if (err == noErr){
		OTInitInetAddress(&inAddr, server_port, host_addr);
			
			sndCall.addr.len 	= sizeof(InetAddress);				
			sndCall.addr.buf	= (unsigned char*) &inAddr;
			sndCall.opt.buf 	= nil;	      /* no connection options */
			sndCall.opt.len 	= 0;
			sndCall.udata.buf 	= nil;	      /* no connection data */
			sndCall.udata.len 	= 0;
			sndCall.sequence 	= 0;	      /* ignored by OTConnect */
			
			err = OTConnect(ep, &sndCall, NULL);
			
			if( err != noErr ){
				printf("Can't connect %s port %d\n", server_name, server_port);
			}
		}
		
		err = OTSetSynchronous(ep);
		if (err == noErr)		
			err = OTSetBlocking(ep);
		
	}
	
	if( err != noErr ){
		if( bind ){
			OTUnbind(ep);
		}
		/* Clean up. */
		if (ep != kOTInvalidEndpointRef) {
			OTCloseProvider(ep);
			ep = nil;
		}
		if (inet_services != nil) {
			OTCloseProvider(inet_services);
			inet_services = nil;
		}
	
		return -1;
	}
	
	return 0;

#endif
}


/* strが同じ文字の繰り返しかどうか調べる */
static bool string_is_repeat(char *str, int len)
{
	char c = str[0];
	int i;

	if (len < 2) return (FALSE);
#ifdef JP
	if (iskanji(c)) return (FALSE);
#endif

	for (i = 1; i < len; i++)
	{
#ifdef JP
		if(c != str[i] || iskanji(str[i])) return (FALSE);
#else
		if(c != str[i]) return (FALSE);
#endif
	}

	return (TRUE);
}

void send_text_to_chuukei_server(int x, int y, int len, int col, char *str)
{
	char buf[1024];
	char buf2[1024];

	if (!chuukei_server || Term != angband_term[0]) return;

	strncpy(buf2, str, len);
	buf2[len] = '\0';

	if (len == 1)
	{
		sprintf(buf, "s%c%c%c%c", x+1, y+1, col, buf2[0]);
	}
	else if(string_is_repeat(buf2, len))
	{
		sprintf(buf, "n%c%c%c%c%c", x+1, y+1, len, col, buf2[0]);
	}
	else
	{
#ifdef SJIS
		sjis2euc(buf2);
#endif
		sprintf(buf, "t%c%c%c%c%s", x+1, y+1, len, col, buf2);
	}

	insert_ringbuf(buf);
}

void send_wipe_to_chuukei_server(int x, int y, int len)
{
	char buf[1024];

	if (!chuukei_server || Term != angband_term[0]) return;

	sprintf(buf, "w%c%c%c", x+1, y+1, len);

	insert_ringbuf(buf);
}

void send_xtra_to_chuukei_server(int n)
{
	char buf[1024];

	if (!chuukei_server || Term != angband_term[0]) return;
	sprintf(buf, "x%c", n+1);

	insert_ringbuf(buf);

	if (n == TERM_XTRA_FRESH)
	{
		sprintf(buf, "d%ld", get_current_time() - epoch_time);
		insert_ringbuf(buf);
	}
}

void send_curs_to_chuukei_server(int x, int y)
{
	char buf[1024];

	if (!chuukei_server || Term != angband_term[0]) return;
	sprintf(buf, "c%c%c", x+1, y+1);

	insert_ringbuf(buf);
}

static int read_sock(void)
{
	char buf[1024];
	int i;

	for (i = 0;; i++)
	{
		if (recv(sd, buf+i, 1, 0) <= 0) 
			return -1;

		if (buf[i] == '\0')
		{
			if (buf[0] == 'd')
			{
				int timestamp = atoi(buf + 1);
				long current_time = get_current_time();

				/* 最初の時間を保存しておく */
				if (!fresh_queue.time[0])
				{
					epoch_time = current_time;
					epoch_time += browse_delay;
					epoch_time -= timestamp;
					time_diff = current_time - timestamp;
				}

				fresh_queue.time[fresh_queue.tail] = timestamp;
				fresh_queue.tail ++;

				if (fresh_queue.tail == FRESH_QUEUE_SIZE)
					fresh_queue.tail = 0;

				/* プレイ側とのディレイを調整 */
				if (time_diff > current_time - timestamp)
				{
					long old_time_diff = time_diff;
					time_diff = current_time - timestamp;
					epoch_time -= (old_time_diff - time_diff);
				}

				if (fresh_queue.tail == fresh_queue.next)
				{
					/* queue溢れ */
					close(sd);
					exit(1);
				}

			}

			if (insert_ringbuf(buf) < 0) 
				return -1;
			return (i);
		}
	}
}


#ifndef WINDOWS
/* Win版の床の中点と壁の豆腐をピリオドとシャープにする。 */
static void win2unix(int col, char *buf)
{
	char kabe;
	if ( col == 9 ) kabe = '%';
	else            kabe = '#';

	while (*buf)
	{
#ifdef JP
		if (iskanji(*buf))
		{
			buf += 2;
			continue;
		}
#endif
		if (*buf == 127) *buf = kabe;
		else if(*buf == 31) *buf = '.';
		buf++;
	}
}
#endif

static bool get_nextbuf(char *buf)
{
	char *ptr = buf;

	while (1)
	{
		*ptr = ring.buf[ring.rptr ++];
		ring.inlen --;
		if (ring.rptr == RINGBUF_SIZE) ring.rptr = 0;
		if (*ptr++ == '\0') break;
	}

	if (buf[0] == 'd') return (FALSE);

	return (TRUE);
}

static bool flush_ringbuf_client(void)
{
	char buf[1024];

	/* 書くデータなし */
	if (fresh_queue.next == fresh_queue.tail) return (FALSE);

	/* まだ書くべき時でない */
	if (fresh_queue.time[fresh_queue.next] > get_current_time() - epoch_time) return (FALSE);

	/* 時間情報(区切り)が得られるまで書く */
	while (get_nextbuf(buf))
	{
		char id;
		int x, y, len, col;
		int i;
		char tmp1, tmp2, tmp3, tmp4;
		char *mesg;

		sscanf(buf, "%c%c%c%c%c", &id, &tmp1, &tmp2, &tmp3, &tmp4);
		x = tmp1-1; y = tmp2-1; len = tmp3; col = tmp4;
		if (id == 's')
		{
			col = tmp3;
			mesg = &buf[4];
		}
		else mesg = &buf[5];
#ifndef WINDOWS
		win2unix(col, mesg);
#endif

		switch (id)
		{
		case 't': /* 通常 */
#ifdef SJIS
			euc2sjis(mesg);
#endif
			(void)((*angband_term[0]->text_hook)(x, y, len, (byte)col, mesg));
			strncpy(&Term->scr->c[y][x], mesg, len);
			for (i = x; i < x+len; i++)
			{
				Term->scr->a[y][i] = col;
			}
			break;

		case 'n': /* 繰り返し */
			for (i = 1; i < len; i++)
			{
				mesg[i] = mesg[0];
			}
			mesg[i] = '\0';
			(void)((*angband_term[0]->text_hook)(x, y, len, (byte)col, mesg));
			strncpy(&Term->scr->c[y][x], mesg, len);
			for (i = x; i < x+len; i++)
			{
				Term->scr->a[y][i] = col;
			}
			break;

		case 's': /* 一文字 */
			(void)((*angband_term[0]->text_hook)(x, y, 1, (byte)col, mesg));
			strncpy(&Term->scr->c[y][x], mesg, 1);
			Term->scr->a[y][x] = col;
			break;

		case 'w':
			(void)((*angband_term[0]->wipe_hook)(x, y, len));
			break;

		case 'x':
			if (x == TERM_XTRA_CLEAR) Term_clear();
			(void)((*angband_term[0]->xtra_hook)(x, 0));
			break;

		case 'c':
			(void)((*angband_term[0]->curs_hook)(x, y));
			break;
		}
	}

	fresh_queue.next++;
	if (fresh_queue.next == FRESH_QUEUE_SIZE) fresh_queue.next = 0;
	return (TRUE);
}

void browse_chuukei()
{
#ifndef MACINTOSH
	fd_set fdset;
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = WAIT;

	FD_ZERO(&fdset);
	FD_SET(sd, &fdset);

	Term_clear();
	Term_fresh();
	Term_xtra(TERM_XTRA_REACT, 0);

	while (1)
	{
		fd_set tmp_fdset;
		struct timeval tmp_tv;

		if (flush_ringbuf_client()) continue;

		tmp_fdset = fdset;
		tmp_tv = tv;

		/* ソケットにデータが来ているかどうか調べる */
		select(sd+1, &tmp_fdset, (fd_set *)NULL, (fd_set *)NULL, &tmp_tv);
		if (FD_ISSET(sd, &tmp_fdset) == 0)
		{
			Term_xtra(TERM_XTRA_FLUSH, 0);
			continue;
		}

		if (read_sock() < 0)
		{
			chuukei_client = FALSE;
		}

		/* 接続が切れた状態で書くべきデータがなくなっていたら終了 */
		if (!chuukei_client && fresh_queue.next == fresh_queue.tail ) break;
	}
#else
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = WAIT;

	Term_clear();
	Term_fresh();
	Term_xtra(TERM_XTRA_REACT, 0);

	while (1)
	{
		struct timeval tmp_tv;
		UInt32	unreadData = 0;
		int n;
		
		if (flush_ringbuf_client()) continue;

		tmp_tv = tv;

		/* ソケットにデータが来ているかどうか調べる */
		
		OTCountDataBytes(ep, &unreadData);
		if(unreadData <= 0 ){
			Term_xtra(TERM_XTRA_FLUSH, 0);
			continue;
		}
		if (read_sock() < 0)
		{
			chuukei_client = FALSE;
		}

		/* 接続が切れた状態で書くべきデータがなくなっていたら終了 */
		if (!chuukei_client && fresh_queue.next == fresh_queue.tail ) break;
	}
#endif /*MACINTOSH*/
}

#endif /* CHUUKEI */
