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
void MonsterLoader50::rd_monster(MonsterEntity &monster)
{
    if (h_older_than(1, 5, 0, 0)) {
        rd_monster_old(this->player_ptr, monster);
        return;
    }

    auto flags = rd_u32b();
    monster.r_idx = i2enum<MonraceId>(rd_s16b());
    monster.fy = rd_byte();
    monster.fx = rd_byte();

    monster.hp = rd_s16b();
    monster.maxhp = rd_s16b();
    monster.max_maxhp = rd_s16b();

    if (h_older_than(2, 1, 2, 1)) {
        monster.dealt_damage = 0;
    } else {
        monster.dealt_damage = rd_s32b();
    }

    monster.ap_r_idx = any_bits(flags, SaveDataMonsterFlagType::AP_R_IDX) ? i2enum<MonraceId>(rd_s16b()) : monster.r_idx;
    monster.sub_align = any_bits(flags, SaveDataMonsterFlagType::SUB_ALIGN) ? rd_byte() : 0;
    monster.mtimed[MonsterTimedEffect::SLEEP] = any_bits(flags, SaveDataMonsterFlagType::SLEEP) ? rd_s16b() : 0;
    monster.mspeed = rd_byte();
    monster.energy_need = rd_s16b();
    monster.mtimed[MonsterTimedEffect::FAST] = any_bits(flags, SaveDataMonsterFlagType::FAST) ? rd_byte() : 0;
    monster.mtimed[MonsterTimedEffect::SLOW] = any_bits(flags, SaveDataMonsterFlagType::SLOW) ? rd_byte() : 0;
    monster.mtimed[MonsterTimedEffect::STUN] = any_bits(flags, SaveDataMonsterFlagType::STUNNED) ? rd_byte() : 0;
    monster.mtimed[MonsterTimedEffect::CONFUSION] = any_bits(flags, SaveDataMonsterFlagType::CONFUSED) ? rd_byte() : 0;
    monster.mtimed[MonsterTimedEffect::FEAR] = any_bits(flags, SaveDataMonsterFlagType::MONFEAR) ? rd_byte() : 0;
    monster.target_y = any_bits(flags, SaveDataMonsterFlagType::TARGET_Y) ? rd_s16b() : 0;
    monster.target_x = any_bits(flags, SaveDataMonsterFlagType::TARGET_X) ? rd_s16b() : 0;
    monster.mtimed[MonsterTimedEffect::INVULNERABILITY] = any_bits(flags, SaveDataMonsterFlagType::INVULNER) ? rd_byte() : 0;
    monster.mflag.clear();
    monster.mflag2.clear();
    if (any_bits(flags, SaveDataMonsterFlagType::SMART)) {
        if (loading_savefile_version_is_older_than(2)) {
            auto tmp32u = rd_u32b();
            migrate_bitflag_to_flaggroup(monster.smart, tmp32u);

            // 3.0.0Alpha10以前のSM_CLONED(ビット位置22)、SM_PET(23)、SM_FRIEDLY(28)をMFLAG2に移行する
            // ビット位置の定義はなくなるので、ビット位置の値をハードコードする。
            std::bitset<32> rd_bits(tmp32u);
            monster.mflag2[MonsterConstantFlagType::CLONED] = rd_bits[22];
            monster.mflag2[MonsterConstantFlagType::PET] = rd_bits[23];
            monster.mflag2[MonsterConstantFlagType::FRIENDLY] = rd_bits[28];
            monster.smart.reset(i2enum<MonsterSmartLearnType>(22)).reset(i2enum<MonsterSmartLearnType>(23)).reset(i2enum<MonsterSmartLearnType>(28));
        } else {
            rd_FlagGroup(monster.smart, rd_byte);
        }
    } else {
        monster.smart.clear();
    }

    monster.exp = any_bits(flags, SaveDataMonsterFlagType::EXP) ? rd_u32b() : 0;
    if (any_bits(flags, SaveDataMonsterFlagType::MFLAG2)) {
        if (loading_savefile_version_is_older_than(2)) {
            auto tmp8u = rd_byte();
            constexpr auto base = enum2i(MonsterConstantFlagType::KAGE);
            migrate_bitflag_to_flaggroup(monster.mflag2, tmp8u, base, 7);
        } else {
            rd_FlagGroup(monster.mflag2, rd_byte);
        }
    }

    if (any_bits(flags, SaveDataMonsterFlagType::NICKNAME)) {
        monster.nickname = rd_string();
    } else {
        monster.nickname.clear();
    }

    monster.parent_m_idx = any_bits(flags, SaveDataMonsterFlagType::PARENT) ? rd_s16b() : 0;
}
