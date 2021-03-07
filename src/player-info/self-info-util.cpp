#include "player-info/self-info-util.h"

self_info_type *initialize_self_info_type(self_info_type *self_ptr)
{
    self_ptr->line = 0;
    for (int i = 0; i < TR_FLAG_SIZE; i++)
        self_ptr->flags[i] = 0L;

    return self_ptr;
}
