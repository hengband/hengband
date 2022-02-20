#include "racial/racial-util.h"
#include "io/input-key-requester.h"
#include "player-base/player-class.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"

rc_type::rc_type(PlayerType *player_ptr)
{
    this->ask = true;
    this->lvl = player_ptr->lev;
    PlayerClass pc(player_ptr);
    this->is_warrior = pc.equals(PlayerClassType::WARRIOR) || pc.equals(PlayerClassType::BERSERKER);
}

void rc_type::add_power(rpi_type& rpi, int number)
{
    rpi.number = number;
    this->power_desc.push_back(std::move(rpi));
}

void rc_type::add_power(rpi_type &rpi, PlayerMutationType flag)
{
    add_power(rpi, enum2i(flag));
}

COMMAND_CODE rc_type::power_count()
{
    return (COMMAND_CODE)this->power_desc.size();
}
