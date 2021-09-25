#include "load/player-class-specific-data-loader.h"
#include "load/load-util.h"
#include "player-info/bard-data-type.h"
#include "player-info/bluemage-data-type.h"
#include "player-info/force-trainer-data-type.h"
#include "player-info/magic-eater-data-type.h"
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

void PlayerClassSpecificDataLoader::operator()(std::shared_ptr<magic_eater_data_type> &magic_eater_data) const
{
    if (loading_savefile_version_is_older_than(9)) {
        auto load_old_item_group = [this](auto &item_group, int index) {
            constexpr size_t old_item_group_size = 36;
            int offset = old_item_group_size * index;
            for (auto i = 0U; i < std::min(item_group.size(), old_item_group_size); ++i) {
                item_group[i].charge = this->magic_num1[offset + i];
                item_group[i].count = this->magic_num2[offset + i];
            }
        };
        load_old_item_group(magic_eater_data->staves, 0);
        load_old_item_group(magic_eater_data->wands, 1);
        load_old_item_group(magic_eater_data->rods, 2);
    } else {
        auto load_item_group = [](auto &item_group) {
            uint16_t item_count;
            rd_u16b(&item_count);
            for (auto i = 0U; i < item_count; ++i) {
                int32_t charge;
                byte count;
                rd_s32b(&charge);
                rd_byte(&count);
                if (i < item_group.size()) {
                    item_group[i].charge = charge;
                    item_group[i].count = count;
                }
            }
        };
        load_item_group(magic_eater_data->staves);
        load_item_group(magic_eater_data->wands);
        load_item_group(magic_eater_data->rods);
    }
}

void PlayerClassSpecificDataLoader::operator()(std::shared_ptr<bard_data_type> &bird_data) const
{
    if (loading_savefile_version_is_older_than(9)) {
        bird_data->singing_song = i2enum<realm_song_type>(this->magic_num1[0]);
        bird_data->interrputing_song = i2enum<realm_song_type>(this->magic_num1[1]);
        bird_data->singing_duration = this->magic_num1[2];
        bird_data->singing_song_spell_idx = this->magic_num2[0];
    } else {
        int32_t tmp32s;
        rd_s32b(&tmp32s);
        bird_data->singing_song = i2enum<realm_song_type>(tmp32s);
        rd_s32b(&tmp32s);
        bird_data->interrputing_song = i2enum<realm_song_type>(tmp32s);
        rd_s32b(&bird_data->singing_duration);
        rd_byte(&bird_data->singing_song_spell_idx);
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
