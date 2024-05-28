#pragma once

class PlayerFear {
public:
    PlayerFear() = default;
    ~PlayerFear() = default;
    PlayerFear(const PlayerFear &) = delete;
    PlayerFear(PlayerFear &&) = delete;
    PlayerFear &operator=(const PlayerFear &) = delete;
    PlayerFear &operator=(PlayerFear &&) = delete;

    short current() const;
    bool is_fearful() const;
    void set(short value);
    void reset();

private:
    short fear = 0;
};
