#pragma once
class PlayerWideSpread {
public:
    PlayerWideSpread() = default;
    ~PlayerWideSpread() = default;
    PlayerWideSpread(const PlayerWideSpread &) = delete;
    PlayerWideSpread(PlayerWideSpread &&) = delete;
    PlayerWideSpread &operator=(const PlayerWideSpread &) = delete;
    PlayerWideSpread &operator=(PlayerWideSpread &&) = delete;

    short current() const;
    bool is_wide_spreaded() const;
    void set(short value);

private:
    short wide_spread = 0;
};
