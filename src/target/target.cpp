#include "target/target.h"
#include "system/floor/floor-info.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "target/target-preparation.h"
#include <variant>

namespace {
/// 最後にターゲットしたもの
Target last_target = Target::none();

/// 特定のマスをターゲットする
struct TargetGrid {
    Pos2D pos = { 0, 0 }; //<! マスの座標
};

/// モンスターをターゲットする
struct TargetMonster {
    short m_idx = 0; //<! モンスターの参照インデックス
};

class IsOkay {
public:
    IsOkay(PlayerType *player_ptr)
        : player_ptr(player_ptr)
    {
    }
    bool operator()(std::monostate) const
    {
        return false;
    }
    bool operator()(const TargetGrid &) const
    {
        return true;
    }
    bool operator()(const TargetMonster &target_monster) const
    {
        return target_able(this->player_ptr, target_monster.m_idx);
    }

private:
    PlayerType *player_ptr;
};

class PositionGettor {
public:
    PositionGettor(PlayerType *player_ptr)
        : player_ptr(player_ptr)
    {
    }
    std::optional<Pos2D> operator()(std::monostate) const
    {
        return std::nullopt;
    }
    std::optional<Pos2D> operator()(const TargetGrid &target_grid) const
    {
        return target_grid.pos;
    }
    std::optional<Pos2D> operator()(const TargetMonster &target_monster) const
    {
        const auto &monster = this->player_ptr->current_floor_ptr->m_list[target_monster.m_idx];
        return monster.get_position();
    }

private:
    PlayerType *player_ptr;
};

class MonsterIndexGetter {
public:
    MonsterIndexGetter() = default;

    std::optional<short> operator()(std::monostate) const
    {
        return std::nullopt;
    }
    std::optional<short> operator()(const TargetGrid &) const
    {
        return std::nullopt;
    }
    std::optional<short> operator()(const TargetMonster &target_monster) const
    {
        return target_monster.m_idx;
    }
};
}

class Target::Impl {
public:
    Impl() = default;
    PlayerType *player_ptr;
    std::variant<std::monostate, TargetGrid, TargetMonster> target;
};

Target::Target()
    : impl(std::make_unique<Impl>())
{
}

Target::Target(const Target &other)
    : impl(std::make_unique<Impl>(*other.impl))
{
}

Target &Target::operator=(const Target &other)
{
    if (this == &other) {
        return *this;
    }
    this->impl = std::make_unique<Impl>(*other.impl);
    return *this;
}

Target::~Target() = default;

/*!
 * @brief なにもターゲットしていないインスタンスを生成する
 */
Target Target::none()
{
    return Target();
}

/*!
 * @brief 特定のマスをターゲットするインスタンスを生成する
 *
 * @param pos ターゲットするマスの座標
 * @return 生成したインスタンス
 */
Target Target::create_grid_target(PlayerType *player_ptr, const Pos2D &pos)
{
    Target target;
    target.impl->player_ptr = player_ptr;
    target.impl->target = TargetGrid{ pos };
    return target;
}

/*!
 * @brief モンスターをターゲットするインスタンスを生成する
 *
 * @param m_idx ターゲットするモンスターの参照インデックス
 * @return 生成したインスタンス
 */
Target Target::create_monster_target(PlayerType *player_ptr, short m_idx)
{
    Target target;
    target.impl->player_ptr = player_ptr;
    target.impl->target = TargetMonster{ m_idx };
    return target;
}

/*!
 * @brief 最後にターゲットしたものを設定する
 * @param target 設定するターゲット
 */
void Target::set_last_target(const Target &target)
{
    last_target = target;
}

/*!
 * @brief 最後にターゲットしたものを取得する
 * @return 最後にターゲットしたもの
 */
Target Target::get_last_target()
{
    return last_target;
}

/*!
 * @brief 最後にターゲットしたものをクリアする
 */
void Target::clear_last_target()
{
    last_target = Target::none();
}

/*!
 * @brief ターゲットが有効かどうかを取得する
 * @return
 * なにもターゲットしていない場合はfalse
 * 特定のマスをターゲットしている場合はtrue
 * モンスターをターゲットしている場合target_able()で判定した結果
 */
bool Target::is_okay() const
{
    return std::visit(IsOkay(this->impl->player_ptr), this->impl->target);
}

/*!
 * @brief ターゲットの座標を取得する
 * @return
 * なにもターゲットしていない場合はstd::nullopt
 * 特定のマスをターゲットしている場合はその座標
 * モンスターをターゲットしている場合target_able()==trueであればモンスターの座標、そうでなければstd::nullopt
 */
std::optional<Pos2D> Target::get_position() const
{
    if (!this->is_okay()) {
        return std::nullopt;
    }
    return std::visit(PositionGettor(this->impl->player_ptr), this->impl->target);
}

/*!
 * @brief ターゲットのモンスター参照IDを取得する
 * @return
 * モンスターをターゲットしていない場合はstd::nullopt
 * モンスターをターゲットしている場合はそのモンスター参照ID
 */
std::optional<short> Target::get_m_idx() const
{
    return std::visit(MonsterIndexGetter(), this->impl->target);
}
