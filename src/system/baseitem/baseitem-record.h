#pragma once

class BaseitemRecord {
public:
    BaseitemRecord() = default;
    BaseitemRecord(const BaseitemRecord &) = delete;
    BaseitemRecord &operator=(const BaseitemRecord &) = delete;
    BaseitemRecord(BaseitemRecord &&) = default;
    BaseitemRecord &operator=(BaseitemRecord &&) = delete;

    bool is_apparent() const;
    short get_appearance_id() const;
    void set_appearance_id(short new_appearance_id);

    bool is_tried() const;
    void mark_trial(bool state);

    bool is_aware() const;
    void mark_awareness(bool state);

private:
    short appearance_id = 0; //!< 未鑑定名の何番目を当てるか(0は未鑑定名なし).
    bool aware = false; //!< ベースアイテムが鑑定済かどうか.
    bool tried = false; //!< ベースアイテムを未鑑定のまま試したことがあるか.
};
