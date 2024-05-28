#pragma once

class PlayerHallucination {
public:
    PlayerHallucination() = default;
    ~PlayerHallucination() = default;
    PlayerHallucination(const PlayerHallucination &) = delete;
    PlayerHallucination(PlayerHallucination &&) = delete;
    PlayerHallucination &operator=(const PlayerHallucination &) = delete;
    PlayerHallucination &operator=(PlayerHallucination &&) = delete;

    short current() const;
    bool is_hallucinated() const;
    void set(short value);
    void reset();

private:
    short hallucination = 0;
};
