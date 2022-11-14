#include "util/sort.h"
#include "artifact/fixed-art-types.h"
#include "dungeon/quest.h"
#include "grid/grid.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-flag-types.h"
#include "system//monster-race-definition.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"

/*
 * @brief クイックソートの実行 / Quick sort in place
 * @param u アイテムやモンスター等への配列
 * @param v 条件基準IDへの参照ポインタ
 * @param a 比較対象のID1
 * @param b 比較対象のID2
 * @param ang_sort_comp 比較用の関数ポインタ
 * @param ang_sort_swap スワップ用の関数ポインタ
 */
static void exe_ang_sort(PlayerType *player_ptr, vptr u, vptr v, int p, int q, bool (*ang_sort_comp)(PlayerType *, vptr, vptr, int, int),
    void (*ang_sort_swap)(PlayerType *, vptr, vptr, int, int))
{
    if (p >= q) {
        return;
    }

    int z = p;
    int a = p;
    int b = q;
    while (true) {
        /* Slide i2 */
        while (!(*ang_sort_comp)(player_ptr, u, v, b, z)) {
            b--;
        }

        /* Slide i1 */
        while (!(*ang_sort_comp)(player_ptr, u, v, z, a)) {
            a++;
        }

        if (a >= b) {
            break;
        }

        (*ang_sort_swap)(player_ptr, u, v, a, b);

        a++, b--;
    }

    /* Recurse left side */
    exe_ang_sort(player_ptr, u, v, p, b, ang_sort_comp, ang_sort_swap);

    /* Recurse right side */
    exe_ang_sort(player_ptr, u, v, b + 1, q, ang_sort_comp, ang_sort_swap);
}

/*
 * @brief クイックソートの受け付け / Accepting auick sort in place
 * @param u アイテムやモンスター等への配列
 * @param v 条件基準IDへの参照ポインタ
 * @param a 比較対象のID1
 * @param b 比較対象のID2
 * @param ang_sort_comp 比較用の関数ポインタ
 * @param ang_sort_swap スワップ用の関数ポインタ
 */
void ang_sort(PlayerType *player_ptr, vptr u, vptr v, int n, bool (*ang_sort_comp)(PlayerType *, vptr, vptr, int, int),
    void (*ang_sort_swap)(PlayerType *, vptr, vptr, int, int))
{
    exe_ang_sort(player_ptr, u, v, 0, n - 1, ang_sort_comp, ang_sort_swap);
}

/*
 * Sorting hook -- comp function -- by "distance to player"
 *
 * We use "u" and "v" to point to arrays of "x" and "y" positions,
 * and sort the arrays by double-distance to the player.
 */
bool ang_sort_comp_distance(PlayerType *player_ptr, vptr u, vptr v, int a, int b)
{
    POSITION *x = (POSITION *)(u);
    POSITION *y = (POSITION *)(v);

    /* Absolute distance components */
    POSITION kx = x[a];
    kx -= player_ptr->x;
    kx = std::abs(kx);
    POSITION ky = y[a];
    ky -= player_ptr->y;
    ky = std::abs(ky);

    /* Approximate Double Distance to the first point */
    POSITION da = ((kx > ky) ? (kx + kx + ky) : (ky + ky + kx));

    /* Absolute distance components */
    kx = x[b];
    kx -= player_ptr->x;
    kx = std::abs(kx);
    ky = y[b];
    ky -= player_ptr->y;
    ky = std::abs(ky);

    /* Approximate Double Distance to the first point */
    POSITION db = ((kx > ky) ? (kx + kx + ky) : (ky + ky + kx));

    /* Compare the distances */
    return da <= db;
}

/*
 * Sorting hook -- comp function -- by importance level of grids
 *
 * We use "u" and "v" to point to arrays of "x" and "y" positions,
 * and sort the arrays by level of monster
 */
