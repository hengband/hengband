#include "save/player-class-specific-data-writer.h"
#include "player-info/bluemage-data-type.h"
#include "player-info/force-trainer-data-type.h"
#include "player-info/smith-data-type.h"
#include "player-info/spell-hex-data-type.h"
#include "save/save-util.h"
#include "util/enum-converter.h"

void PlayerClassSpecificDataWriter::operator()(const std::shared_ptr<smith_data_type> &smith_data) const
{
    for (auto [essence, amount] : smith_data->essences) {
        if (amount > 0) {
            wr_s16b(enum2i(essence));
            wr_s16b(amount);
        }
    }
    wr_s16b(-1);
    wr_s16b(-1);
}

void PlayerClassSpecificDataWriter::operator()(const std::shared_ptr<force_trainer_data_type> &force_trainer_data) const
{
    wr_s32b(force_trainer_data->ki);
}

void PlayerClassSpecificDataWriter::operator()(const std::shared_ptr<bluemage_data_type> &bluemage_data) const
{
    wr_FlagGroup(bluemage_data->learnt_blue_magics, wr_byte);
}

void PlayerClassSpecificDataWriter::operator()(const std::shared_ptr<spell_hex_data_type> &spell_hex_data) const
{
    wr_FlagGroup(spell_hex_data->casting_spells, wr_byte);
    wr_s32b(spell_hex_data->revenge_power);
    wr_byte(enum2i(spell_hex_data->revenge_type));
    wr_byte(spell_hex_data->revenge_turn);
}
