/* File: japanese.c */


#include "angband.h"

#ifdef JP

/*日本語動詞活用 (打つ＞打って,打ち etc) */

#define CMPTAIL(y) strncmp(&in[l-strlen(y)],y,strlen(y))

/* 殴る,蹴る＞殴り,蹴る */
void jverb1( const char *in , char *out){
int l=strlen(in);
strcpy(out,in);

if( CMPTAIL("する")==0) sprintf(&out[l-4],"し");else
if( CMPTAIL("いる")==0) sprintf(&out[l-4],"いて");else

if( CMPTAIL("える")==0) sprintf(&out[l-4],"え");else
if( CMPTAIL("ける")==0) sprintf(&out[l-4],"け");else
if( CMPTAIL("げる")==0) sprintf(&out[l-4],"げ");else
if( CMPTAIL("せる")==0) sprintf(&out[l-4],"せ");else
if( CMPTAIL("ぜる")==0) sprintf(&out[l-4],"ぜ");else
if( CMPTAIL("てる")==0) sprintf(&out[l-4],"て");else
if( CMPTAIL("でる")==0) sprintf(&out[l-4],"で");else
if( CMPTAIL("ねる")==0) sprintf(&out[l-4],"ね");else
if( CMPTAIL("へる")==0) sprintf(&out[l-4],"へ");else
if( CMPTAIL("べる")==0) sprintf(&out[l-4],"べ");else
if( CMPTAIL("める")==0) sprintf(&out[l-4],"め");else
if( CMPTAIL("れる")==0) sprintf(&out[l-4],"れ");else

if( CMPTAIL("う")==0) sprintf(&out[l-2],"い");else
if( CMPTAIL("く")==0) sprintf(&out[l-2],"き");else
if( CMPTAIL("ぐ")==0) sprintf(&out[l-2],"ぎ");else
if( CMPTAIL("す")==0) sprintf(&out[l-2],"し");else
if( CMPTAIL("ず")==0) sprintf(&out[l-2],"じ");else
if( CMPTAIL("つ")==0) sprintf(&out[l-2],"ち");else
if( CMPTAIL("づ")==0) sprintf(&out[l-2],"ぢ");else
if( CMPTAIL("ぬ")==0) sprintf(&out[l-2],"に");else
if( CMPTAIL("ふ")==0) sprintf(&out[l-2],"ひ");else
if( CMPTAIL("ぶ")==0) sprintf(&out[l-2],"び");else
if( CMPTAIL("む")==0) sprintf(&out[l-2],"み");else
if( CMPTAIL("る")==0) sprintf(&out[l-2],"り");else

  sprintf(&out[l],"そして");}

/* 殴る,蹴る> 殴って蹴る */
void jverb2( const char *in , char *out){
int l=strlen(in);
strcpy(out,in);

if( CMPTAIL("する")==0) sprintf(&out[l-4],"して");else
if( CMPTAIL("いる")==0) sprintf(&out[l-4],"いて");else

if( CMPTAIL("える")==0) sprintf(&out[l-4],"えて");else
if( CMPTAIL("ける")==0) sprintf(&out[l-4],"けて");else
if( CMPTAIL("げる")==0) sprintf(&out[l-4],"げて");else
if( CMPTAIL("せる")==0) sprintf(&out[l-4],"せて");else
if( CMPTAIL("ぜる")==0) sprintf(&out[l-4],"ぜて");else
if( CMPTAIL("てる")==0) sprintf(&out[l-4],"てって");else
if( CMPTAIL("でる")==0) sprintf(&out[l-4],"でて");else
if( CMPTAIL("ねる")==0) sprintf(&out[l-4],"ねて");else
if( CMPTAIL("へる")==0) sprintf(&out[l-4],"へて");else
if( CMPTAIL("べる")==0) sprintf(&out[l-4],"べて");else
if( CMPTAIL("める")==0) sprintf(&out[l-4],"めて");else
if( CMPTAIL("れる")==0) sprintf(&out[l-4],"れて");else

if( CMPTAIL("う")==0) sprintf(&out[l-2],"って");else
if( CMPTAIL("く")==0) sprintf(&out[l-2],"いて");else
if( CMPTAIL("ぐ")==0) sprintf(&out[l-2],"いで");else
if( CMPTAIL("す")==0) sprintf(&out[l-2],"して");else
if( CMPTAIL("ず")==0) sprintf(&out[l-2],"じて");else
if( CMPTAIL("つ")==0) sprintf(&out[l-2],"って");else
if( CMPTAIL("づ")==0) sprintf(&out[l-2],"って");else
if( CMPTAIL("ぬ")==0) sprintf(&out[l-2],"ねて");else
if( CMPTAIL("ふ")==0) sprintf(&out[l-2],"へて");else
if( CMPTAIL("ぶ")==0) sprintf(&out[l-2],"んで");else
if( CMPTAIL("む")==0) sprintf(&out[l-2],"んで");else
if( CMPTAIL("る")==0) sprintf(&out[l-2],"って");else
  sprintf(&out[l],"ことにより");}

