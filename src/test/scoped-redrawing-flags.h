/*!
 * @file scoped-redrawing-flags.h
 * @brief テストで更新フラグ (DeferredUpdateRequests) を退避し、スコープを抜けるときに元へ戻す補助関数
 */

#pragma once

#include "system/deferred-update-requests.h"
#include "util/enum-range.h"
#include "util/finalizer.h"
#include "util/flag-group.h"

namespace test {

/*!
 * @brief 更新フラグのうち、指定した型のフラグをすべて覚えておき、スコープを抜けるときに元の状態に戻す
 * @tparam Flag フラグの列挙型
 * @return スコープを抜けるときにフラグを戻すファイナライザ
 */
template <RedrawingFlagType Flag>
[[nodiscard]] auto scoped_restore_redrawing_flags()
{
    auto &dur = DeferredUpdateRequests::get_instance();
    constexpr EnumRange range(static_cast<Flag>(0), Flag::MAX);
    const EnumClassFlagGroup<Flag> all_flags(range);
    EnumClassFlagGroup<Flag> was_set;
    for (const auto flag : range) {
        if (dur.has(flag)) {
            was_set.set(flag);
        }
    }

    return util::make_finalizer([&dur, all_flags, was_set] {
        dur.reset_flags(all_flags);
        dur.set_flags(was_set);
    });
}
}