bool ang_sort_comp_importance(PlayerType *player_ptr, vptr u, vptr v, int a, int b)
{
    POSITION *x = (POSITION *)(u);
    POSITION *y = (POSITION *)(v);
    grid_type *ca_ptr = &player_ptr->current_floor_ptr->grid_array[y[a]][x[a]];
    grid_type *cb_ptr = &player_ptr->current_floor_ptr->grid_array[y[b]][x[b]];
    monster_type *ma_ptr = &player_ptr->current_floor_ptr->m_list[ca_ptr->m_idx];
    monster_type *mb_ptr = &player_ptr->current_floor_ptr->m_list[cb_ptr->m_idx];
    monster_race *ap_ra_ptr, *ap_rb_ptr;

    /* The player grid */
    if (y[a] == player_ptr->y && x[a] == player_ptr->x) {
        return true;
    }

    if (y[b] == player_ptr->y && x[b] == player_ptr->x) {
        return false;
    }

    /* Extract monster race */
    if (ca_ptr->m_idx && ma_ptr->ml) {
        ap_ra_ptr = &monraces_info[ma_ptr->ap_r_idx];
    } else {
        ap_ra_ptr = nullptr;
    }

    if (cb_ptr->m_idx && mb_ptr->ml) {
        ap_rb_ptr = &monraces_info[mb_ptr->ap_r_idx];
    } else {
        ap_rb_ptr = nullptr;
    }

    if (ap_ra_ptr && !ap_rb_ptr) {
        return true;
    }

    if (!ap_ra_ptr && ap_rb_ptr) {
        return false;
    }

    /* Compare two monsters */
    if (ap_ra_ptr && ap_rb_ptr) {
        /* Unique monsters first */
        if (ap_ra_ptr->kind_flags.has(MonsterKindType::UNIQUE) && ap_rb_ptr->kind_flags.has_not(MonsterKindType::UNIQUE)) {
            return true;
        }
        if (ap_ra_ptr->kind_flags.has_not(MonsterKindType::UNIQUE) && ap_rb_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
            return false;
        }

        /* Shadowers first (あやしい影) */
        if (ma_ptr->mflag2.has(MonsterConstantFlagType::KAGE) && mb_ptr->mflag2.has_not(MonsterConstantFlagType::KAGE)) {
            return true;
        }
        if (ma_ptr->mflag2.has_not(MonsterConstantFlagType::KAGE) && mb_ptr->mflag2.has(MonsterConstantFlagType::KAGE)) {
            return false;
        }

        /* Unknown monsters first */
        if (!ap_ra_ptr->r_tkills && ap_rb_ptr->r_tkills) {
            return true;
        }
        if (ap_ra_ptr->r_tkills && !ap_rb_ptr->r_tkills) {
            return false;
        }

        /* Higher level monsters first (if known) */
        if (ap_ra_ptr->r_tkills && ap_rb_ptr->r_tkills) {
            if (ap_ra_ptr->level > ap_rb_ptr->level) {
                return true;
            }
            if (ap_ra_ptr->level < ap_rb_ptr->level) {
                return false;
            }
        }

        /* Sort by index if all conditions are same */
        if (ma_ptr->ap_r_idx > mb_ptr->ap_r_idx) {
            return true;
        }
        if (ma_ptr->ap_r_idx < mb_ptr->ap_r_idx) {
            return false;
        }
    }

    /* An object get higher priority */
    if (!player_ptr->current_floor_ptr->grid_array[y[a]][x[a]].o_idx_list.empty() && player_ptr->current_floor_ptr->grid_array[y[b]][x[b]].o_idx_list.empty()) {
        return true;
    }

    if (player_ptr->current_floor_ptr->grid_array[y[a]][x[a]].o_idx_list.empty() && !player_ptr->current_floor_ptr->grid_array[y[b]][x[b]].o_idx_list.empty()) {
        return false;
    }

    /* Priority from the terrain */
    if (terrains_info[ca_ptr->feat].priority > terrains_info[cb_ptr->feat].priority) {
        return true;
    }

    if (terrains_info[ca_ptr->feat].priority < terrains_info[cb_ptr->feat].priority) {
        return false;
    }

    /* If all conditions are same, compare distance */
    return ang_sort_comp_distance(player_ptr, u, v, a, b);
}

/*
 * Sorting hook -- swap function -- by "distance to player"
 *
 * We use "u" and "v" to point to arrays of "x" and "y" positions,
 * and sort the arrays by distance to the player.
 */
