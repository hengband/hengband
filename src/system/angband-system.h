#pragma once

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

    uint8_t version_major{}; //!< 変愚蛮怒バージョン(メジャー番号)
    uint8_t version_minor{}; //!< 変愚蛮怒バージョン(マイナー番号)
    uint8_t version_patch{}; //!< 変愚蛮怒バージョン(パッチ番号)
    uint8_t version_extra{}; //!< 変愚蛮怒バージョン(エクストラ番号)

    uint8_t savefile_key{}; //!< セーブファイルエンコードキー(XOR)

    void set_phase_out(bool new_status);
    bool is_phase_out() const;
    int get_max_range() const;
    uint32_t get_seed_flavor() const;
    void set_seed_flavor(const uint32_t seed);
    uint32_t get_seed_town() const;
    void set_seed_town(const uint32_t seed);
    Xoshiro128StarStar &get_rng();
    void set_rng(const Xoshiro128StarStar &rng_);

private:
    AngbandSystem() = default;

    static AngbandSystem instance;
    bool phase_out_stat = false; // カジノ闘技場の観戦状態等に利用。NPCの処理の対象にならず自身もほとんどの行動ができない.
    Xoshiro128StarStar rng; //!< Uniform random bit generator for <random>
    uint32_t seed_flavor{}; /* アイテム未鑑定名をシャッフルするための乱数シード */
    uint32_t seed_town{}; /* ランダム生成される町をレイアウトするための乱数シード */
};
