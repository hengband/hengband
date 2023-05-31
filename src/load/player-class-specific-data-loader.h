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
class SniperData;
struct samurai_data_type;
struct monk_data_type;
struct ninja_data_type;
struct spell_hex_data_type;

class PlayerClassSpecificDataLoader {
public:
    void operator()(no_class_specific_data &) const;
    void operator()(std::shared_ptr<spell_hex_data_type> &spell_hex_data) const;
    void operator()(std::shared_ptr<smith_data_type> &smith_data) const;
    void operator()(std::shared_ptr<force_trainer_data_type> &force_trainer_data) const;
    void operator()(std::shared_ptr<bluemage_data_type> &bluemage_data) const;
    void operator()(std::shared_ptr<magic_eater_data_type> &magic_eater_data) const;
    void operator()(std::shared_ptr<bard_data_type> &bird_data) const;
    void operator()(std::shared_ptr<mane_data_type> &mane_data) const;
    void operator()(std::shared_ptr<SniperData> &sniper_data) const;
    void operator()(std::shared_ptr<samurai_data_type> &samurai_data) const;
    void operator()(std::shared_ptr<monk_data_type> &monk_data) const;
    void operator()(std::shared_ptr<ninja_data_type> &ninja_data) const;
};