void ang_sort_swap_position(PlayerType *player_ptr, vptr u, vptr v, int a, int b)
{
    /* Unused */
    (void)player_ptr;

    POSITION *x = (POSITION *)(u);
    POSITION *y = (POSITION *)(v);

    POSITION temp = x[a];
    x[a] = x[b];
    x[b] = temp;

    temp = y[a];
    y[a] = y[b];
    y[b] = temp;
}

/*
 * Sorting hook -- Comp function -- see below
 *
 * We use "u" to point to array of monster indexes,
 * and "v" to select the type of sorting to perform on "u".
 */
bool ang_sort_art_comp(PlayerType *player_ptr, vptr u, vptr v, int a, int b)
{
    /* Unused */
    (void)player_ptr;

    uint16_t *who = (uint16_t *)(u);
    uint16_t *why = (uint16_t *)(v);

    const auto w1 = i2enum<FixedArtifactId>(who[a]);
    const auto w2 = i2enum<FixedArtifactId>(who[b]);

    int z1;
    int z2;

    /* Sort by total kills */
    if (*why >= 3) {
        /* Extract total kills */
        z1 = enum2i(artifacts_info.at(w1).bi_key.tval());
        z2 = enum2i(artifacts_info.at(w2).bi_key.tval());

        /* Compare total kills */
        if (z1 < z2) {
            return true;
        }

        if (z1 > z2) {
            return false;
        }
    }

    /* Sort by monster level */
    if (*why >= 2) {
        /* Extract levels */
        z1 = artifacts_info.at(w1).bi_key.sval().value();
        z2 = artifacts_info.at(w2).bi_key.sval().value();

        /* Compare levels */
        if (z1 < z2) {
            return true;
        }

        if (z1 > z2) {
            return false;
        }
    }

    /* Sort by monster experience */
    if (*why >= 1) {
        /* Extract experience */
        z1 = artifacts_info.at(w1).level;
        z2 = artifacts_info.at(w2).level;

        /* Compare experience */
        if (z1 < z2) {
            return true;
        }

        if (z1 > z2) {
            return false;
        }
    }

    /* Compare indexes */
    return w1 <= w2;
}

/*
 * Sorting hook -- Swap function -- see below
 *
 * We use "u" to point to array of monster indexes,
 * and "v" to select the type of sorting to perform.
 */
void ang_sort_art_swap(PlayerType *player_ptr, vptr u, vptr v, int a, int b)
{
    /* Unused */
    (void)player_ptr;
    (void)v;

    uint16_t *who = (uint16_t *)(u);
    uint16_t holder = who[a];
    who[a] = who[b];
    who[b] = holder;
}

bool ang_sort_comp_quest_num(PlayerType *player_ptr, vptr u, vptr v, int a, int b)
{
    /* Unused */
    (void)player_ptr;
    (void)v;

    const auto &quest_list = QuestList::get_instance();
    QuestId *q_num = (QuestId *)u;
    const auto *qa = &quest_list[q_num[a]];
    const auto *qb = &quest_list[q_num[b]];
    return (qa->comptime != qb->comptime) ? (qa->comptime < qb->comptime) : (qa->level <= qb->level);
}

void ang_sort_swap_quest_num(PlayerType *player_ptr, vptr u, vptr v, int a, int b)
{
    /* Unused */
    (void)player_ptr;
    (void)v;

    QuestId *q_num = (QuestId *)u;
    QuestId tmp = q_num[a];
    q_num[a] = q_num[b];
    q_num[b] = tmp;
}

/*!
 * @brief ペット入りモンスターボールをソートするための比較関数
 * @param u 所持品配列の参照ポインタ
 * @param v 未使用
 * @param a 所持品ID1
 * @param b 所持品ID2
 * @return 1の方が大であればTRUE
 */
