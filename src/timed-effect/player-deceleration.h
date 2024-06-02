#pragma once

class PlayerDeceleration {
public:
    PlayerDeceleration() = default;
    ~PlayerDeceleration() = default;
    PlayerDeceleration(const PlayerDeceleration &) = delete;
    PlayerDeceleration(PlayerDeceleration &&) = delete;
    PlayerDeceleration &operator=(const PlayerDeceleration &) = delete;
    PlayerDeceleration &operator=(PlayerDeceleration &&) = delete;

    short current() const;
    bool is_slow() const;
    void set(short value);
    void reset();

private:
    short deceleration = 0;
};
