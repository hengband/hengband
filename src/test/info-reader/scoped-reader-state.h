#pragma once

#include "info-reader/info-reader-util.h"
#include "system/h-type.h"
#include "world/world.h"

namespace test {

/*!
 * @brief データの読み込み処理のテストのために、エラーの位置とメッセージの出力を設定し、スコープを抜けるときに元へ戻す
 * @details 読み込み処理はエラーが起きたデータの ID を error_idx に書き、メッセージを出すことがある。
 *          テストでは端末を初期化していないため、メッセージを出さないようにする必要がある。
 *          msg_print() は時間停止中 (timewalk_m_idx が 0 以外) には何も出さないので、これを利用する。
 *          データの一覧などを入れ替えるテスト用の構造体では、メンバとして持たせて使う。
 */
class ScopedReaderState {
public:
    /*!
     * @param initial_error_idx テストの間に設定しておく error_idx の値
     */
    explicit ScopedReaderState(int initial_error_idx = -1)
        : saved_error_idx(error_idx)
        , saved_timewalk_m_idx(AngbandWorld::get_instance().timewalk_m_idx)
    {
        error_idx = initial_error_idx;
        AngbandWorld::get_instance().timewalk_m_idx = 1;
    }

    ~ScopedReaderState()
    {
        error_idx = this->saved_error_idx;
        AngbandWorld::get_instance().timewalk_m_idx = this->saved_timewalk_m_idx;
    }

    ScopedReaderState(const ScopedReaderState &) = delete;
    ScopedReaderState &operator=(const ScopedReaderState &) = delete;

private:
    int saved_error_idx;
    MONSTER_IDX saved_timewalk_m_idx;
};

}
