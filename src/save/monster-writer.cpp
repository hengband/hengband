#include "save/monster-writer.h"
#include "load/old/monster-flag-types-savefile50.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "save/save-util.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"

static BIT_FLAGS write_monster_flags(const MonsterEntity &monster)
{
    BIT_FLAGS flags = 0x00000000;
    if (!monster.is_original_ap()) {
        set_bits(flags, SaveDataMonsterFlagType::AP_R_IDX);
    }

    if (monster.sub_align) {
        set_bits(flags, SaveDataMonsterFlagType::SUB_ALIGN);
    }

    if (monster.is_asleep()) {
        set_bits(flags, SaveDataMonsterFlagType::CSLEEP);
    }

    if (monster.is_accelerated()) {
        set_bits(flags, SaveDataMonsterFlagType::FAST);
    }

    if (monster.is_decelerated()) {
        set_bits(flags, SaveDataMonsterFlagType::SLOW);
    }

    if (monster.is_stunned()) {
        set_bits(flags, SaveDataMonsterFlagType::STUNNED);
    }

    if (monster.is_confused()) {
        set_bits(flags, SaveDataMonsterFlagType::CONFUSED);
    }

    if (monster.is_fearful()) {
        set_bits(flags, SaveDataMonsterFlagType::MONFEAR);
    }

    if (monster.target_y) {
        set_bits(flags, SaveDataMonsterFlagType::TARGET_Y);
    }

    if (monster.target_x) {
        set_bits(flags, SaveDataMonsterFlagType::TARGET_X);
    }

    if (monster.is_invulnerable()) {
        set_bits(flags, SaveDataMonsterFlagType::INVULNER);
    }

    if (monster.smart.any()) {
        set_bits(flags, SaveDataMonsterFlagType::SMART);
    }

    if (monster.exp) {
        set_bits(flags, SaveDataMonsterFlagType::EXP);
    }

    if (monster.mflag2.any()) {
        set_bits(flags, SaveDataMonsterFlagType::MFLAG2);
    }

    if (monster.is_named()) {
        set_bits(flags, SaveDataMonsterFlagType::NICKNAME);
    }

    if (monster.has_parent()) {
        set_bits(flags, SaveDataMonsterFlagType::PARENT);
    }

    wr_u32b(flags);
    return flags;
}

static void write_monster_info(const MonsterEntity &monster, const BIT_FLAGS flags)
{
    byte tmp8u;
    if (any_bits(flags, SaveDataMonsterFlagType::FAST)) {
        tmp8u = (byte)monster.mtimed[MTIMED_FAST];
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::SLOW)) {
        tmp8u = (byte)monster.mtimed[MTIMED_SLOW];
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::STUNNED)) {
        tmp8u = (byte)monster.mtimed[MTIMED_STUNNED];
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::CONFUSED)) {
        tmp8u = (byte)monster.mtimed[MTIMED_CONFUSED];
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::MONFEAR)) {
        tmp8u = (byte)monster.mtimed[MTIMED_MONFEAR];
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::TARGET_Y)) {
        wr_s16b((int16_t)monster.target_y);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::TARGET_X)) {
        wr_s16b((int16_t)monster.target_x);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::INVULNER)) {
        tmp8u = (byte)monster.mtimed[MTIMED_INVULNER];
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::SMART)) {
        wr_FlagGroup(monster.smart, wr_byte);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::EXP)) {
        wr_u32b(monster.exp);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::MFLAG2)) {
        wr_FlagGroup(monster.mflag2, wr_byte);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::NICKNAME)) {
        wr_string(monster.nickname);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::PARENT)) {
        wr_s16b(monster.parent_m_idx);
    }
}

/*!
 * @brief モンスター情報をセーブデータに書き込む
 * @param monster モンスター情報への参照
 */
void wr_monster(const MonsterEntity &monster)
{
    const auto flags = write_monster_flags(monster);

    wr_s16b(enum2i(monster.r_idx));
    wr_byte((byte)monster.fy);
    wr_byte((byte)monster.fx);
    wr_s16b((int16_t)monster.hp);
    wr_s16b((int16_t)monster.maxhp);
    wr_s16b((int16_t)monster.max_maxhp);
    wr_u32b(monster.dealt_damage);

    if (any_bits(flags, SaveDataMonsterFlagType::AP_R_IDX)) {
        wr_s16b(enum2i(monster.ap_r_idx));
    }

    if (any_bits(flags, SaveDataMonsterFlagType::SUB_ALIGN)) {
        wr_byte(monster.sub_align);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::CSLEEP)) {
        wr_s16b(monster.mtimed[MTIMED_CSLEEP]);
    }

    wr_byte((byte)monster.mspeed);
    wr_s16b(monster.energy_need);
    write_monster_info(monster, flags);
}

/*!
 * @brief モンスターの思い出を書き込む / Write a "lore" record
 * @param r_idx モンスター種族ID
 */
void wr_lore(MonsterRaceId r_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    wr_s16b((int16_t)r_ptr->r_sights);
    wr_s16b((int16_t)r_ptr->r_deaths);
    wr_s16b((int16_t)r_ptr->r_pkills);
    wr_s16b((int16_t)r_ptr->r_akills);
    wr_s16b((int16_t)r_ptr->r_tkills);

    wr_byte(r_ptr->r_wake);
    wr_byte(r_ptr->r_ignore);

    byte tmp8u = r_ptr->r_can_evolve ? 1 : 0;
    wr_byte(tmp8u);

    wr_byte((byte)r_ptr->r_drop_gold);
    wr_byte((byte)r_ptr->r_drop_item);

    wr_byte(0); /* unused now */
    wr_byte(r_ptr->r_cast_spell);

    wr_byte(r_ptr->r_blows[0]);
    wr_byte(r_ptr->r_blows[1]);
    wr_byte(r_ptr->r_blows[2]);
    wr_byte(r_ptr->r_blows[3]);

    wr_FlagGroup(r_ptr->r_resistance_flags, wr_byte);
    wr_FlagGroup(r_ptr->r_ability_flags, wr_byte);
    wr_FlagGroup(r_ptr->r_aura_flags, wr_byte);
    wr_FlagGroup(r_ptr->r_behavior_flags, wr_byte);
    wr_FlagGroup(r_ptr->r_kind_flags, wr_byte);
    wr_FlagGroup(r_ptr->r_drop_flags, wr_byte);
    wr_FlagGroup(r_ptr->r_feature_flags, wr_byte);
    wr_FlagGroup(r_ptr->r_special_flags, wr_byte);
    wr_FlagGroup(r_ptr->r_misc_flags, wr_byte);

    wr_byte((byte)r_ptr->max_num);
    wr_s16b(r_ptr->floor_id);

    wr_s16b(r_ptr->defeat_level);
    wr_u32b(r_ptr->defeat_time);
    wr_byte(0);
}
