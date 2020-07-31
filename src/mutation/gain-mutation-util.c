#include "mutation/gain-mutation-util.h"

gm_type *initialize_gm_type(gm_type *gm_ptr)
{
    gm_ptr->muta_class = NULL;
    gm_ptr->muta_which = 0;
    gm_ptr->muta_desc = "";
    gm_ptr->muta_chosen = FALSE;
    return gm_ptr;
}
