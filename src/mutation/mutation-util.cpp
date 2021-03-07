#include "mutation/mutation-util.h"

glm_type *initialize_glm_type(glm_type *gm_ptr, MUTATION_IDX choose_mut)
{
    gm_ptr->muta_class = NULL;
    gm_ptr->muta_which = 0;
    gm_ptr->muta_desc = "";
    gm_ptr->muta_chosen = FALSE;
    gm_ptr->choose_mut = choose_mut;
    return gm_ptr;
}
