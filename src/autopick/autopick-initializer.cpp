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

    autopick_list.clear();
    autopick_type entry;
    autopick_new_entry(&entry, easy_autopick_inscription, true);
    autopick_list.push_back(std::move(entry));
}
