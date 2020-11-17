#include "info-reader/vault-reader.h"
#include "main/angband-headers.h"
#include "room/rooms-vault.h"
#include "util/string-processor.h"

/*!
 * @brief Vault情報(v_info)のパース関数 /
 * Initialize the "v_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_v_info(char *buf, angband_header *head)
{
    char *s;
    static vault_type *v_ptr = NULL;

    if (buf[0] == 'N') {
        s = angband_strchr(buf + 2, ':');
        if (!s)
            return 1;

        *s++ = '\0';
        if (!*s)
            return 1;

        int i = atoi(buf + 2);
        if (i <= error_idx)
            return 4;
        if (i >= head->info_num)
            return 2;

        error_idx = i;
        v_ptr = &v_info[i];
        if (!add_name(&v_ptr->name, head, s))
            return 7;
    } else if (!v_ptr)
        return 3;
    else if (buf[0] == 'D') {
        s = buf + 2;
        if (!add_text(&v_ptr->text, head, s, FALSE))
            return 7;
    } else if (buf[0] == 'X') {
        EFFECT_ID typ, rat, hgt, wid;
        if (4 != sscanf(buf + 2, "%d:%d:%d:%d", &typ, &rat, &hgt, &wid))
            return 1;

        v_ptr->typ = (ROOM_IDX)typ;
        v_ptr->rat = (PROB)rat;
        v_ptr->hgt = (POSITION)hgt;
        v_ptr->wid = (POSITION)wid;
    } else
        return 6;

    return 0;
}
