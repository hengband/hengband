#pragma once

#include "system/angband.h"

#include "object-enchant/tr-flags.h"
#include "system/player-type-definition.h"

#include <initializer_list>
#include <memory>
#include <variant>

enum class SamuraiStance : uint8_t;
enum class MonkStance : uint8_t;

class PlayerClass {
public:
    PlayerClass() = delete;
    PlayerClass(player_type *player_ptr);
    virtual ~PlayerClass() = default;

    TrFlags tr_flags() const;
    TrFlags form_tr_flags() const;

    bool can_resist_stun() const;
    bool is_wizard() const;

    bool lose_balance();
    void break_samurai_stance(std::initializer_list<SamuraiStance> stance_list);
    SamuraiStance get_samurai_stance() const;
    bool samurai_stance_is(SamuraiStance stance) const;
    void set_samurai_stance(SamuraiStance stance) const;

    MonkStance get_monk_stance() const;
    bool monk_stance_is(MonkStance stance) const;
    void set_monk_stance(MonkStance stance) const;

    void init_specific_data();
    template <typename T>
    std::shared_ptr<T> get_specific_data() const;

private:
    player_type *player_ptr;
};

/**
 * @brief 職業固有データへのアクセスを取得する
 * @details 事前条件: init_specifid_data を呼び出し職業固有データ領域の初期化を行っておくこと
 *
 * @tparam T 職業固有データの型
 * @return 職業固有データTへのアクセスができる std::shared_ptr<T> を返す。
 * プレイヤーが職業固有データTを使用できない職業の場合はなにも所有権を持たない std::shared_ptr<T> を返す。
 */
template <typename T>
std::shared_ptr<T> PlayerClass::get_specific_data() const
{
    if (!std::holds_alternative<std::shared_ptr<T>>(this->player_ptr->class_specific_data)) {
        return nullptr;
    }

    return std::get<std::shared_ptr<T>>(this->player_ptr->class_specific_data);
}
