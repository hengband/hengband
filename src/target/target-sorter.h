#pragma once

#include "util/point-2d.h"
#include <vector>

enum class SortKind {
    DISTANCE,
    IMPORTANCE,
};

class FloorType;
class TargetSorter {
public:
    TargetSorter(const Pos2D &p_pos, const std::vector<int> &ys, const std::vector<int> &xs, SortKind kind);
    void sort(const FloorType &floor); //!< @details フィールド変数にはしたくない (将来的にシングルトン化予定＆インクルード周りのコンパイルエラー多発)ので引数にする.
    const std::vector<int> &get_result_y() const;
    const std::vector<int> &get_result_x() const;

private:
    Pos2D p_pos; //!< プレイヤーの現在位置
    std::vector<int> ys; //!< フロアのY座標群 @todo 将来的にvector<Pos2D> へまとめる
    std::vector<int> xs; //!< フロアのX座標群 @todo 将来的にvector<Pos2D> へまとめる
    SortKind kind;

    void exe_sort(const FloorType &floor, int a, int b);
};
