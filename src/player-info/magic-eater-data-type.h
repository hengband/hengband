#pragma once

#include <cstdint>
#include <optional>
#include <vector>

inline constexpr int EATER_ITEM_GROUP_SIZE = 256; //!< 魔道具1種あたりの最大数
inline constexpr int EATER_STAFF_BASE = 0; //!< 杖の開始番号(繰り返しコマンド用)
inline constexpr int EATER_WAND_BASE = EATER_STAFF_BASE + EATER_ITEM_GROUP_SIZE; //!< 魔法棒の開始番号(繰り返しコマンド用)
inline constexpr int EATER_ROD_BASE = EATER_WAND_BASE + EATER_ITEM_GROUP_SIZE; //!< ロッドの開始番号(繰り返しコマンド用)
inline constexpr int EATER_CHARGE = 0x10000L;
inline constexpr int EATER_ROD_CHARGE = 0x10L;

enum class ItemKindType : short;
class BaseitemKey;
struct magic_eater_data_type {
    struct magic_type {
        int charge{}; //!< 充填量 (杖/魔法棒とロッドで仕様が異なる)
        uint8_t count{}; //!< 取り込んだ回数(杖/魔法棒)もしくは本数(ロッド)
    };

    magic_eater_data_type();

    std::vector<magic_type> staves{}; //!< 杖のデータ
    std::vector<magic_type> wands{}; //!< 魔法棒のデータ
    std::vector<magic_type> rods{}; //!< ロッドのデータ
    inline static std::vector<magic_eater_data_type::magic_type> none{}; //!< いずれの魔道具でもないダミー

    std::optional<BaseitemKey> check_magic_eater_spell_repeat() const;
    std::vector<magic_type> &get_item_group(ItemKindType tval);

private:
    const std::vector<magic_type> &get_item_group(ItemKindType tval) const;
};
