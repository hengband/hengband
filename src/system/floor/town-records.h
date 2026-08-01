#pragma once

/*!
 * @brief 町の記録を管理するクラス
 * @author Hourier
 * @date 2026/05/29
 */

#include "util/flag-group.h"

constexpr size_t SECRET_TOWN = 5; // @details ズルの町番号.

enum class TownId {
    OUTPOST = 0,
    TELMORA = 1,
    MORIVANT = 2,
    ANGWIL = 3,
    ZUL = 4,
    RLYEH = 5,
    MAX,
};

class TownRecords {
public:
    TownRecords(TownRecords &&) = delete;
    TownRecords(const TownRecords &) = delete;
    TownRecords &operator=(const TownRecords &) = delete;
    TownRecords &operator=(TownRecords &&) = delete;

    static TownRecords &get_instance();

    bool has_visited(TownId town_id) const;
    void set_visited(TownId town_id);
    void initialize();
    size_t size() const;

    void set_ids(const EnumClassFlagGroup<TownId> &loaded_data); //!< for load only.
    EnumClassFlagGroup<TownId> get_ids() const; //!< for save only.

private:
    TownRecords() = default;

    static TownRecords instance;

    EnumClassFlagGroup<TownId> visited_ids;

    void validate_town_id(TownId town_id) const;
};
