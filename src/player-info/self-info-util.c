#include "player-info/self-info-util.h"

self_info_type *initialize_self_info_type(self_info_type *si_ptr)
{
    si_ptr->line = 0;
    for (int i = 0; i < TR_FLAG_SIZE; i++)
        si_ptr->flags[i] = 0L;

    return si_ptr;
}
