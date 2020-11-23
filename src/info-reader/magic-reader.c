#include "info-reader/magic-reader.h"
#include "main/angband-headers.h"
#include "player/player-class.h"
#include "util/string-processor.h"

/*!
 * @brief 職業魔法情報(m_info)のパース関数 /
 * Initialize the "m_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_m_info(char *buf, angband_header *head)
{
    static player_magic *m_ptr = NULL;
    static int realm, magic_idx = 0, readable = 0;

    if (buf[0] == 'N') {
        int i = atoi(buf + 2);

        if (i <= error_idx)
            return 4;
        if (i >= head->info_num)
            return 2;

        error_idx = i;
        m_ptr = &m_info[i];
    } else if (!m_ptr) {
        return 3;
    } else if (buf[0] == 'I') {
        char *book, *stat;
        int xtra, type, first, weight;
        char *s;
        s = angband_strchr(buf + 2, ':');

        /* Verify that colon */
        if (!s)
            return 1;

        /* Nuke the colon, advance to the name */
        *s++ = '\0';

        book = buf + 2;

        if (streq(book, "SORCERY"))
            m_ptr->spell_book = TV_SORCERY_BOOK;
        else if (streq(book, "LIFE"))
            m_ptr->spell_book = TV_LIFE_BOOK;
        else if (streq(book, "MUSIC"))
            m_ptr->spell_book = TV_MUSIC_BOOK;
        else if (streq(book, "HISSATSU"))
            m_ptr->spell_book = TV_HISSATSU_BOOK;
        else if (streq(book, "NONE"))
            m_ptr->spell_book = 0;
        else
            return 5;

        stat = s;
        s = angband_strchr(s, ':');
        if (!s)
            return 1;
        *s++ = '\0';

        if (streq(stat, "STR"))
            m_ptr->spell_stat = A_STR;
        else if (streq(stat, "INT"))
            m_ptr->spell_stat = A_INT;
        else if (streq(stat, "WIS"))
            m_ptr->spell_stat = A_WIS;
        else if (streq(stat, "DEX"))
            m_ptr->spell_stat = A_DEX;
        else if (streq(stat, "CON"))
            m_ptr->spell_stat = A_CON;
        else if (streq(stat, "CHR"))
            m_ptr->spell_stat = A_CHR;
        else
            return 5;

        if (4 != sscanf(s, "%x:%d:%d:%d", (uint *)&xtra, &type, &first, &weight))
            return 1;

        m_ptr->spell_xtra = xtra;
        m_ptr->spell_type = type;
        m_ptr->spell_first = first;
        m_ptr->spell_weight = weight;
    } else if (buf[0] == 'R') {
        if (2 != sscanf(buf + 2, "%d:%d", &realm, &readable))
            return 1;

        magic_idx = 0;
    } else if (buf[0] == 'T') {
        int level, mana, fail, exp;

        if (!readable)
            return 1;
        if (4 != sscanf(buf + 2, "%d:%d:%d:%d", &level, &mana, &fail, &exp))
            return 1;

        m_ptr->info[realm][magic_idx].slevel = (PLAYER_LEVEL)level;
        m_ptr->info[realm][magic_idx].smana = (MANA_POINT)mana;
        m_ptr->info[realm][magic_idx].sfail = (PERCENTAGE)fail;
        m_ptr->info[realm][magic_idx].sexp = (EXP)exp;
        magic_idx++;
    } else
        return 6;

    return 0;
}
