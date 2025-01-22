#pragma once

#include <chrono>
#include <cstdint>

class ElapsedTime {
public:
    ElapsedTime();
    explicit ElapsedTime(uint32_t current_sec);

    uint32_t elapsed_sec() const;
    void update();
    void reset();
    void pause();
    void unpause();

private:
    std::chrono::milliseconds elapsed_time;
    std::chrono::steady_clock::time_point last_update_time;
    bool is_paused = true;
};
