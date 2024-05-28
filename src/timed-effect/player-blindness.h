#pragma once

class PlayerBlindness {
public:
    PlayerBlindness() = default;
    ~PlayerBlindness() = default;
    PlayerBlindness(const PlayerBlindness &) = delete;
    PlayerBlindness(PlayerBlindness &&) = delete;
    PlayerBlindness &operator=(const PlayerBlindness &) = delete;
    PlayerBlindness &operator=(PlayerBlindness &&) = delete;

    short current() const;
    bool is_blind() const;
    void set(short value);
    void reset();

private:
    short blindness = 0;
};
