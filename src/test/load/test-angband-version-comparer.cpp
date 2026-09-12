/*!
 * @brief セーブデータのバージョン比較のテスト
 */

#include "load/angband-version-comparer.h"
#include "system/angband-system.h"
#include "system/angband-version.h"
#include "util/finalizer.h"
#include <doctest/doctest.h>

namespace {

struct VersionComparison {
    AngbandVersion loaded;
    AngbandVersion threshold;
    bool older;
};

}

TEST_CASE("h_older_than compares the loaded version in component order")
{
    auto &system = AngbandSystem::get_instance();
    const auto restore_version = util::make_finalizer([&system, saved = system.get_version()]() { system.set_version(saved); });

    // 下位桁が逆転していても、最初に異なる上位桁で判定する。
    constexpr VersionComparison cases[] = {
        { { 3, 2, 4, 6 }, { 3, 2, 4, 6 }, false },
        { { 2, 255, 255, 255 }, { 3, 0, 0, 0 }, true },
        { { 3, 0, 0, 0 }, { 2, 255, 255, 255 }, false },
        { { 3, 1, 255, 255 }, { 3, 2, 0, 0 }, true },
        { { 3, 2, 0, 0 }, { 3, 1, 255, 255 }, false },
        { { 3, 2, 3, 255 }, { 3, 2, 4, 0 }, true },
        { { 3, 2, 4, 0 }, { 3, 2, 3, 255 }, false },
        { { 3, 2, 4, 5 }, { 3, 2, 4, 6 }, true },
        { { 3, 2, 4, 7 }, { 3, 2, 4, 6 }, false },
        { { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, false },
        { { 255, 255, 255, 255 }, { 255, 255, 255, 255 }, false },
        { { 0, 0, 0, 0 }, { 0, 0, 0, 255 }, true },
        { { 0, 0, 0, 255 }, { 0, 0, 0, 0 }, false },
    };

    for (const auto &entry : cases) {
        CAPTURE(static_cast<int>(entry.loaded.major));
        CAPTURE(static_cast<int>(entry.loaded.minor));
        CAPTURE(static_cast<int>(entry.loaded.patch));
        CAPTURE(static_cast<int>(entry.loaded.extra));
        CAPTURE(static_cast<int>(entry.threshold.major));
        CAPTURE(static_cast<int>(entry.threshold.minor));
        CAPTURE(static_cast<int>(entry.threshold.patch));
        CAPTURE(static_cast<int>(entry.threshold.extra));
        system.set_version(entry.loaded);
        CHECK(h_older_than(entry.threshold.major, entry.threshold.minor, entry.threshold.patch, entry.threshold.extra) == entry.older);
    }
}

TEST_CASE("h_older_than legacy comparison gives priority to major minor and patch")
{
    auto &system = AngbandSystem::get_instance();
    const auto restore_version = util::make_finalizer([&system, saved = system.get_version()]() { system.set_version(saved); });

    constexpr VersionComparison cases[] = {
        { { 2, 255, 255, 255 }, { 3, 0, 0, 0 }, true },
        { { 3, 0, 0, 0 }, { 2, 255, 255, 0 }, false },
        { { 3, 1, 255, 255 }, { 3, 2, 0, 0 }, true },
        { { 3, 2, 0, 0 }, { 3, 1, 255, 0 }, false },
        { { 3, 2, 3, 255 }, { 3, 2, 4, 0 }, true },
        { { 3, 2, 4, 0 }, { 3, 2, 3, 0 }, false },
        { { 0, 0, 0, 255 }, { 0, 0, 0, 0 }, false },
        { { 255, 255, 255, 255 }, { 255, 255, 255, 0 }, false },
    };

    for (const auto &entry : cases) {
        CAPTURE(static_cast<int>(entry.loaded.major));
        CAPTURE(static_cast<int>(entry.loaded.minor));
        CAPTURE(static_cast<int>(entry.loaded.patch));
        CAPTURE(static_cast<int>(entry.threshold.major));
        CAPTURE(static_cast<int>(entry.threshold.minor));
        CAPTURE(static_cast<int>(entry.threshold.patch));
        system.set_version(entry.loaded);
        CHECK(h_older_than(entry.threshold.major, entry.threshold.minor, entry.threshold.patch) == entry.older);
    }
}

TEST_CASE("h_older_than legacy comparison ignores every extra version")
{
    auto &system = AngbandSystem::get_instance();
    const auto restore_version = util::make_finalizer([&system, saved = system.get_version()]() { system.set_version(saved); });

    for (int extra = 0; extra <= 255; ++extra) {
        CAPTURE(extra);
        system.set_version({ 3, 2, 4, static_cast<uint8_t>(extra) });
        CHECK_FALSE(h_older_than(3, 2, 4));
    }
}