bool ang_sort_comp_pet(PlayerType *player_ptr, vptr u, vptr v, int a, int b)
{
    /* Unused */
    (void)v;

    uint16_t *who = (uint16_t *)(u);

    int w1 = who[a];
    int w2 = who[b];

    monster_type *m_ptr1 = &player_ptr->current_floor_ptr->m_list[w1];
    monster_type *m_ptr2 = &player_ptr->current_floor_ptr->m_list[w2];
    monster_race *r_ptr1 = &monraces_info[m_ptr1->r_idx];
    monster_race *r_ptr2 = &monraces_info[m_ptr2->r_idx];

    if (m_ptr1->nickname && !m_ptr2->nickname) {
        return true;
    }

    if (m_ptr2->nickname && !m_ptr1->nickname) {
        return false;
    }

    if (r_ptr1->kind_flags.has(MonsterKindType::UNIQUE) && r_ptr2->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return true;
    }

    if (r_ptr2->kind_flags.has(MonsterKindType::UNIQUE) && r_ptr1->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return false;
    }

    if (r_ptr1->level > r_ptr2->level) {
        return true;
    }

    if (r_ptr2->level > r_ptr1->level) {
        return false;
    }

    if (m_ptr1->hp > m_ptr2->hp) {
        return true;
    }

    if (m_ptr2->hp > m_ptr1->hp) {
        return false;
    }

    return w1 <= w2;
}

/*!
 * @brief モンスター種族情報を特定の基準によりソートするための比較処理
 * Sorting hook -- Comp function -- see below
 * @param u モンスター種族情報の入れるポインタ
 * @param v 条件基準ID
 * @param a 比較するモンスター種族のID1
 * @param b 比較するモンスター種族のID2
 * @return 2の方が大きければTRUEを返す
 * We use "u" to point to array of monster indexes,
 * and "v" to select the type of sorting to perform on "u".
 */
bool ang_sort_comp_hook(PlayerType *player_ptr, vptr u, vptr v, int a, int b)
{
    /* Unused */
    (void)player_ptr;

    MonsterRaceId *who = (MonsterRaceId *)(u);
    uint16_t *why = (uint16_t *)(v);

    auto w1 = who[a];
    auto w2 = who[b];

    int z1, z2;

    /* Sort by player kills */
    if (*why >= 4) {
        /* Extract player kills */
        z1 = monraces_info[w1].r_pkills;
        z2 = monraces_info[w2].r_pkills;

        /* Compare player kills */
        if (z1 < z2) {
            return true;
        }
        if (z1 > z2) {
            return false;
        }
    }

    /* Sort by total kills */
    if (*why >= 3) {
        /* Extract total kills */
        z1 = monraces_info[w1].r_tkills;
        z2 = monraces_info[w2].r_tkills;

        /* Compare total kills */
        if (z1 < z2) {
            return true;
        }
        if (z1 > z2) {
            return false;
        }
    }

    /* Sort by monster level */
    if (*why >= 2) {
        /* Extract levels */
        z1 = monraces_info[w1].level;
        z2 = monraces_info[w2].level;

        /* Compare levels */
        if (z1 < z2) {
            return true;
        }
        if (z1 > z2) {
            return false;
        }
    }

    /* Sort by monster experience */
    if (*why >= 1) {
        /* Extract experience */
        z1 = monraces_info[w1].mexp;
        z2 = monraces_info[w2].mexp;

        /* Compare experience */
        if (z1 < z2) {
            return true;
        }
        if (z1 > z2) {
            return false;
        }
    }

    /* Compare indexes */
    return w1 <= w2;
}

/*!
 * @brief モンスター種族情報を特定の基準によりソートするためのスワップ処理
 * Sorting hook -- Swap function -- see below
 * @param u モンスター種族情報の入れるポインタ
 * @param v 未使用
 * @param a スワップするモンスター種族のID1
 * @param b スワップするモンスター種族のID2
 * @details
 * We use "u" to point to array of monster indexes,
 * and "v" to select the type of sorting to perform.
 */
void ang_sort_swap_hook(PlayerType *player_ptr, vptr u, vptr v, int a, int b)
{
    /* Unused */
    (void)player_ptr;
    (void)v;

    uint16_t *who = (uint16_t *)(u);
    uint16_t holder;

    holder = who[a];
    who[a] = who[b];
    who[b] = holder;
}

/*
 * hook function to sort monsters by level
 */
