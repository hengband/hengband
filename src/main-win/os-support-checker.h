/*!
 * @brief OSやハードウェアレベルのサポート状況チェック定義
 * @author Hourier
 * @date 2025/01/08
 */

#pragma once

#include <optional>
#include <string>

class OsSupportChecker {
public:
    static std::optional<std::string> check_avx_enabled();
};
