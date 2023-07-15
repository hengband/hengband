#pragma once

#include "mind/drs-types.h"
#include "system/angband.h"
#include <functional>
#include <type_traits>
#include <unordered_map>

enum class AttributeType;
struct mspell_cast_msg_blind;
class PlayerType;

class MSpellMessageData {
public:
    MSpellMessageData();
    template <typename FUNC>
    MSpellMessageData(const FUNC &output)
        : output(output)
    {
        static_assert(std::is_invocable_r<bool, decltype(output), PlayerType *, MONSTER_IDX, MONSTER_IDX, int>::value);
    }
    MSpellMessageData(const mspell_cast_msg_blind &msg_string);
    MSpellMessageData(const std::string_view &blind, const std::string_view &to_player, const std::string_view &to_monster);
    std::function<bool(PlayerType *, MONSTER_IDX, MONSTER_IDX, int)> output;
};

class MSpellDrsData {
public:
    MSpellDrsData();
    MSpellDrsData(std::initializer_list<drs_type> drs);
    MSpellDrsData(const drs_type &drs);
    std::function<void(PlayerType *, MONSTER_IDX)> execute;
};

class MSpellData {
public:
    MSpellData();
    MSpellData(const MSpellMessageData &msg);
    MSpellData(const MSpellMessageData &msg, const AttributeType &type);
    MSpellData(const MSpellMessageData &msg, const AttributeType &type, const MSpellDrsData &drs);
    MSpellData(const MSpellData &) = default;
    MSpellData(MSpellData &&) = default;
    MSpellData &operator=(const MSpellData &) = default;
    MSpellData &operator=(MSpellData &&) = default;
    virtual ~MSpellData() = default;

    MSpellMessageData msg;
    AttributeType type;
    MSpellDrsData drs;
    bool contain;
};

template <typename T>
MSpellData get_mspell_data(std::unordered_map<T, MSpellData> map, T key)
{
    if (auto it = map.find(key); it != map.end()) {
        return it->second;
    }
    return MSpellData();
}
