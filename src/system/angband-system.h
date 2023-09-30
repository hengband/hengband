#pragma once

class AngbandSystem {
public:
    AngbandSystem(const AngbandSystem &) = delete;
    AngbandSystem(AngbandSystem &&) = delete;
    AngbandSystem &operator=(const AngbandSystem &) = delete;
    AngbandSystem &operator=(AngbandSystem &&) = delete;

    static AngbandSystem &get_instance();
    void set_watch(bool new_status);
    bool is_watching() const;

private:
    AngbandSystem() = default;

    static AngbandSystem instance;
    bool watch_stat = false; // カジノ闘技場の観戦状態等に利用。NPCの処理の対象にならず自身もほとんどの行動ができない.
};
