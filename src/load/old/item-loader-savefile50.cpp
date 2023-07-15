#include "load/old/item-loader-savefile50.h"
#include "artifact/fixed-art-types.h"
#include "game-option/runtime-arguments.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "load/old/item-flag-types-savefile50.h"
#include "load/old/load-v1-5-0.h"
#include "load/savedata-old-flag-types.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "object/tval-types.h"
#include "sv-definition/sv-lite-types.h"
#include "system/angband.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"

/*!
 * @brief アイテムオブジェクトを読み込む(v3.0.0 Savefile ver50まで)
 * @param o_ptr アイテムオブジェクト保存先ポインタ
 */
void ItemLoader50::rd_item(ItemEntity *o_ptr)
{
    if (h_older_than(1, 5, 0, 0)) {
        rd_item_old(o_ptr);
        return;
    }

    auto flags = rd_u32b();
    o_ptr->bi_id = rd_s16b();
    o_ptr->iy = rd_byte();
    o_ptr->ix = rd_byte();
    auto &baseitem = o_ptr->get_baseitem();
    o_ptr->bi_key = baseitem.bi_key;
    o_ptr->pval = any_bits(flags, SaveDataItemFlagType::PVAL) ? rd_s16b() : 0;
    o_ptr->discount = any_bits(flags, SaveDataItemFlagType::DISCOUNT) ? rd_byte() : 0;
    o_ptr->number = any_bits(flags, SaveDataItemFlagType::NUMBER) ? rd_byte() : 1;
    o_ptr->weight = rd_s16b();
    if (any_bits(flags, SaveDataItemFlagType::FIXED_ARTIFACT_IDX)) {
        if (h_older_than(3, 0, 0, 2)) {
            o_ptr->fixed_artifact_idx = i2enum<FixedArtifactId>(rd_byte());
        } else {
            o_ptr->fixed_artifact_idx = i2enum<FixedArtifactId>(rd_s16b());
        }
    } else {
        o_ptr->fixed_artifact_idx = FixedArtifactId::NONE;
    }

    o_ptr->ego_idx = i2enum<EgoType>(any_bits(flags, SaveDataItemFlagType::EGO_IDX) ? rd_byte() : 0);
    o_ptr->timeout = any_bits(flags, SaveDataItemFlagType::TIMEOUT) ? rd_s16b() : 0;
    o_ptr->to_h = any_bits(flags, SaveDataItemFlagType::TO_H) ? rd_s16b() : 0;
    o_ptr->to_d = any_bits(flags, SaveDataItemFlagType::TO_D) ? rd_s16b() : 0;
    o_ptr->to_a = any_bits(flags, SaveDataItemFlagType::TO_A) ? rd_s16b() : 0;
    o_ptr->ac = any_bits(flags, SaveDataItemFlagType::AC) ? rd_s16b() : 0;
    o_ptr->dd = any_bits(flags, SaveDataItemFlagType::DD) ? rd_byte() : 0;
    o_ptr->ds = any_bits(flags, SaveDataItemFlagType::DS) ? rd_byte() : 0;
    o_ptr->ident = any_bits(flags, SaveDataItemFlagType::IDENT) ? rd_byte() : 0;
    o_ptr->marked.clear();
    if (any_bits(flags, SaveDataItemFlagType::MARKED)) {
        rd_FlagGroup_bytes(o_ptr->marked, rd_byte, 1);
    }

    /* Object flags */
    if (loading_savefile_version_is_older_than(7)) {
        constexpr SavedataItemOlderThan7FlagType old_savefile_art_flags[] = {
            SavedataItemOlderThan7FlagType::ART_FLAGS0,
            SavedataItemOlderThan7FlagType::ART_FLAGS1,
            SavedataItemOlderThan7FlagType::ART_FLAGS2,
            SavedataItemOlderThan7FlagType::ART_FLAGS3,
            SavedataItemOlderThan7FlagType::ART_FLAGS4,
        };
        auto start = 0;
        for (auto f : old_savefile_art_flags) {
            if (any_bits(flags, f)) {
                auto tmp32u = rd_u32b();
                migrate_bitflag_to_flaggroup(o_ptr->art_flags, tmp32u, start);
            }
            start += 32;
        }
    } else {
        if (any_bits(flags, SaveDataItemFlagType::ART_FLAGS)) {
            rd_FlagGroup(o_ptr->art_flags, rd_byte);
        } else {
            o_ptr->art_flags.clear();
        }
    }

    if (any_bits(flags, SaveDataItemFlagType::CURSE_FLAGS)) {
        if (loading_savefile_version_is_older_than(5)) {
            auto tmp32u = rd_u32b();
            migrate_bitflag_to_flaggroup(o_ptr->curse_flags, tmp32u);
        } else {
            rd_FlagGroup(o_ptr->curse_flags, rd_byte);
        }
    } else {
        o_ptr->curse_flags.clear();
    }

    o_ptr->held_m_idx = any_bits(flags, SaveDataItemFlagType::HELD_M_IDX) ? rd_s16b() : 0;
    if (loading_savefile_version_is_older_than(12)) {
        if (any_bits(flags, SavedataItemOlderThan12FlagType::XTRA1)) {
            strip_bytes(1);
        }
    }

    if (any_bits(flags, SaveDataItemFlagType::ACTIVATION_ID)) {
        if (h_older_than(3, 0, 0, 2)) {
            o_ptr->activation_id = i2enum<RandomArtActType>(rd_byte());
        } else {
            o_ptr->activation_id = i2enum<RandomArtActType>(rd_s16b());
        }
    } else {
        o_ptr->activation_id = i2enum<RandomArtActType>(0);
    }

    // xtra3フィールドが複数目的に共用されていた頃の名残.
    const auto tval = o_ptr->bi_key.tval();
    if (loading_savefile_version_is_older_than(12)) {
        uint8_t tmp8s = any_bits(flags, SavedataItemOlderThan12FlagType::XTRA3) ? rd_byte() : 0;
        if (tval == ItemKindType::CHEST) {
            o_ptr->chest_level = tmp8s;
        } else if (tval == ItemKindType::CAPTURE) {
            o_ptr->captured_monster_speed = tmp8s;
        }
    } else {
        o_ptr->chest_level = any_bits(flags, SaveDataItemFlagType::CHEST_LEVEL) ? rd_byte() : 0;
        o_ptr->captured_monster_speed = any_bits(flags, SaveDataItemFlagType::CAPTURED_MONSTER_SPEED) ? rd_byte() : 0;
    }

    // xtra4フィールドが複数目的に共用されていた頃の名残.
    if (loading_savefile_version_is_older_than(13)) {
        int16_t xtra4 = any_bits(flags, SavedataItemOlderThan13FlagType::XTRA4) ? rd_s16b() : 0;
        if (o_ptr->is_fuel()) {
            o_ptr->fuel = static_cast<short>(xtra4);
        } else if (tval == ItemKindType::CAPTURE) {
            o_ptr->captured_monster_current_hp = xtra4;
        } else {
            o_ptr->smith_hit = static_cast<byte>(xtra4 >> 8);
            o_ptr->smith_damage = static_cast<byte>(xtra4 & 0x000f);
        }
    } else {
        o_ptr->fuel = any_bits(flags, SaveDataItemFlagType::FUEL) ? rd_s16b() : 0;
        o_ptr->captured_monster_current_hp = any_bits(flags, SaveDataItemFlagType::CAPTURED_MONSTER_CURRENT_HP) ? rd_s16b() : 0;
    }

    if (o_ptr->is_fuel() && (o_ptr->bi_key.tval() == ItemKindType::LITE)) {
        const auto fuel_max = o_ptr->bi_key.sval() == SV_LITE_TORCH ? FUEL_TORCH : FUEL_LAMP;
        if (o_ptr->fuel < 0 || o_ptr->fuel > fuel_max) {
            o_ptr->fuel = 0;
        }
    }

    o_ptr->captured_monster_max_hp = any_bits(flags, SaveDataItemFlagType::XTRA5) ? rd_s16b() : 0;
    o_ptr->feeling = any_bits(flags, SaveDataItemFlagType::FEELING) ? rd_byte() : 0;
    o_ptr->stack_idx = any_bits(flags, SaveDataItemFlagType::STACK_IDX) ? rd_s16b() : 0;
    if (any_bits(flags, SaveDataItemFlagType::SMITH) && !loading_savefile_version_is_older_than(7)) {
        if (auto tmp16s = rd_s16b(); tmp16s > 0) {
            o_ptr->smith_effect = static_cast<SmithEffectType>(tmp16s);
        }

        if (auto tmp16s = rd_s16b(); tmp16s > 0) {
            o_ptr->smith_act_idx = static_cast<RandomArtActType>(tmp16s);
        }

        if (!loading_savefile_version_is_older_than(13)) {
            o_ptr->smith_hit = rd_byte();
            o_ptr->smith_damage = rd_byte();
        }
    }

    if (any_bits(flags, SaveDataItemFlagType::INSCRIPTION)) {
        char buf[128];
        rd_string(buf, sizeof(buf));
        o_ptr->inscription.emplace(buf);
    } else {
        o_ptr->inscription.reset();
    }

    if (any_bits(flags, SaveDataItemFlagType::ART_NAME)) {
        char buf[128];
        rd_string(buf, sizeof(buf));
        o_ptr->randart_name.emplace(buf);
    } else {
        o_ptr->randart_name.reset();
    }

    if (!h_older_than(2, 1, 2, 4)) {
        return;
    }

    if ((o_ptr->ego_idx == EgoType::DARK) || (o_ptr->ego_idx == EgoType::ANCIENT_CURSE) || (o_ptr->is_specific_artifact(FixedArtifactId::NIGHT))) {
        o_ptr->art_flags.set(TR_LITE_M1);
        o_ptr->art_flags.reset(TR_LITE_1);
        o_ptr->art_flags.reset(TR_LITE_2);
        o_ptr->art_flags.reset(TR_LITE_3);
        return;
    }

    const auto sval = o_ptr->bi_key.sval();
    if (o_ptr->ego_idx == EgoType::LITE_DARKNESS) {
        if (tval != ItemKindType::LITE) {
            o_ptr->art_flags.set(TR_LITE_M1);
            return;
        }

        if (sval == SV_LITE_TORCH) {
            o_ptr->art_flags.set(TR_LITE_M1);
        } else if (sval == SV_LITE_LANTERN) {
            o_ptr->art_flags.set(TR_LITE_M2);
        } else if (sval == SV_LITE_FEANOR) {
            o_ptr->art_flags.set(TR_LITE_M3);
        }

        return;
    }

    if (tval == ItemKindType::LITE) {
        if (o_ptr->is_fixed_artifact()) {
            o_ptr->art_flags.set(TR_LITE_3);
            return;
        }

        if (sval == SV_LITE_TORCH) {
            o_ptr->art_flags.set(TR_LITE_1);
            o_ptr->art_flags.set(TR_LITE_FUEL);
            return;
        }

        if (sval == SV_LITE_LANTERN) {
            o_ptr->art_flags.set(TR_LITE_2);
            o_ptr->art_flags.set(TR_LITE_FUEL);
            return;
        }

        if (sval == SV_LITE_FEANOR) {
            o_ptr->art_flags.set(TR_LITE_2);
            return;
        }
    }
}
