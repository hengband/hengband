#pragma once

class LoreTracker {
public:
    ~LoreTracker() = default;
    LoreTracker(LoreTracker &&) = delete;
    LoreTracker(const LoreTracker &) = delete;
    LoreTracker &operator=(const LoreTracker &) = delete;
    LoreTracker &operator=(LoreTracker &&) = delete;

    static LoreTracker &get_instance();

private:
    LoreTracker() = default;

    static LoreTracker instance;
};
