#include "autopick/autopick-pref-processor.h"
#include "autopick/autopick-entry.h"
#include "autopick/autopick-util.h"
#include "system/angband.h"

/*!
 * @brief Process line for auto picker/destroyer.
 */
void process_autopick_file_command(char *buf)
{
    autopick_type an_entry, *entry = &an_entry;
    int i;
    for (i = 0; buf[i]; i++) {
#ifdef JP
        if (iskanji(buf[i])) {
            i++;
            continue;
        }
#endif
        if (iswspace(buf[i]) && buf[i] != ' ')
            break;
    }

    buf[i] = 0;
    if (!autopick_new_entry(entry, buf, false))
        return;

    for (i = 0; i < max_autopick; i++) {
        if (!strcmp(entry->name, autopick_list[i].name) && entry->flag[0] == autopick_list[i].flag[0] && entry->flag[1] == autopick_list[i].flag[1]
            && entry->dice == autopick_list[i].dice && entry->bonus == autopick_list[i].bonus) {
            autopick_free_entry(entry);
            return;
        }
    }

    add_autopick_list(entry);
}
