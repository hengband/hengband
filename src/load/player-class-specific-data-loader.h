#pragma once

#include "system/angband.h"
#include "system/system-variables.h"

struct no_class_specific_data;
struct smith_data_type;
struct force_trainer_data_type;
struct bluemage_data_type;
struct magic_eater_data_type;
struct bard_data_type;
struct mane_data_type;
struct spell_hex_data_type;

class PlayerClassSpecificDataLoader {
public:
    void operator()(no_class_specific_data &) const;
    void operator()(std::shared_ptr<spell_hex_data_type> &spell_hex_data) const;
    void operator()(std::shared_ptr<smith_data_type> &smith_data) const;
    void operator()(std::shared_ptr<force_trainer_data_type> &) const;
    void operator()(std::shared_ptr<bluemage_data_type> &bluemage_data) const;
    void operator()(std::shared_ptr<magic_eater_data_type> &magic_eater_data) const;
    void operator()(std::shared_ptr<bard_data_type> &bird_data) const;
    void operator()(std::shared_ptr<mane_data_type> &mane_data) const;
};
