#pragma once

enum class MonraceId : short;
class MonraceDefinition;
class LoreTracker {
public:
    ~LoreTracker() = default;
    LoreTracker(LoreTracker &&) = delete;
    LoreTracker(const LoreTracker &) = delete;
    LoreTracker &operator=(const LoreTracker &) = delete;
    LoreTracker &operator=(LoreTracker &&) = delete;

    static LoreTracker &get_instance();
    bool is_tracking() const;
    bool is_tracking(MonraceId trackee) const;
    MonraceId get_trackee() const;
    const MonraceDefinition &get_tracking_monrace() const;
    void set_trackee(MonraceId new_monrace_id);

private:
    LoreTracker() = default;

    static LoreTracker instance;
    MonraceId monrace_id{};
};
