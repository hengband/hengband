#pragma once

#include <memory>

struct no_class_specific_data;
struct spell_hex_data_type;
struct smith_data_type;
struct force_trainer_data_type;
struct bluemage_data_type;

class PlayerClassSpecificDataWriter {
public:
    void operator()(const no_class_specific_data &) const {}
    void operator()(const std::shared_ptr<spell_hex_data_type> &spell_hex_data) const;
    void operator()(const std::shared_ptr<smith_data_type> &smith_data) const;
    void operator()(const std::shared_ptr<force_trainer_data_type> &force_trainer_data) const;
    void operator()(const std::shared_ptr<bluemage_data_type> &bluemage_data) const;
};
