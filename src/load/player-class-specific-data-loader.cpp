#include "load/player-class-specific-data-loader.h"
#include "load/load-util.h"
#include "player-info/bluemage-data-type.h"
#include "player-info/force-trainer-data-type.h"
#include "player-info/smith-data-type.h"
#include "player-info/spell-hex-data-type.h"
#include "util/enum-converter.h"

void PlayerClassSpecificDataLoader::operator()(std::shared_ptr<smith_data_type> &smith_data) const
{
    if (loading_savefile_version_is_older_than(9)) {
        for (auto i = 0; i < MAX_SPELLS; ++i) {
            if (this->magic_num1[i] > 0) {
                smith_data->essences[i2enum<SmithEssence>(i)] = static_cast<int16_t>(this->magic_num1[i]);
            }
        }
    } else {
        while (true) {
            int16_t essence, amount;
            rd_s16b(&essence);
            rd_s16b(&amount);
            if (essence < 0 && amount < 0) {
                break;
            }
            smith_data->essences[i2enum<SmithEssence>(essence)] = amount;
        }
    }
}

void PlayerClassSpecificDataLoader::operator()(std::shared_ptr<force_trainer_data_type> &force_trainer_data) const
{
    if (loading_savefile_version_is_older_than(9)) {
        force_trainer_data->ki = this->magic_num1[0];
    } else {
        rd_s32b(&force_trainer_data->ki);
    }
}

void PlayerClassSpecificDataLoader::operator()(std::shared_ptr<bluemage_data_type> &bluemage_data) const
{
    if (loading_savefile_version_is_older_than(9)) {
        for (int i = 0, count = std::min(enum2i(RF_ABILITY::MAX), MAX_SPELLS); i < count; ++i) {
            bluemage_data->learnt_blue_magics.set(i2enum<RF_ABILITY>(i), this->magic_num2[i] != 0);
        }
    } else {
        rd_FlagGroup(bluemage_data->learnt_blue_magics, rd_byte);
    }
}

void PlayerClassSpecificDataLoader::operator()(std::shared_ptr<spell_hex_data_type> &spell_hex_data) const
{
    if (loading_savefile_version_is_older_than(9)) {
        migrate_bitflag_to_flaggroup(spell_hex_data->casting_spells, this->magic_num1[0]);
        spell_hex_data->revenge_power = this->magic_num1[2];
        spell_hex_data->revenge_type = i2enum<SpellHexRevengeType>(this->magic_num2[1]);
        spell_hex_data->revenge_turn = this->magic_num2[2];
    } else {
        rd_FlagGroup(spell_hex_data->casting_spells, rd_byte);
        rd_s32b(&spell_hex_data->revenge_power);
        byte tmp8u;
        rd_byte(&tmp8u);
        spell_hex_data->revenge_type = i2enum<SpellHexRevengeType>(tmp8u);
        rd_byte(&spell_hex_data->revenge_turn);
    }
}
