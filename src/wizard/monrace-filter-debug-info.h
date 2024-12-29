/*!
 * @brief モンスター生成フィルタのデバッグ用統計情報定義
 * @author Hourier
 * @date 2024/12/28
 */

#pragma once

#include <string>

// デバッグ用統計情報。
class MonraceFilterDebugInfo {
public:
    MonraceFilterDebugInfo();

    void update(int probabilty, int level);
    std::string to_string() const;

private:
    int num_monrace = 0; // 重み(prob2)が正の要素数
    int min_level; // 重みが正の要素のうち最小階
    int max_level = 0; // 重みが正の要素のうち最大階
    int total_probability = 0; // 重みの総和
};