/* 殴る,蹴る > 殴ったり蹴ったり */
void jverb3( const char *in , char *out){
int l=strlen(in);
strcpy(out,in);

if( CMPTAIL("する")==0) sprintf(&out[l-4],"した");else
if( CMPTAIL("いる")==0) sprintf(&out[l-4],"いた");else

if( CMPTAIL("える")==0) sprintf(&out[l-4],"えた");else
if( CMPTAIL("ける")==0) sprintf(&out[l-4],"けた");else
if( CMPTAIL("げる")==0) sprintf(&out[l-4],"げた");else
if( CMPTAIL("せる")==0) sprintf(&out[l-4],"せた");else
if( CMPTAIL("ぜる")==0) sprintf(&out[l-4],"ぜた");else
if( CMPTAIL("てる")==0) sprintf(&out[l-4],"てった");else
if( CMPTAIL("でる")==0) sprintf(&out[l-4],"でた");else
if( CMPTAIL("ねる")==0) sprintf(&out[l-4],"ねた");else
if( CMPTAIL("へる")==0) sprintf(&out[l-4],"へた");else
if( CMPTAIL("べる")==0) sprintf(&out[l-4],"べた");else
if( CMPTAIL("める")==0) sprintf(&out[l-4],"めた");else
if( CMPTAIL("れる")==0) sprintf(&out[l-4],"れた");else

if( CMPTAIL("う")==0) sprintf(&out[l-2],"った");else
if( CMPTAIL("く")==0) sprintf(&out[l-2],"いた");else
if( CMPTAIL("ぐ")==0) sprintf(&out[l-2],"いだ");else
if( CMPTAIL("す")==0) sprintf(&out[l-2],"した");else
if( CMPTAIL("ず")==0) sprintf(&out[l-2],"じた");else
if( CMPTAIL("つ")==0) sprintf(&out[l-2],"った");else
if( CMPTAIL("づ")==0) sprintf(&out[l-2],"った");else
if( CMPTAIL("ぬ")==0) sprintf(&out[l-2],"ねた");else
if( CMPTAIL("ふ")==0) sprintf(&out[l-2],"へた");else
if( CMPTAIL("ぶ")==0) sprintf(&out[l-2],"んだ");else
if( CMPTAIL("む")==0) sprintf(&out[l-2],"んだ");else
if( CMPTAIL("る")==0) sprintf(&out[l-2],"った");else
  sprintf(&out[l],"ことや");}


void jverb( const char *in , char *out , int flag){
  switch (flag){
  case JVERB_AND:jverb1(in , out);break;
  case JVERB_TO :jverb2(in , out);break;
  case JVERB_OR :jverb3(in , out);break;
  }
}

char* strstr_j(const char* s, const char* t)
{
	int i, l1, l2;

	l1 = strlen(s);
	l2 = strlen(t);
	if (l1 >= l2) {
		for(i = 0; i <= l1 - l2; i++) {
			if(!strncmp(s + i, t, l2))
				return (char *)s + i;
			if (iskanji(*(s + i)))
				i++;
		}
	}
	return NULL;
}


char *strchr_j(const char *ptr, char ch)
{
	for ( ; *ptr != '\0'; ++ptr)
	{
		if (*ptr == ch) return (char *)ptr;
		if (iskanji(*ptr)) ++ptr;
	}

	return NULL;
}


/*
 * Convert SJIS string to EUC string
 */
