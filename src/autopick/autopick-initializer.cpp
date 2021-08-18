#include "autopick/autopick-initializer.h"
#include "autopick/autopick-entry.h"
#include "autopick/autopick-util.h"
#include "system/angband.h"

/*!
 * @brief Initialize the autopick
 */
void init_autopick(void)
{
    static const char easy_autopick_inscription[] = "(:=g";
    autopick_type entry;
    int i;

    if (!autopick_list) {
        max_max_autopick = MAX_AUTOPICK_DEFAULT;
        C_MAKE(autopick_list, max_max_autopick, autopick_type);
        max_autopick = 0;
    }

    for (i = 0; i < max_autopick; i++)
        autopick_free_entry(&autopick_list[i]);

    max_autopick = 0;
    autopick_new_entry(&entry, easy_autopick_inscription, true);
    autopick_list[max_autopick++] = entry;
}
