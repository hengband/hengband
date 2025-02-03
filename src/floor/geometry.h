#pragma once

#include "system/angband-exceptions.h"
#include "system/angband.h"
#include "util/point-2d.h"
#include <array>
#include <span>
#include <utility>

/*!
 * @brief 視界及び光源の過渡処理配列サイズ / Maximum size of the "temp" array
 * @details We must be as large as "VIEW_MAX" and "LITE_MAX" for proper functioning
 * of "update_view()" and "update_lite()".  We must also be as large as the
 * largest illuminatable room, but no room is larger than 800 grids.  We
 * must also be large enough to allow "good enough" use as a circular queue,
 * to calculate monster flow, but note that the flow code is "paranoid".
 */
#define TEMP_MAX 2298

class Direction {
public:
    /*!
     * @brief 方向クラスのコンストラクタ
     *
     * 方向ID dir で指定した方向を示すクラスを生成する。
     *
     * 方向IDは5を中央とし以下のように定義される。
     *
     * 7 8 9
     *  \|/
     * 4-5-6
     *  /|\
     * 1 2 3
     *
     * 0: 5と同じ
     *
     * @param dir 方向ID
     */
    constexpr explicit Direction(int dir)
        : dir_(dir)
    {
        if ((dir < 0) || std::cmp_greater_equal(dir, DIR_TO_VEC.size())) {
            THROW_EXCEPTION(std::logic_error, "Invalid direction is specified!");
        }
    }

    constexpr Direction()
        : dir_(0)
    {
    }

    static constexpr std::span<const Direction> directions();
    static constexpr std::span<const Direction> directions_8();
    static constexpr std::span<const Direction> directions_4();
    static constexpr std::span<const Direction> directions_diag4();
    static constexpr std::span<const Direction> directions_reverse();
    static constexpr std::span<const Direction> directions_8_reverse();

    /*!
     * @brief 方向を示すベクトルを取得する
     *
     * ベクトルのy, xの値はその方向を示すY成分/X成分があれば1もしくは-1、なければ0を返す。
     *
     * @return 方向を示すベクトル
     */
    constexpr Pos2DVec vec() const
    {
        return DIR_TO_VEC[this->dir_];
    }

    /*!
     * @brief 方向IDを取得する
     * @return 方向ID
     */
    constexpr int dir() const noexcept
    {
        return this->dir_;
    }

private:
    /// 方向IDに対応するベクトルの定義
    static constexpr std::array<Pos2DVec, 10> DIR_TO_VEC = {
        { { 0, 0 }, { 1, -1 }, { 1, 0 }, { 1, 1 }, { 0, -1 }, { 0, 0 }, { 0, 1 }, { -1, -1 }, { -1, 0 }, { -1, 1 } }
    };

    int dir_; //<! 方向ID
};

/* 以降の定義は直接使用しないようdetail名前空間に入れておく*/
namespace detail {
/// 下・上・右・左・右下・左下・右上・左上・中央の順にDirectionクラスのインスタンスを保持する配列
constexpr std::array<Direction, 9> DIRECTIONS = {
    { Direction(2), Direction(8), Direction(6), Direction(4), Direction(3), Direction(1), Direction(9), Direction(7), Direction(5) }
};

constexpr std::array<Direction, 9> reverse_array(const std::array<Direction, 9> &arr)
{
    std::array<Direction, 9> res{};
    for (std::size_t i = 0; i < 9; ++i) {
        res[i] = arr[9 - 1 - i];
    }
    return res;
}

/// DIRECTIONSの要素を逆順にした配列
constexpr auto REVERSE_DIRECTIONS = reverse_array(DIRECTIONS);
}

/*!
 * @brief 下・上・右・左・右下・左下・右上・左上・中央 の順にDirectionクラスのオブジェクトを参照する配列を返す
 */
constexpr std::span<const Direction> Direction::directions()
{
    return detail::DIRECTIONS;
}
/*!
 * @brief 下・上・右・左・右下・左下・右上・左上 の順にDirectionクラスのオブジェクトを参照する配列を返す
 */
constexpr std::span<const Direction> Direction::directions_8()
{
    return Direction::directions().first(8);
}
/*!
 * @brief 下・上・右・左 の順にDirectionクラスのオブジェクトを参照する配列を返す
 */
constexpr std::span<const Direction> Direction::directions_4()
{
    return Direction::directions().first(4);
}
/*!
 * @brief 右下・左下・右上・左上 の順にDirectionクラスのオブジェクトを参照する配列を返す
 */
constexpr std::span<const Direction> Direction::directions_diag4()
{
    return Direction::directions().subspan(4, 4);
}

/*!
 * @brief Directions::directions() の逆順の配列を返す
 */
constexpr std::span<const Direction> Direction::directions_reverse()
{
    return detail::REVERSE_DIRECTIONS;
}

/*!
 * @brief Directions::directions_8() の逆順の配列を返す
 * @note 斜め方向を先に処理したい時に使用する
 */
constexpr std::span<const Direction> Direction::directions_8_reverse()
{
    return Direction::directions_reverse().subspan(1, 8);
}

constexpr std::array<int, 10> ddx = { { 0, -1, 0, 1, -1, 0, 1, -1, 0, 1 } }; //!< dddで定義した順にベクトルのX軸成分.
constexpr std::array<int, 10> ddy = { { 0, 1, 1, 1, 0, 0, 0, -1, -1, -1 } }; //!< dddで定義した順にベクトルのY軸成分.

//! 下方向から反時計回りに8方向への方向ベクトル配列
constexpr std::array<Pos2DVec, 8> CCW_DD = {
    { { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 } }
};

class PlayerType;
Pos2D mmove2(const Pos2D &pos_orig, const Pos2D &pos1, const Pos2D &pos2);
bool player_can_see_bold(PlayerType *player_ptr, POSITION y, POSITION x);

class MonsterEntity;
bool is_seen(PlayerType *player_ptr, MonsterEntity *m_ptr);
