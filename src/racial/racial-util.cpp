#include "racial/racial-util.h"
#include "io/input-key-requester.h"

rc_type *initialize_rc_type(player_type *creature_ptr, rc_type *rc_ptr)
{
    rc_ptr->num = 0;
    rc_ptr->command_code = 0;
    rc_ptr->ask = TRUE;
    rc_ptr->lvl = creature_ptr->lev;
    rc_ptr->cast = FALSE;
    rc_ptr->is_warrior = (creature_ptr->pclass == CLASS_WARRIOR || creature_ptr->pclass == CLASS_BERSERKER);
    rc_ptr->menu_line = use_menu ? 1 : 0;
    for (int i = 0; i < MAX_RACIAL_POWERS; i++) {
        strcpy(rc_ptr->power_desc[i].racial_name, "");
        rc_ptr->power_desc[i].number = 0;
    }

    return rc_ptr;
}
