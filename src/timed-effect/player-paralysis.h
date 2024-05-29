#pragma once

class PlayerParalysis {
public:
    PlayerParalysis() = default;
    ~PlayerParalysis() = default;
    PlayerParalysis(const PlayerParalysis &) = delete;
    PlayerParalysis(PlayerParalysis &&) = delete;
    PlayerParalysis &operator=(const PlayerParalysis &) = delete;
    PlayerParalysis &operator=(PlayerParalysis &&) = delete;

    short current() const;
    bool is_paralyzed() const;
    void set(short value);
    void reset();

private:
    short paralysis = 0;
};
