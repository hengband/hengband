/* File: japanese.c */


#include "angband.h"

#ifdef JP

typedef struct sindarin2j sindarin2j;

struct sindarin2j {
  char *sindarin;
  char *kana;
};

static const sindarin2j s2j_table[]= {
{"mb","nb"},{"mp","np"},{"mv","nv"},{"mm","nm"},
{"x","ks"},
{"ar$","a-ru$"},{"ir$","ia$"},{"or$","o-ru$"},
{"ra","ラ"},{"ri","リ"},{"ru","ル"},{"re","レ"},{"ro","ロ"},
{"ir","ia"},{"ur","ua"},{"er","ea"},
{"ar","aル"},
{"sha","シャ"},{"shi","シ"},{"shu","シュ"},{"she","シェ"},{"sho","ショ"},
{"tha","サ"},{"thi","シ"},{"thu","ス"},{"the","セ"},{"tho","ソ"},
{"cha","ハ"},{"chi","ヒ"},{"chu","フ"},{"che","ヘ"},{"cho","ホ"},
{"dha","ザ"},{"dhi","ジ"},{"dhu","ズ"},{"dhe","ゼ"},{"dho","ゾ"},
{"ba","バ"},{"bi","ビ"},{"bu","ブ"},{"be","ベ"},{"bo","ボ"},
{"ca","カ"},{"ci","キ"},{"cu","ク"},{"ce","ケ"},{"co","コ"},
{"da","ダ"},{"di","ディ"},{"du","ドゥ"},{"de","デ"},{"do","ド"},
{"fa","ファ"},{"fi","フィ"},{"fu","フ"},{"fe","フェ"},{"fo","フォ"},
{"ga","ガ"},{"gi","ギ"},{"gu","グ"},{"ge","ゲ"},{"go","ゴ"},
{"ha","ハ"},{"hi","ヒ"},{"hu","フ"},{"he","ヘ"},{"ho","ホ"},
{"ja","ジャ"},{"ji","ジ"},{"ju","ジュ"},{"je","ジェ"},{"jo","ジョ"},
{"ka","カ"},{"ki","キ"},{"ku","ク"},{"ke","ケ"},{"ko","コ"},
{"la","ラ"},{"li","リ"},{"lu","ル"},{"le","レ"},{"lo","ロ"},
{"ma","マ"},{"mi","ミ"},{"mu","ム"},{"me","メ"},{"mo","モ"},
{"na","ナ"},{"ni","ニ"},{"nu","ヌ"},{"ne","ネ"},{"no","ノ"},
{"pa","パ"},{"pi","ピ"},{"pu","プ"},{"pe","ペ"},{"po","ポ"},
{"qu","ク"},
{"sa","サ"},{"si","シ"},{"su","ス"},{"se","セ"},{"so","ソ"},
{"ta","タ"},{"ti","ティ"},{"tu","トゥ"},{"te","テ"},{"to","ト"},
{"va","ヴァ"},{"vi","ヴィ"},{"vu","ヴ"},{"ve","ヴェ"},{"vo","ヴォ"},
{"wa","ワ"},{"wi","ウィ"},{"wu","ウ"},{"we","ウェ"},{"wo","ウォ"},
{"ya","ヤ"},{"yu","ユ"},{"yo","ヨ"},
{"za","ザ"},{"zi","ジ"},{"zu","ズ"},{"ze","ゼ"},{"zo","ゾ"},
{"dh","ズ"},
{"ch","フ"},
{"th","ス"},
{"b","ブ"},
{"c","ク"},
{"d","ド"},
{"f","フ"},
{"g","グ"},
{"h","フ"},
{"j","ジュ"},
{"k","ク"},
{"l","ル"},
{"m","ム"},
{"n","ン"},
{"p","プ"},
{"q","ク"},
{"r","ル"},
{"s","ス"},
{"t","ト"},
{"v","ヴ"},
{"w","ウ"},
{"y","イ"},
{"a","ア"},
{"i","イ"},
{"u","ウ"},
{"e","エ"},
{"o","オ"},
{"-","ー"},
{NULL,NULL}
};

unsigned char *sindarin_to_kana( unsigned char *sindarin ){
static unsigned char buf1[256], buf2[256];
int idx;

sprintf(buf1,"%s$",sindarin);
for(idx=0;buf1[idx];idx++) if( isupper(buf1[idx]))buf1[idx]=tolower(buf1[idx]);

for(idx=0; s2j_table[idx].sindarin != NULL;idx++){
  unsigned char *pat1 = s2j_table[idx].sindarin;
  unsigned char *pat2 = s2j_table[idx].kana;
  int len=strlen(pat1);
  unsigned char *dest = buf2;
  unsigned char *src = buf1;
 
  for( ; *src ; ){
    if( strncmp( src, pat1, len)==0 ){
     strcpy( dest, pat2 );
     src+=len;
     dest+= strlen(pat2);
    }
    else
      if( iskanji(*src) ){
	*dest = *src;
	dest++; src++;
        *dest=*src;
	dest++; src++;
      }	
      else{
        *dest=*src;
	dest++; src++;
      }    
  }  
  *dest=0;
  strcpy(buf1,buf2);
}
idx=0;
while( buf1[idx] != '$' ) idx++;
buf1[idx]=0;
return(buf1);
}

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

/* 2バイト文字を考慮しながら最大 n バイト文字列をコピーする */
size_t mb_strlcpy(char *dst, const char *src, size_t size)
{
	unsigned char *d = (unsigned char*)dst;
	const unsigned char *s = (unsigned char*)src;
	size_t n = 0;

	/* reserve for NUL termination */
	size--;

	/* Copy as many bytes as will fit */
	while(n < size) {
		if (iskanji(*d)) {
			if(n + 2 >= size || !*(d+1)) break;
			*d++ = *s++;
			*d++ = *s++;
			n += 2;
		} else {
			*d++ = *s++;
			n++;
		}
	}
	*d = '\0';
	while(*s++) n++;
	return n;
}

#endif /* JP */

