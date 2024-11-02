#include "load/old/monster-loader-savefile50.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "load/old/load-v1-5-0.h"
#include "load/old/monster-flag-types-savefile50.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"

MonsterLoader50::MonsterLoader50(PlayerType *player_ptr)
    : player_ptr(player_ptr)
{
}

/*!
 * @brief モンスターを読み込む(v3.0.0 Savefile ver50まで)
 */
void MonsterLoader50::rd_monster(MonsterEntity *m_ptr_)
{
    this->m_ptr = m_ptr_;
    if (h_older_than(1, 5, 0, 0)) {
        rd_monster_old(this->m_ptr);
        return;
    }

    auto flags = rd_u32b();
    this->m_ptr->r_idx = i2enum<MonsterRaceId>(rd_s16b());
    this->m_ptr->fy = rd_byte();
    this->m_ptr->fx = rd_byte();

    this->m_ptr->hp = rd_s16b();
    this->m_ptr->maxhp = rd_s16b();
    this->m_ptr->max_maxhp = rd_s16b();

    if (h_older_than(2, 1, 2, 1)) {
        this->m_ptr->dealt_damage = 0;
    } else {
        this->m_ptr->dealt_damage = rd_s32b();
    }

    this->m_ptr->ap_r_idx = any_bits(flags, SaveDataMonsterFlagType::AP_R_IDX) ? i2enum<MonsterRaceId>(rd_s16b()) : this->m_ptr->r_idx;
    this->m_ptr->sub_align = any_bits(flags, SaveDataMonsterFlagType::SUB_ALIGN) ? rd_byte() : 0;
    this->m_ptr->mtimed[MonsterTimedEffect::SLEEP] = any_bits(flags, SaveDataMonsterFlagType::SLEEP) ? rd_s16b() : 0;
    this->m_ptr->mspeed = rd_byte();
    this->m_ptr->energy_need = rd_s16b();
    this->m_ptr->mtimed[MonsterTimedEffect::FAST] = any_bits(flags, SaveDataMonsterFlagType::FAST) ? rd_byte() : 0;
    this->m_ptr->mtimed[MonsterTimedEffect::SLOW] = any_bits(flags, SaveDataMonsterFlagType::SLOW) ? rd_byte() : 0;
    this->m_ptr->mtimed[MonsterTimedEffect::STUN] = any_bits(flags, SaveDataMonsterFlagType::STUNNED) ? rd_byte() : 0;
    this->m_ptr->mtimed[MonsterTimedEffect::CONFUSION] = any_bits(flags, SaveDataMonsterFlagType::CONFUSED) ? rd_byte() : 0;
    this->m_ptr->mtimed[MonsterTimedEffect::FEAR] = any_bits(flags, SaveDataMonsterFlagType::MONFEAR) ? rd_byte() : 0;
    this->m_ptr->target_y = any_bits(flags, SaveDataMonsterFlagType::TARGET_Y) ? rd_s16b() : 0;
    this->m_ptr->target_x = any_bits(flags, SaveDataMonsterFlagType::TARGET_X) ? rd_s16b() : 0;
    this->m_ptr->mtimed[MonsterTimedEffect::INVULNERABILITY] = any_bits(flags, SaveDataMonsterFlagType::INVULNER) ? rd_byte() : 0;
    this->m_ptr->mflag.clear();
    this->m_ptr->mflag2.clear();
    if (any_bits(flags, SaveDataMonsterFlagType::SMART)) {
        if (loading_savefile_version_is_older_than(2)) {
            auto tmp32u = rd_u32b();
            migrate_bitflag_to_flaggroup(this->m_ptr->smart, tmp32u);

            // 3.0.0Alpha10以前のSM_CLONED(ビット位置22)、SM_PET(23)、SM_FRIEDLY(28)をMFLAG2に移行する
            // ビット位置の定義はなくなるので、ビット位置の値をハードコードする。
            std::bitset<32> rd_bits(tmp32u);
            this->m_ptr->mflag2[MonsterConstantFlagType::CLONED] = rd_bits[22];
            this->m_ptr->mflag2[MonsterConstantFlagType::PET] = rd_bits[23];
            this->m_ptr->mflag2[MonsterConstantFlagType::FRIENDLY] = rd_bits[28];
            this->m_ptr->smart.reset(i2enum<MonsterSmartLearnType>(22)).reset(i2enum<MonsterSmartLearnType>(23)).reset(i2enum<MonsterSmartLearnType>(28));
        } else {
            rd_FlagGroup(this->m_ptr->smart, rd_byte);
        }
    } else {
        this->m_ptr->smart.clear();
    }

    this->m_ptr->exp = any_bits(flags, SaveDataMonsterFlagType::EXP) ? rd_u32b() : 0;
    if (any_bits(flags, SaveDataMonsterFlagType::MFLAG2)) {
        if (loading_savefile_version_is_older_than(2)) {
            auto tmp8u = rd_byte();
            constexpr auto base = enum2i(MonsterConstantFlagType::KAGE);
            migrate_bitflag_to_flaggroup(this->m_ptr->mflag2, tmp8u, base, 7);
        } else {
            rd_FlagGroup(this->m_ptr->mflag2, rd_byte);
        }
    }

    if (any_bits(flags, SaveDataMonsterFlagType::NICKNAME)) {
        this->m_ptr->nickname = rd_string();
    } else {
        this->m_ptr->nickname.clear();
    }

    this->m_ptr->parent_m_idx = any_bits(flags, SaveDataMonsterFlagType::PARENT) ? rd_s16b() : 0;
}
