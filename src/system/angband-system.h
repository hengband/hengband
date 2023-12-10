#pragma once

#include <stdint.h>

class AngbandSystem {
public:
    AngbandSystem(const AngbandSystem &) = delete;
    AngbandSystem(AngbandSystem &&) = delete;
    AngbandSystem &operator=(const AngbandSystem &) = delete;
    AngbandSystem &operator=(AngbandSystem &&) = delete;

    static AngbandSystem &get_instance();
    void set_phase_out(bool new_status);
    bool is_phase_out() const;
    int get_max_range() const;

    uint8_t h_ver_major{}; //!< 変愚蛮怒バージョン(メジャー番号) / Hengband version (major ver.)
    uint8_t h_ver_minor{}; //!< 変愚蛮怒バージョン(マイナー番号) / Hengband version (minor ver.)
    uint8_t h_ver_patch{}; //!< 変愚蛮怒バージョン(パッチ番号) / Hengband version (patch ver.)
    uint8_t h_ver_extra{}; //!< 変愚蛮怒バージョン(エクストラ番号) / Hengband version (extra ver.)

private:
    AngbandSystem() = default;

    static AngbandSystem instance;
    bool phase_out_stat = false; // カジノ闘技場の観戦状態等に利用。NPCの処理の対象にならず自身もほとんどの行動ができない.
};
