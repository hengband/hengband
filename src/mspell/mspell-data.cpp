#include "mspell/mspell-data.h"
#include "effect/attribute-types.h"
#include "monster/monster-update.h"
#include "mspell/mspell-util.h"
#include "system/player-type-definition.h"

MSpellData::MSpellData()
    : msg()
    , type()
    , drs()
    , contain(false)
{
}

MSpellData::MSpellData(const MSpellMessageData &msg)
    : msg(msg)
    , type()
    , drs()
    , contain(true)
{
}

MSpellData::MSpellData(const MSpellMessageData &msg, const AttributeType &type)
    : msg(msg)
    , type(type)
    , drs()
    , contain(true)
{
}

MSpellData::MSpellData(const MSpellMessageData &msg, const AttributeType &type, const MSpellDrsData &drs)
    : msg(msg)
    , type(type)
    , drs(drs)
    , contain(true)
{
}

MSpellMessageData::MSpellMessageData()
    : output([](PlayerType *, MONSTER_IDX, MONSTER_IDX, int) { return false; })
{
}

MSpellMessageData::MSpellMessageData(const mspell_cast_msg_blind &msg_string)
    : output([msg_string](auto player_ptr, auto m_idx, auto t_idx, int target_type) {
        return monspell_message(player_ptr, m_idx, t_idx, msg_string, target_type);
    })
{
}

MSpellMessageData::MSpellMessageData(const std::string_view &blind, const std::string_view &to_player, const std::string_view &to_monster)
    : output([blind, to_player, to_monster](auto player_ptr, auto m_idx, auto t_idx, int target_type) {
        return monspell_message(player_ptr, m_idx, t_idx, { blind.data(), to_player.data(), to_monster.data() }, target_type);
    })
{
}

MSpellDrsData::MSpellDrsData()
    : execute([](PlayerType *, MONSTER_IDX) {})
{
}

MSpellDrsData::MSpellDrsData(std::initializer_list<drs_type> drs)
    : execute([drs](auto player_ptr, auto m_idx) {
        for (auto &d : drs) {
            update_smart_learn(player_ptr, m_idx, d);
        }
    })
{
}

MSpellDrsData::MSpellDrsData(const drs_type &drs)
    : execute([drs](auto player_ptr, auto m_idx) { update_smart_learn(player_ptr, m_idx, drs); })
{
}
