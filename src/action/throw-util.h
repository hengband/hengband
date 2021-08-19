#pragma once
/*!
 * @file throw-util.h
 * @brief 投擲処理関連ヘッダ
 */

#include "system/angband.h"
#include "system/system-variables.h"

// Item Throw.
struct grid_type;
struct monster_type;
struct object_type;
struct player_type;
struct it_type {
public:
    it_type() = default;
    it_type(player_type *creature_ptr, object_type *q_ptr, const int delay_factor_val, const int mult, const bool boomerang, const OBJECT_IDX shuriken);
    virtual ~it_type() = default;
    object_type *q_ptr;
    int mult;
    int msec;
    bool boomerang;
    OBJECT_IDX shuriken;
    OBJECT_IDX item{};
    POSITION y{};
    POSITION x{};
    POSITION ty{};
    POSITION tx{};
    POSITION prev_y{};
    POSITION prev_x{};
    POSITION ny[19]{};
    POSITION nx[19]{};
    int chance{};
    int tdam{};
    int tdis{};
    int cur_dis{};
    int visible{};
    PERCENTAGE corruption_possibility{};
    object_type *o_ptr{};
    bool hit_body = false;
    bool hit_wall = false;
    bool equiped_item = false;
    bool return_when_thrown = false;
    GAME_TEXT o_name[MAX_NLEN]{};
    BIT_FLAGS obj_flags[TR_FLAG_SIZE]{};
    bool come_back = false;
    bool do_drop = true;
    grid_type *g_ptr{};
    monster_type *m_ptr{};
    GAME_TEXT m_name[MAX_NLEN]{};
    int back_chance{};
    char o2_name[MAX_NLEN]{};
    bool super_boomerang{};

    bool check_can_throw();
    void calc_throw_range();
    bool calc_throw_grid();
    void reflect_inventory_by_throw();
    void set_class_specific_throw_params();
    void set_racial_chance();

private:
    player_type *creature_ptr;

    bool check_what_throw();
    bool check_throw_boomerang(concptr *q, concptr *s);
};
