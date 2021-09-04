#include "player-info/self-info-util.h"

self_info_type *initialize_self_info_type(self_info_type *self_ptr)
{
    self_ptr->line = 0;
    self_ptr->flags.fill(0U);

    return self_ptr;
}
