#include "io-dump/dump-remover.h"
#include "io-dump/dump-util.h"
#include "io/read-pref-file.h"
#include "util/angband-files.h"

/*!
 * @brief prefファイルを選択して処理する /
 * Ask for a "user pref line" and process it
 * @brief prf出力内容を消去する /
 * Remove old lines automatically generated before.
 * @param orig_file 消去を行うファイル名
 */
void remove_auto_dump(concptr orig_file, concptr auto_dump_mark)
{
    char buf[1024];
    bool between_mark = false;
    bool changed = false;
    int line_num = 0;
    long header_location = 0;
    char header_mark_str[80];
    char footer_mark_str[80];

    sprintf(header_mark_str, auto_dump_header, auto_dump_mark);
    sprintf(footer_mark_str, auto_dump_footer, auto_dump_mark);
    size_t mark_len = strlen(footer_mark_str);

    FILE *orig_fff;
    orig_fff = angband_fopen(orig_file, "r");
    if (!orig_fff) {
        return;
    }

    FILE *tmp_fff = nullptr;
    char tmp_file[FILE_NAME_SIZE];
    if (!open_temporary_file(&tmp_fff, tmp_file)) {
        return;
    }

    while (true) {
        if (angband_fgets(orig_fff, buf, sizeof(buf))) {
            if (between_mark) {
                fseek(orig_fff, header_location, SEEK_SET);
                between_mark = false;
                continue;
            } else {
                break;
            }
        }

        if (!between_mark) {
            if (!strcmp(buf, header_mark_str)) {
                header_location = ftell(orig_fff);
                line_num = 0;
                between_mark = true;
                changed = true;
            } else {
                fprintf(tmp_fff, "%s\n", buf);
            }

            continue;
        }

        if (!strncmp(buf, footer_mark_str, mark_len)) {
            int tmp;
            if (!sscanf(buf + mark_len, " (%d)", &tmp) || tmp != line_num) {
                fseek(orig_fff, header_location, SEEK_SET);
            }

            between_mark = false;
            continue;
        }

        line_num++;
    }

    angband_fclose(orig_fff);
    angband_fclose(tmp_fff);

    if (changed) {
        tmp_fff = angband_fopen(tmp_file, "r");
        orig_fff = angband_fopen(orig_file, "w");
        while (!angband_fgets(tmp_fff, buf, sizeof(buf))) {
            fprintf(orig_fff, "%s\n", buf);
        }

        angband_fclose(orig_fff);
        angband_fclose(tmp_fff);
    }

    fd_kill(tmp_file);
}
