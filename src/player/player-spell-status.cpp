#include "player/player-spell-status.h"
#include "player-base/player-class.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

namespace {

void set_flag_bit(BIT_FLAGS &flags, int bit_pos, bool value)
{
    if (value) {
        set_bits(flags, 1U << bit_pos);
    } else {
        reset_bits(flags, 1U << bit_pos);
    }
}

}

PlayerSpellStatus::PlayerSpellStatus(PlayerType *player_ptr)
    : player_ptr(player_ptr)
{
}

PlayerSpellStatus::Realm PlayerSpellStatus::realm1() const
{
    return Realm(this->player_ptr, true);
}

PlayerSpellStatus::Realm PlayerSpellStatus::realm2() const
{
    return Realm(this->player_ptr, false);
}

PlayerSpellStatus::Realm::Realm(PlayerType *player_ptr, bool is_realm1)
    : player_ptr(player_ptr)
    , is_realm1(is_realm1)
{
}

void PlayerSpellStatus::Realm::initialize()
{
    auto &learned = this->is_realm1 ? this->player_ptr->spell_learned1 : this->player_ptr->spell_learned2;
    auto &worked = this->is_realm1 ? this->player_ptr->spell_worked1 : this->player_ptr->spell_worked2;
    auto &forgotten = this->is_realm1 ? this->player_ptr->spell_forgotten1 : this->player_ptr->spell_forgotten2;

    const auto is_sorcerer = PlayerClass(player_ptr).equals(PlayerClassType::SORCERER);
    learned = worked = is_sorcerer ? 0xffffffffU : 0;
    forgotten = 0;

    auto is_erase_spell_id = this->is_realm1 ? [](int spell_id) { return spell_id >= 32; } : [](int spell_id) { return spell_id < 32; };
    std::erase_if(this->player_ptr->spell_order_learned, is_erase_spell_id);
}

bool PlayerSpellStatus::Realm::is_nothing_learned() const
{
    const auto learned = this->is_realm1 ? this->player_ptr->spell_learned1 : this->player_ptr->spell_learned2;
    return learned == 0;
}

bool PlayerSpellStatus::Realm::is_learned(int spell_id) const
{
    const auto learned = this->is_realm1 ? this->player_ptr->spell_learned1 : this->player_ptr->spell_learned2;
    return any_bits(learned, 1U << spell_id);
}

bool PlayerSpellStatus::Realm::is_worked(int spell_id) const
{
    const auto worked = this->is_realm1 ? this->player_ptr->spell_worked1 : this->player_ptr->spell_worked2;
    return any_bits(worked, 1U << spell_id);
}

bool PlayerSpellStatus::Realm::is_forgotten(int spell_id) const
{
    const auto forgotten = this->is_realm1 ? this->player_ptr->spell_forgotten1 : this->player_ptr->spell_forgotten2;
    return any_bits(forgotten, 1U << spell_id);
}

void PlayerSpellStatus::Realm::set_learned(int spell_id, bool value)
{
    auto &learned = this->is_realm1 ? this->player_ptr->spell_learned1 : this->player_ptr->spell_learned2;
    set_flag_bit(learned, spell_id, value);
}

void PlayerSpellStatus::Realm::set_worked(int spell_id, bool value)
{
    auto &worked = this->is_realm1 ? this->player_ptr->spell_worked1 : this->player_ptr->spell_worked2;
    set_flag_bit(worked, spell_id, value);
}

void PlayerSpellStatus::Realm::set_forgotten(int spell_id, bool value)
{
    auto &forgotten = this->is_realm1 ? this->player_ptr->spell_forgotten1 : this->player_ptr->spell_forgotten2;
    set_flag_bit(forgotten, spell_id, value);
}
