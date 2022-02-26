#include "autopick/autopick-pref-processor.h"
#include "autopick/autopick-entry.h"
#include "autopick/autopick-util.h"
#include "system/angband.h"

/*!
 * @brief Process line for auto picker/destroyer.
 */
void process_autopick_file_command(char *buf)
{
    autopick_type entry;
    int i;
    for (i = 0; buf[i]; i++) {
#ifdef JP
        if (iskanji(buf[i])) {
            i++;
            continue;
        }
#endif
        if (iswspace(buf[i]) && buf[i] != ' ') {
            break;
        }
    }

    buf[i] = 0;
    if (!autopick_new_entry(&entry, buf, false)) {
        return;
    }

    for (const auto &item : autopick_list) {
        if ((entry.name == item.name) && entry.flag[0] == item.flag[0] && entry.flag[1] == item.flag[1] && entry.dice == item.dice && entry.bonus == item.bonus) {
            return;
        }
    }

    autopick_list.push_back(std::move(entry));
}
