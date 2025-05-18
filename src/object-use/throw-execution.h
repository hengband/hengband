#pragma once
/*!
 * @file throw-util.h
 * @brief 投擲処理関連ヘッダ
 */

#include "object-enchant/tr-flags.h"
#include "system/angband.h"
#include "system/system-variables.h"
#include "util/flag-group.h"
#include <string>

class Grid;
class MonsterEntity;
class ItemEntity;
class PlayerType;

class ObjectThrowHitMonster {
public:
    ObjectThrowHitMonster(PlayerType *player_ptr, POSITION y, POSITION x);

    MONSTER_IDX m_idx{};
    MonsterEntity *m_ptr{};
    std::string m_name{};
};

class ObjectThrowEntity {
public:
    ObjectThrowEntity() = default;
    ObjectThrowEntity(
        PlayerType *player_ptr, ItemEntity *q_ptr, const int delay_factor_val, const int mult, const bool boomerang, const OBJECT_IDX shuriken);
    virtual ~ObjectThrowEntity() = default;

    ItemEntity *q_ptr;
    OBJECT_IDX i_idx{};
    POSITION y{};
    POSITION x{};
    POSITION prev_y{};
    POSITION prev_x{};
    bool equiped_item = false;
    PERCENTAGE corruption_possibility{};

    bool check_can_throw();
    void calc_throw_range();
    bool calc_throw_grid();
    void reflect_inventory_by_throw();
    void set_class_specific_throw_params();
    void set_racial_chance();
    void exe_throw();
    void display_figurine_throw();
    void display_potion_throw();
    void check_boomerang_throw();
    void process_boomerang_back();
    void drop_thrown_item();
    bool has_hit_monster() const;

private:
    PlayerType *player_ptr;
    OBJECT_IDX shuriken;
    int mult;
    int msec;
    bool boomerang;
    POSITION ty{};
    POSITION tx{};
    POSITION ny[19]{};
    POSITION nx[19]{};
    int chance{};
    int tdam{};
    int tdis{};
    int cur_dis{};
    ItemEntity *o_ptr{};
    bool hit_wall = false;
    bool return_when_thrown = false;
    std::string o_name{};
    TrFlags obj_flags{};
    bool come_back = false;
    bool do_drop = true;
    int back_chance{};
    std::string o2_name{};
    bool super_boomerang{};
    std::optional<ObjectThrowHitMonster> hit_monster{};

    bool check_what_throw();
    bool check_throw_boomerang();
    bool check_racial_target_bold();
    void check_racial_target_seen();
    bool check_racial_target_monster();
    void attack_racial_power();
    void display_attack_racial_power();
    void calc_racial_power_damage();
    void process_boomerang_throw();
    void display_boomerang_throw();
};
