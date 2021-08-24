#include "object-hook/hook-checker.h"
#include "system/object-type-definition.h"
#include "object-enchant/special-object-flags.h"

bool object_is_valid(const object_type *o_ptr)
{
    return o_ptr->k_idx != 0;
}

bool object_is_held_monster(const object_type *o_ptr)
{
    return o_ptr->held_m_idx != 0;
}

bool object_is_broken(const object_type *o_ptr)
{
    return (o_ptr->ident & IDENT_BROKEN) != 0;
}

bool object_is_cursed(const object_type *o_ptr)
{
    return o_ptr->curse_flags.any();
}
