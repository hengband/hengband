/*!
 * @file stack-trace.h
 * @brief スタックトレースを取得するクラスの宣言
 *
 * プラットフォームに依存するため、プラットフォームごとに実装が必要
 *
 * 実装ファイル:
 * - Windows: main-win/stack-trace-win.cpp
 * - Linux/macOS(Unix系): main-unix/stack-trace-unix.cpp
 */

#pragma once

#include <string>
#include <vector>

namespace util {

class StackTrace {
public:
    StackTrace();
    ~StackTrace();

    std::string dump() const;

private:
    struct Frame;
    std::vector<Frame> frames;
};

}
