#pragma once

#include "system/angband-version.h"
#include "util/rng-xoshiro.h"
#include <cstdint>
#include <string>
#include <string_view>

class AngbandSystem {
public:
    AngbandSystem(const AngbandSystem &) = delete;
    AngbandSystem(AngbandSystem &&) = delete;
    AngbandSystem &operator=(const AngbandSystem &) = delete;
    AngbandSystem &operator=(AngbandSystem &&) = delete;
    static AngbandSystem &get_instance();

    uint8_t savefile_key{}; //!< セーブファイルエンコードキー(XOR)

    void set_panic_save(bool state);
    bool is_panic_save_executed() const;
    void set_awaiting_report_score(bool state);
    bool is_awaiting_report_status() const;
    void set_phase_out(bool new_status);
    bool is_phase_out() const;
    int get_max_range() const;
    uint32_t get_seed_flavor() const;
    void set_seed_flavor(const uint32_t seed);
    uint32_t get_seed_town() const;
    void set_seed_town(const uint32_t seed);
    Xoshiro128StarStar &get_rng();
    void set_rng(const Xoshiro128StarStar &rng_);
    AngbandVersion &get_version();
    const AngbandVersion &get_version() const;
    void set_version(const AngbandVersion &new_version);
    std::string build_version_expression(VersionExpression expression) const;

private:
    AngbandSystem() = default;

    static AngbandSystem instance;

    bool panic_save = false;
    bool awaiting_report_score = false;
    bool phase_out_stat = false; // カジノ闘技場の観戦状態等に利用。NPCの処理の対象にならず自身もほとんどの行動ができない.
    Xoshiro128StarStar rng; //!< Uniform random bit generator for <random>
    uint32_t seed_flavor{}; /* アイテム未鑑定名をシャッフルするための乱数シード */
    uint32_t seed_town{}; /* ランダム生成される町をレイアウトするための乱数シード */
    AngbandVersion version{};
};
