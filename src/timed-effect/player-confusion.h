#pragma once

class PlayerConfusion {
public:
    PlayerConfusion() = default;
    ~PlayerConfusion() = default;
    PlayerConfusion(const PlayerConfusion &) = delete;
    PlayerConfusion(PlayerConfusion &&) = delete;
    PlayerConfusion &operator=(const PlayerConfusion &) = delete;
    PlayerConfusion &operator=(PlayerConfusion &&) = delete;

    short current() const;
    bool is_confused() const;
    void set(short value);
    void reset();

private:
    short confusion = 0;
};
