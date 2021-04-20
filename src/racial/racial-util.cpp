#include "racial/racial-util.h"
#include "io/input-key-requester.h"

rc_type::rc_type(player_type *creature_ptr)
{
    this->ask = true;
    this->lvl = creature_ptr->lev;
    this->is_warrior = (creature_ptr->pclass == CLASS_WARRIOR || creature_ptr->pclass == CLASS_BERSERKER);
    this->menu_line = use_menu ? 1 : 0;
}

rpi_type rc_type::make_power(std::string name)
{
    auto rpi = rpi_type();
    rpi.racial_name = name;
    return rpi;
}

void rc_type::add_power(rpi_type& rpi, int number)
{
    rpi.number = number;
    this->power_desc.emplace_back(rpi);
}

COMMAND_CODE rc_type::size()
{
    return (COMMAND_CODE)this->power_desc.size();
}
