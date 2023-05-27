#pragma once

class UnixUserIds {
public:
    UnixUserIds(const UnixUserIds &) = delete;
    UnixUserIds(UnixUserIds &&) = delete;
    UnixUserIds &operator=(const UnixUserIds &) = delete;
    UnixUserIds &operator=(UnixUserIds &&) = delete;
    ~UnixUserIds() = default;

    static UnixUserIds &get_instance();
    int get_user_id() const;
    void set_user_id(const int id);
    void mod_user_id(const int increment);
    int get_effective_user_id() const;
    void set_effective_user_id(const int id);
    int get_effective_group_id() const;
    void set_effective_group_id(const int id);

private:
    UnixUserIds() = default;

    static UnixUserIds instance;
    int user_id = 0;
    int effective_user_id = 0;
    int effective_group_id = 0;
};
