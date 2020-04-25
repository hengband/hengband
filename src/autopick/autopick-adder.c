#include "angband.h"
#include "autopick/autopick-util.h"
#include "autopick/autopick-adder.h"

/*
 * Add one line to autopick_list[]
 */
void add_autopick_list(autopick_type *entry)
{
	if (max_autopick >= max_max_autopick)
	{
		int old_max_max_autopick = max_max_autopick;
		autopick_type *old_autopick_list = autopick_list;
		max_max_autopick += MAX_AUTOPICK_DEFAULT;
		C_MAKE(autopick_list, max_max_autopick, autopick_type);
		(void)C_COPY(autopick_list, old_autopick_list, old_max_max_autopick, autopick_type);
		C_KILL(old_autopick_list, old_max_max_autopick, autopick_type);
	}

	autopick_list[max_autopick] = *entry;
	max_autopick++;
}
