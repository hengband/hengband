#pragma once

class PlayerHallucination {
public:
    PlayerHallucination() = default;

    short current() const;
    bool is_hallucinated() const;
    void set(short value);
    void reset();

private:
    short hallucination = 0;
};
