#pragma once

class HealthBarTracker {
public:
    ~HealthBarTracker() = default;
    HealthBarTracker(HealthBarTracker &&) = delete;
    HealthBarTracker(const HealthBarTracker &) = delete;
    HealthBarTracker &operator=(const HealthBarTracker &) = delete;
    HealthBarTracker &operator=(HealthBarTracker &&) = delete;

    static HealthBarTracker &get_instance();
    bool is_tracking() const;
    bool is_tracking(short m_idx) const;
    short get_trackee() const;
    void set_trackee(short m_idx);
    void set_flag_if_tracking(short m_idx) const;

private:
    HealthBarTracker() = default;

    static HealthBarTracker instance;
    short tracking_m_idx;
};
