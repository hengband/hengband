#include "load/player-class-specific-data-loader.h"
#include "load/load-util.h"
#include "player-info/bard-data-type.h"
#include "player-info/bluemage-data-type.h"
#include "player-info/force-trainer-data-type.h"
#include "player-info/magic-eater-data-type.h"
#include "player-info/mane-data-type.h"
#include "player-info/smith-data-type.h"
#include "player-info/spell-hex-data-type.h"
#include "util/enum-converter.h"

#include <tuple>
#include <vector>

namespace {

//! 職業間で配列データを共有して使っていた時の配列長。古いセーブデータのマイグレーション用。
constexpr int OLD_SAVEFILE_MAX_SPELLS = 108;

std::tuple<std::vector<int32_t>, std::vector<byte>> load_old_savfile_magic_num()
{
    std::vector<int32_t> magic_num1(OLD_SAVEFILE_MAX_SPELLS);
    std::vector<byte> magic_num2(OLD_SAVEFILE_MAX_SPELLS);

    if (loading_savefile_version_is_older_than(9)) {
        for (auto &item : magic_num1) {
            rd_s32b(&item);
        }
        for (auto &item : magic_num2) {
            rd_byte(&item);
        }
    }

    return std::make_tuple(std::move(magic_num1), std::move(magic_num2));
}

}

void PlayerClassSpecificDataLoader::operator()(no_class_specific_data &) const
{
    if (loading_savefile_version_is_older_than(9)) {
        // マイグレーションすべきデータは無いので読み捨てる
        load_old_savfile_magic_num();
    }
}

void PlayerClassSpecificDataLoader::operator()(std::shared_ptr<smith_data_type> &smith_data) const
{
    if (loading_savefile_version_is_older_than(9)) {
        auto [magic_num1, magic_num2] = load_old_savfile_magic_num();
        for (auto i = 0; i < OLD_SAVEFILE_MAX_SPELLS; ++i) {
            if (magic_num1[i] > 0) {
                smith_data->essences[i2enum<SmithEssence>(i)] = static_cast<int16_t>(magic_num1[i]);
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
        auto [magic_num1, magic_num2] = load_old_savfile_magic_num();
        force_trainer_data->ki = magic_num1[0];
    } else {
        rd_s32b(&force_trainer_data->ki);
    }
}

void PlayerClassSpecificDataLoader::operator()(std::shared_ptr<bluemage_data_type> &bluemage_data) const
{
    if (loading_savefile_version_is_older_than(9)) {
        auto [magic_num1, magic_num2] = load_old_savfile_magic_num();
        for (int i = 0, count = std::min(enum2i(RF_ABILITY::MAX), OLD_SAVEFILE_MAX_SPELLS); i < count; ++i) {
            bluemage_data->learnt_blue_magics.set(i2enum<RF_ABILITY>(i), magic_num2[i] != 0);
        }
    } else {
        rd_FlagGroup(bluemage_data->learnt_blue_magics, rd_byte);
    }
}

void PlayerClassSpecificDataLoader::operator()(std::shared_ptr<magic_eater_data_type> &magic_eater_data) const
{
    if (loading_savefile_version_is_older_than(9)) {
        auto [magic_num1, magic_num2] = load_old_savfile_magic_num();
        auto load_old_item_group = [magic_num1 = std::move(magic_num1), magic_num2 = std::move(magic_num2)](auto &item_group, int index) {
            constexpr size_t old_item_group_size = 36;
            int offset = old_item_group_size * index;
            for (auto i = 0U; i < std::min(item_group.size(), old_item_group_size); ++i) {
                item_group[i].charge = magic_num1[offset + i];
                item_group[i].count = magic_num2[offset + i];
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
        auto [magic_num1, magic_num2] = load_old_savfile_magic_num();
        bird_data->singing_song = i2enum<realm_song_type>(magic_num1[0]);
        bird_data->interrputing_song = i2enum<realm_song_type>(magic_num1[1]);
        bird_data->singing_duration = magic_num1[2];
        bird_data->singing_song_spell_idx = magic_num2[0];
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

void PlayerClassSpecificDataLoader::operator()(std::shared_ptr<mane_data_type> &mane_data) const
{
    if (loading_savefile_version_is_older_than(9)) {
        // 古いセーブファイルのものまね師のデータは magic_num には保存されていないので読み捨てる
        load_old_savfile_magic_num();
    } else {
        int16_t count;
        rd_s16b(&count);
        for (; count > 0; --count) {
            int16_t spell, damage;
            rd_s16b(&spell);
            rd_s16b(&damage);
            mane_data->mane_list.push_back({ i2enum<RF_ABILITY>(spell), damage });
        }
    }
}

void PlayerClassSpecificDataLoader::operator()(std::shared_ptr<spell_hex_data_type> &spell_hex_data) const
{
    if (loading_savefile_version_is_older_than(9)) {
        auto [magic_num1, magic_num2] = load_old_savfile_magic_num();
        migrate_bitflag_to_flaggroup(spell_hex_data->casting_spells, magic_num1[0]);
        spell_hex_data->revenge_power = magic_num1[2];
        spell_hex_data->revenge_type = i2enum<SpellHexRevengeType>(magic_num2[1]);
        spell_hex_data->revenge_turn = magic_num2[2];
    } else {
        rd_FlagGroup(spell_hex_data->casting_spells, rd_byte);
        rd_s32b(&spell_hex_data->revenge_power);
        byte tmp8u;
        rd_byte(&tmp8u);
        spell_hex_data->revenge_type = i2enum<SpellHexRevengeType>(tmp8u);
        rd_byte(&spell_hex_data->revenge_turn);
    }
}
