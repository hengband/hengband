#pragma once

class ItemEntity;
class BaseitemTracker {
public:
    ~BaseitemTracker() = default;
    BaseitemTracker(BaseitemTracker &&) = delete;
    BaseitemTracker(const BaseitemTracker &) = delete;
    BaseitemTracker &operator=(const BaseitemTracker &) = delete;
    BaseitemTracker &operator=(BaseitemTracker &&) = delete;

    static BaseitemTracker &get_instance();
    bool is_tracking() const;
    ItemEntity get_trackee() const;
    void set_trackee(short new_bi_id);

private:
    BaseitemTracker() = default;

    static BaseitemTracker instance;
    short bi_id = 0;
};