bool ang_sort_comp_monster_level(PlayerType *player_ptr, vptr u, vptr v, int a, int b)
{
    /* Unused */
    (void)player_ptr;
    (void)v;

    MonsterRaceId *who = (MonsterRaceId *)(u);

    auto w1 = who[a];
    auto w2 = who[b];

    monster_race *r_ptr1 = &monraces_info[w1];
    monster_race *r_ptr2 = &monraces_info[w2];

    if (r_ptr2->level > r_ptr1->level) {
        return true;
    }

    if (r_ptr1->level > r_ptr2->level) {
        return false;
    }

    if (r_ptr2->kind_flags.has(MonsterKindType::UNIQUE) && r_ptr1->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return true;
    }

    if (r_ptr1->kind_flags.has(MonsterKindType::UNIQUE) && r_ptr2->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return false;
    }

    return w1 <= w2;
}

/*!
 * @brief ペットになっているモンスターをソートするための比較処理
 * @param u モンスターの構造体配列
 * @param v 未使用
 * @param a 比較対象のモンスターID1
 * @param b 比較対象のモンスターID2
 * @return 2番目が大ならばTRUEを返す
 */
bool ang_sort_comp_pet_dismiss(PlayerType *player_ptr, vptr u, vptr v, int a, int b)
{
    /* Unused */
    (void)v;

    uint16_t *who = (uint16_t *)(u);

    int w1 = who[a];
    int w2 = who[b];

    monster_type *m_ptr1 = &player_ptr->current_floor_ptr->m_list[w1];
    monster_type *m_ptr2 = &player_ptr->current_floor_ptr->m_list[w2];
    monster_race *r_ptr1 = &monraces_info[m_ptr1->r_idx];
    monster_race *r_ptr2 = &monraces_info[m_ptr2->r_idx];

    if (w1 == player_ptr->riding) {
        return true;
    }

    if (w2 == player_ptr->riding) {
        return false;
    }

    if (m_ptr1->nickname && !m_ptr2->nickname) {
        return true;
    }

    if (m_ptr2->nickname && !m_ptr1->nickname) {
        return false;
    }

    if (!m_ptr1->parent_m_idx && m_ptr2->parent_m_idx) {
        return true;
    }

    if (!m_ptr2->parent_m_idx && m_ptr1->parent_m_idx) {
        return false;
    }

    if (r_ptr1->kind_flags.has(MonsterKindType::UNIQUE) && r_ptr2->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return true;
    }

    if (r_ptr2->kind_flags.has(MonsterKindType::UNIQUE) && r_ptr1->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return false;
    }

    if (r_ptr1->level > r_ptr2->level) {
        return true;
    }

    if (r_ptr2->level > r_ptr1->level) {
        return false;
    }

    if (m_ptr1->hp > m_ptr2->hp) {
        return true;
    }

    if (m_ptr2->hp > m_ptr1->hp) {
        return false;
    }

    return w1 <= w2;
}

/*!
 * @brief フロア保存時のgrid情報テンプレートをソートするための比較処理
 * @param u gridテンプレートの参照ポインタ
 * @param v 未使用
 * @param a スワップするモンスター種族のID1
 * @param b スワップするモンスター種族のID2
 * @return aの方が大きければtrue
 */
bool ang_sort_comp_cave_temp(PlayerType *player_ptr, vptr u, vptr v, int a, int b)
{
    /* Unused */
    (void)player_ptr;
    (void)v;

    grid_template_type *who = (grid_template_type *)(u);

    uint16_t o1 = who[a].occurrence;
    uint16_t o2 = who[b].occurrence;

    return o2 <= o1;
}

/*!
 * @brief フロア保存時のgrid情報テンプレートをソートするためのスワップ処理 / Sorting hook -- Swap function
 * @param u gridテンプレートの参照ポインタ
 * @param v 未使用
 * @param a スワップするモンスター種族のID1
 * @param b スワップするモンスター種族のID2
 */
void ang_sort_swap_cave_temp(PlayerType *player_ptr, vptr u, vptr v, int a, int b)
{
    /* Unused */
    (void)player_ptr;
    (void)v;

    grid_template_type *who = (grid_template_type *)(u);
    grid_template_type holder;

    holder = who[a];
    who[a] = who[b];
    who[b] = holder;
}
