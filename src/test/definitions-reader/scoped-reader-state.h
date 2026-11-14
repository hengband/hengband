#pragma once

#include "system/h-type.h"
#include "world/world.h"

namespace test {

/*!
 * @brief データの読み込み処理のテストのために、メッセージの出力を設定し、スコープを抜けるときに元へ戻す
 * @details テストでは端末を初期化していないため、メッセージを出さないようにする必要がある。
 *          msg_print() は時間停止中 (timewalk_m_idx が 0 以外) には何も出さないので、これを利用する。
 *          データの一覧などを入れ替えるテスト用の構造体では、メンバとして持たせて使う。
 */
class ScopedReaderState {
public:
    ScopedReaderState()
        : saved_timewalk_m_idx(AngbandWorld::get_instance().timewalk_m_idx)
    {
        AngbandWorld::get_instance().timewalk_m_idx = 1;
    }

    ~ScopedReaderState()
    {
        AngbandWorld::get_instance().timewalk_m_idx = this->saved_timewalk_m_idx;
    }

    ScopedReaderState(const ScopedReaderState &) = delete;
    ScopedReaderState &operator=(const ScopedReaderState &) = delete;

private:
    MONSTER_IDX saved_timewalk_m_idx;
};

}
