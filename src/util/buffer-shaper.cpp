#include "util/buffer-shaper.h"

void shape_buffer(concptr str, int maxlen, char *tbuf, size_t bufsize)
{
    int read_pt = 0;
    int write_pt = 0;
    int line_len = 0;
    int word_punct = 0;
    char ch[3];
    ch[2] = '\0';

    while (str[read_pt]) {
#ifdef JP
        bool kinsoku = false;
        bool kanji;
#endif
        int ch_len = 1;
        ch[0] = str[read_pt];
        ch[1] = '\0';
#ifdef JP
        kanji = iskanji(ch[0]);

        if (kanji) {
            ch[1] = str[read_pt + 1];
            ch_len = 2;

            if (strcmp(ch, "。") == 0 || strcmp(ch, "、") == 0 || strcmp(ch, "ィ") == 0 || strcmp(ch, "ー") == 0) {
                kinsoku = true;
            }
        } else if (!isprint(ch[0])) {
            ch[0] = ' ';
        }
#else
        if (!isprint(ch[0])) {
            ch[0] = ' ';
        }
#endif

        if (line_len + ch_len > maxlen - 1 || str[read_pt] == '\n') {
            int word_len = read_pt - word_punct;
#ifdef JP
            if (kanji && !kinsoku) {
                /* nothing */;
            } else
#endif
                if (ch[0] == ' ' || word_len >= line_len / 2) {
                read_pt++;
            } else {
                read_pt = word_punct;
                if (str[word_punct] == ' ') {
                    read_pt++;
                }
                write_pt -= word_len;
            }

            tbuf[write_pt++] = '\0';
            line_len = 0;
            word_punct = read_pt;
            continue;
        }

        if (ch[0] == ' ') {
            word_punct = read_pt;
        }

#ifdef JP
        if (!kinsoku) {
            word_punct = read_pt;
        }
#endif

        if ((size_t)(write_pt + 3) >= bufsize) {
            break;
        }

        tbuf[write_pt++] = ch[0];
        line_len++;
        read_pt++;
#ifdef JP
        if (kanji) {
            tbuf[write_pt++] = ch[1];
            line_len++;
            read_pt++;
        }
#endif
    }

    tbuf[write_pt] = '\0';
    tbuf[write_pt + 1] = '\0';
}
