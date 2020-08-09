#include "action/throw-util.h"

it_type *initialize_it_type(
    it_type *item_throw_ptr, object_type *q_ptr, const int delay_factor_val, const int mult, const bool boomerang, const OBJECT_IDX shuriken)
{
    item_throw_ptr->mult = mult;
    item_throw_ptr->boomerang = boomerang;
    item_throw_ptr->shuriken = shuriken;
    item_throw_ptr->q_ptr = q_ptr;
    item_throw_ptr->hit_body = FALSE;
    item_throw_ptr->hit_wall = FALSE;
    item_throw_ptr->equiped_item = FALSE;
    item_throw_ptr->return_when_thrown = FALSE;
    item_throw_ptr->msec = delay_factor_val * delay_factor_val * delay_factor_val;
    item_throw_ptr->come_back = FALSE;
    item_throw_ptr->do_drop = TRUE;
    return item_throw_ptr;
}
