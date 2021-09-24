#pragma once

#include "system/angband.h"
#include "system/system-variables.h"

struct no_class_specific_data;
struct smith_data_type;
struct force_trainer_data_type;
struct bluemage_data_type;
struct spell_hex_data_type;

class PlayerClassSpecificDataLoader {
public:
    PlayerClassSpecificDataLoader(int32_t (&magic_num1)[MAX_SPELLS], byte (&magic_num2)[MAX_SPELLS])
        : magic_num1(magic_num1)
        , magic_num2(magic_num2)
    {
    }
    PlayerClassSpecificDataLoader(const PlayerClassSpecificDataLoader &) = delete;
    PlayerClassSpecificDataLoader &operator=(const PlayerClassSpecificDataLoader &) = delete;

    void operator()(no_class_specific_data &) {}
    void operator()(std::shared_ptr<spell_hex_data_type> &spell_hex_data) const;
    void operator()(std::shared_ptr<smith_data_type> &smith_data) const;
    void operator()(std::shared_ptr<force_trainer_data_type> &) const;
    void operator()(std::shared_ptr<bluemage_data_type> &bluemage_data) const;

private:
    int32_t (&magic_num1)[MAX_SPELLS];
    byte (&magic_num2)[MAX_SPELLS];
};
