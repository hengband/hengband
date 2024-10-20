#include "save/monster-entity-writer.h"
#include "load/old/monster-flag-types-savefile50.h"
#include "save/save-util.h"
#include "system/monster-entity.h"
#include "util/enum-converter.h"

MonsterEntityWriter::MonsterEntityWriter(const MonsterEntity &monster)
    : monster(monster)
{
}

/*!
 * @brief モンスター情報をセーブデータに書き込む
 */
void MonsterEntityWriter::write_to_savedata() const
{
    const auto flags = this->write_monster_flags();

    wr_s16b(enum2i(this->monster.r_idx));
    wr_byte((byte)this->monster.fy);
    wr_byte((byte)this->monster.fx);
    wr_s16b((int16_t)this->monster.hp);
    wr_s16b((int16_t)this->monster.maxhp);
    wr_s16b((int16_t)this->monster.max_maxhp);
    wr_u32b(this->monster.dealt_damage);

    if (any_bits(flags, SaveDataMonsterFlagType::AP_R_IDX)) {
        wr_s16b(enum2i(this->monster.ap_r_idx));
    }

    if (any_bits(flags, SaveDataMonsterFlagType::SUB_ALIGN)) {
        wr_byte(this->monster.sub_align);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::SLEEP)) {
        wr_s16b(this->monster.mtimed.at(MonsterTimedEffect::SLEEP));
    }

    wr_byte((byte)this->monster.mspeed);
    wr_s16b(this->monster.energy_need);
    this->write_monster_info(flags);
}

uint32_t MonsterEntityWriter::write_monster_flags() const
{
    uint32_t flags = 0x00000000;
    if (!this->monster.is_original_ap()) {
        set_bits(flags, SaveDataMonsterFlagType::AP_R_IDX);
    }

    if (this->monster.sub_align) {
        set_bits(flags, SaveDataMonsterFlagType::SUB_ALIGN);
    }

    if (this->monster.is_asleep()) {
        set_bits(flags, SaveDataMonsterFlagType::SLEEP);
    }

    if (this->monster.is_accelerated()) {
        set_bits(flags, SaveDataMonsterFlagType::FAST);
    }

    if (this->monster.is_decelerated()) {
        set_bits(flags, SaveDataMonsterFlagType::SLOW);
    }

    if (this->monster.is_stunned()) {
        set_bits(flags, SaveDataMonsterFlagType::STUNNED);
    }

    if (this->monster.is_confused()) {
        set_bits(flags, SaveDataMonsterFlagType::CONFUSED);
    }

    if (this->monster.is_fearful()) {
        set_bits(flags, SaveDataMonsterFlagType::MONFEAR);
    }

    if (this->monster.target_y) {
        set_bits(flags, SaveDataMonsterFlagType::TARGET_Y);
    }

    if (this->monster.target_x) {
        set_bits(flags, SaveDataMonsterFlagType::TARGET_X);
    }

    if (this->monster.is_invulnerable()) {
        set_bits(flags, SaveDataMonsterFlagType::INVULNER);
    }

    if (this->monster.smart.any()) {
        set_bits(flags, SaveDataMonsterFlagType::SMART);
    }

    if (this->monster.exp) {
        set_bits(flags, SaveDataMonsterFlagType::EXP);
    }

    if (this->monster.mflag2.any()) {
        set_bits(flags, SaveDataMonsterFlagType::MFLAG2);
    }

    if (this->monster.is_named()) {
        set_bits(flags, SaveDataMonsterFlagType::NICKNAME);
    }

    if (this->monster.has_parent()) {
        set_bits(flags, SaveDataMonsterFlagType::PARENT);
    }

    wr_u32b(flags);
    return flags;
}

void MonsterEntityWriter::write_monster_info(uint32_t flags) const
{
    byte tmp8u;
    if (any_bits(flags, SaveDataMonsterFlagType::FAST)) {
        tmp8u = (byte)this->monster.mtimed.at(MonsterTimedEffect::FAST);
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::SLOW)) {
        tmp8u = (byte)this->monster.mtimed.at(MonsterTimedEffect::SLOW);
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::STUNNED)) {
        tmp8u = (byte)this->monster.mtimed.at(MonsterTimedEffect::STUN);
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::CONFUSED)) {
        tmp8u = (byte)this->monster.mtimed.at(MonsterTimedEffect::CONFUSION);
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::MONFEAR)) {
        tmp8u = (byte)this->monster.mtimed.at(MonsterTimedEffect::FEAR);
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::TARGET_Y)) {
        wr_s16b((int16_t)this->monster.target_y);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::TARGET_X)) {
        wr_s16b((int16_t)this->monster.target_x);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::INVULNER)) {
        tmp8u = (byte)this->monster.mtimed.at(MonsterTimedEffect::INVULNERABILITY);
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::SMART)) {
        wr_FlagGroup(this->monster.smart, wr_byte);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::EXP)) {
        wr_u32b(this->monster.exp);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::MFLAG2)) {
        wr_FlagGroup(this->monster.mflag2, wr_byte);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::NICKNAME)) {
        wr_string(this->monster.nickname);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::PARENT)) {
        wr_s16b(this->monster.parent_m_idx);
    }
}