void sjis2euc(char *str)
{
	int i;
	unsigned char c1, c2;
	unsigned char *tmp;

	int len = strlen(str);

	C_MAKE(tmp, len+1, byte);

	for (i = 0; i < len; i++)
	{
		c1 = str[i];
		if (c1 & 0x80)
		{
			i++;
			c2 = str[i];
			if (c2 >= 0x9f)
			{
				c1 = c1 * 2 - (c1 >= 0xe0 ? 0xe0 : 0x60);
				c2 += 2;
			}
			else
			{
				c1 = c1 * 2 - (c1 >= 0xe0 ? 0xe1 : 0x61);
				c2 += 0x60 + (c2 < 0x7f);
			}
			tmp[i - 1] = c1;
			tmp[i] = c2;
		}
		else
			tmp[i] = c1;
	}
	tmp[len] = 0;
	strcpy(str, (char *)tmp);

	C_KILL(tmp, len+1, byte);
}  


/*
 * Convert EUC string to SJIS string
 */
void euc2sjis(char *str)
{
	int i;
	unsigned char c1, c2;
	unsigned char *tmp;
	
	int len = strlen(str);

	C_MAKE(tmp, len+1, byte);

	for (i = 0; i < len; i++)
	{
		c1 = str[i];
		if (c1 & 0x80)
		{
			i++;
			c2 = str[i];
			if (c1 % 2)
			{
				c1 = (c1 >> 1) + (c1 < 0xdf ? 0x31 : 0x71);
				c2 -= 0x60 + (c2 < 0xe0);
			}
			else
			{
				c1 = (c1 >> 1) + (c1 < 0xdf ? 0x30 : 0x70);
				c2 -= 2;
			}

			tmp[i - 1] = c1;
			tmp[i] = c2;
		}
		else
			tmp[i] = c1;
	}
	tmp[len] = 0;
	strcpy(str, (char *)tmp);

	C_KILL(tmp, len+1, byte);
}  


/*
 * strを環境に合った文字コードに変換し、変換前の文字コードを返す。
 * strの長さに制限はない。
 *
 * 0: Unknown
 * 1: ASCII (Never known to be ASCII in this function.)
 * 2: EUC
 * 3: SJIS
 */
byte codeconv(char *str)
{
	byte code = 0;
	int i;

	for (i = 0; str[i]; i++)
	{
		unsigned char c1;
		unsigned char c2;

		/* First byte */
		c1 = str[i];

		/* ASCII? */
		if (!(c1 & 0x80)) continue;

		/* Second byte */
		i++;
		c2 = str[i];

		if (((0xa1 <= c1 && c1 <= 0xdf) || (0xfd <= c1 && c1 <= 0xfe)) &&
		    (0xa1 <= c2 && c2 <= 0xfe))
		{
			/* Only EUC is allowed */
			if (!code)
			{
				/* EUC */
				code = 2;
			}

			/* Broken string? */
			else if (code != 2)
			{
				/* No conversion */
				return 0;
			}
		}

		else if (((0x81 <= c1 && c1 <= 0x9f) &&
			  ((0x40 <= c2 && c2 <= 0x7e) || (0x80 <= c2 && c2 <= 0xfc))) ||
			 ((0xe0 <= c1 && c1 <= 0xfc) &&
			  (0x40 <= c2 && c2 <= 0x7e)))
		{
			/* Only SJIS is allowed */
			if (!code)
			{
				/* SJIS */
				code = 3;
			}

			/* Broken string? */
			else if (code != 3)
			{
				/* No conversion */
				return 0;
			}
		}
	}


	switch (code)
	{
#ifdef EUC
	case 3:
		/* SJIS -> EUC */
		sjis2euc(str);
		break;
#endif

#ifdef SJIS
	case 2:
		/* EUC -> SJIS */
		euc2sjis(str);

		break;
#endif
	}

	/* Return kanji code */
	return code;
}

/* 文字列sのxバイト目が漢字の1バイト目かどうか判定する */
bool iskanji2(cptr s, int x)
{
	int i;

	for (i = 0; i < x; i++)
	{
		if (iskanji(s[i])) i++;
	}
	if ((x == i) && iskanji(s[x])) return TRUE;

	return FALSE;
}

#endif /* JP */

