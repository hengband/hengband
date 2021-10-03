#include "save/player-class-specific-data-writer.h"
#include "player-info/bard-data-type.h"
#include "player-info/bluemage-data-type.h"
#include "player-info/force-trainer-data-type.h"
#include "player-info/magic-eater-data-type.h"
#include "player-info/mane-data-type.h"
#include "player-info/monk-data-type.h"
#include "player-info/ninja-data-type.h"
#include "player-info/samurai-data-type.h"
#include "player-info/smith-data-type.h"
#include "player-info/sniper-data-type.h"
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

void PlayerClassSpecificDataWriter::operator()(const std::shared_ptr<magic_eater_data_type> &magic_eater_data) const
{
    auto write_item_group = [](const auto &item_group) {
        wr_u16b(static_cast<uint16_t>(item_group.size()));
        for (const auto &item : item_group) {
            wr_s32b(item.charge);
            wr_byte(item.count);
        }
    };
    write_item_group(magic_eater_data->staves);
    write_item_group(magic_eater_data->wands);
    write_item_group(magic_eater_data->rods);
}

void PlayerClassSpecificDataWriter::operator()(const std::shared_ptr<bard_data_type> &bird_data) const
{
    wr_s32b(enum2i(bird_data->singing_song));
    wr_s32b(enum2i(bird_data->interrputing_song));
    wr_s32b(bird_data->singing_duration);
    wr_byte(bird_data->singing_song_spell_idx);
}

void PlayerClassSpecificDataWriter::operator()(const std::shared_ptr<mane_data_type> &mane_data) const
{
    wr_s16b(static_cast<int16_t>(mane_data->mane_list.size()));

    for (const auto &mane : mane_data->mane_list) {
        wr_s16b(static_cast<int16_t>(mane.spell));
        wr_s16b(static_cast<int16_t>(mane.damage));
    }
}

void PlayerClassSpecificDataWriter::operator()(const std::shared_ptr<sniper_data_type> &sniper_data) const
{
    wr_s16b(sniper_data->concent);
}

void PlayerClassSpecificDataWriter::operator()(const std::shared_ptr<samurai_data_type> &samurai_data) const
{
    wr_byte(enum2i(samurai_data->stance));
}

void PlayerClassSpecificDataWriter::operator()(const std::shared_ptr<monk_data_type> &monk_data) const
{
    wr_byte(enum2i(monk_data->stance));
}

void PlayerClassSpecificDataWriter::operator()(const std::shared_ptr<ninja_data_type> &ninja_data) const
{
    wr_byte(ninja_data->kawarimi ? 1 : 0);
    wr_byte(ninja_data->s_stealth ? 1 : 0);
}

void PlayerClassSpecificDataWriter::operator()(const std::shared_ptr<spell_hex_data_type> &spell_hex_data) const
{
    wr_FlagGroup(spell_hex_data->casting_spells, wr_byte);
    wr_s32b(spell_hex_data->revenge_power);
    wr_byte(enum2i(spell_hex_data->revenge_type));
    wr_byte(spell_hex_data->revenge_turn);
}
