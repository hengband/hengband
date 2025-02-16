#include "monster-floor/monster-movement-direction-list.h"
#include "floor/geometry.h"

/*!
 * @brief MonsterMoveクラスのコンストラクタ
 *
 * 移動方向のリストが空の状態のインスタンスを生成する
 *
 * @param m_idx モンスターの参照インデックス
 */
MonsterMovementDirectionList::MonsterMovementDirectionList(MONSTER_IDX m_idx)
    : m_idx(m_idx)
{
}

/*!
 * @brief ランダムに移動するように設定したMonsterMoveクラスのインスタンスを生成する
 *
 * @param m_idx モンスターの参照インデックス
 * @return 生成したインスタンス
 */
MonsterMovementDirectionList MonsterMovementDirectionList::random_move(MONSTER_IDX m_idx)
{
    MonsterMovementDirectionList mmdl(m_idx);
    mmdl.movement_directions.assign(4, Direction::self());
    return mmdl;
}

/*!
 * @brief モンスターの移動方向をリストに追加する
 * @param move 移動方向
 */
void MonsterMovementDirectionList::add_movement_direction(const Direction &dir)
{
    this->movement_directions.push_back(dir);
}

/*!
 * @brief モンスターの移動方向をリストに追加する
 * @param moves 移動方向のリスト
 */
void MonsterMovementDirectionList::add_movement_directions(std::initializer_list<Direction> dirs)
{
    this->movement_directions.insert(this->movement_directions.end(), dirs.begin(), dirs.end());
}

/*!
 * @brief モンスターの移動方向のリストを取得する
 * @return モンスターの移動方向のリスト
 */
std::span<const Direction> MonsterMovementDirectionList::get_movement_directions() const
{
    return this->movement_directions;
}

/*!
 * @brief モンスターの参照インデックスを取得する
 * @return モンスターの参照インデックス
 */
MONSTER_IDX MonsterMovementDirectionList::get_m_idx() const
{
    return this->m_idx;
}
