#pragma once

class InnerGameData {
public:
    InnerGameData(InnerGameData &&) = delete;
    InnerGameData(const InnerGameData &) = delete;
    InnerGameData &operator=(const InnerGameData &) = delete;
    InnerGameData &operator=(InnerGameData &&) = delete;
    ~InnerGameData() = default;
    static InnerGameData &get_instance();

private:
    InnerGameData() = default;
    static InnerGameData instance;
};
