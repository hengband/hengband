#pragma once

#include <string_view>
#include <tl/optional.hpp>

enum class PlayerMutationType;

/*!
 * @brief 突然変異の抽選値の区間と、獲得・喪失時のメッセージ
 */
struct MutationRollEntry {
    int max_roll; //!< この区間の抽選値の上限 (下限は1つ前の区間の上限+1)
    PlayerMutationType type; //!< 抽選される突然変異
    std::string_view gain_message; //!< 獲得したときのメッセージ
    std::string_view lose_message; //!< 失ったときのメッセージ
};

//! 突然変異の抽選値の最大値 (抽選値は1からこの値まで)
constexpr int MUTATION_ROLL_MAX = 193;

tl::optional<const MutationRollEntry &> find_mutation_by_roll(int roll);
