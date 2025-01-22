#include "util/elapsed-time.h"

using namespace std::chrono;

ElapsedTime::ElapsedTime()
    : ElapsedTime(0)
{
}

ElapsedTime::ElapsedTime(uint32_t current_sec)
    : elapsed_time(duration_cast<milliseconds>(seconds(current_sec)))
    , last_update_time(steady_clock::now())
{
}

uint32_t ElapsedTime::elapsed_sec() const
{
    const auto elapsed_sec = duration_cast<seconds>(this->elapsed_time).count();
    return static_cast<uint32_t>(elapsed_sec);
}

void ElapsedTime::update()
{
    if (this->is_paused) {
        return;
    }

    const auto now = steady_clock::now();
    const auto duration = now - this->last_update_time;

    this->elapsed_time += duration_cast<milliseconds>(duration);
    this->last_update_time = now;
}

void ElapsedTime::reset()
{
    *this = {};
}

void ElapsedTime::pause()
{
    this->is_paused = true;
}

void ElapsedTime::unpause()
{
    this->is_paused = false;
    this->last_update_time = steady_clock::now();
}
