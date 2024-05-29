#pragma once

class PlayerAcceleration {
public:
    PlayerAcceleration() = default;
    ~PlayerAcceleration() = default;
    PlayerAcceleration(const PlayerAcceleration &) = delete;
    PlayerAcceleration(PlayerAcceleration &&) = delete;
    PlayerAcceleration &operator=(const PlayerAcceleration &) = delete;
    PlayerAcceleration &operator=(PlayerAcceleration &&) = delete;

    short current() const;
    bool is_fast() const;
    void set(short value);
    void reset();

private:
    short acceleration = 0;
};
