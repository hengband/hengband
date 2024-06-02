#pragma once

class PlayerPoison {
public:
    PlayerPoison() = default;
    ~PlayerPoison() = default;
    PlayerPoison(const PlayerPoison &) = delete;
    PlayerPoison(PlayerPoison &&) = delete;
    PlayerPoison &operator=(const PlayerPoison &) = delete;
    PlayerPoison &operator=(PlayerPoison &&) = delete;

    short current() const;
    bool is_poisoned() const;
    void set(short value);
    void reset();

private:
    short poison = 0;
};
