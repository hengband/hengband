#include "floor/fixed-map-generator.h"
#include "artifact/fixed-art-generator.h"
#include "artifact/fixed-art-types.h"
#include "dungeon/quest.h"
#include "floor/floor-object.h"
#include "floor/wild.h"
#include "grid/object-placer.h"
#include "info-reader/fixed-map-parser.h"
#include "info-reader/general-parser.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/random-grid-effect-types.h"
#include "io/tokenizer.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-util.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "sv-definition/sv-scroll-types.h"
#include "system/angband-system.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "system/artifact/artifact-record.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-fixed-map.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "window/main-window-util.h"

qtwg_type *initialize_quest_generator_type(qtwg_type *qtwg_ptr, int ymin, int xmin, int ymax, int xmax, int *y, int *x)
{
    qtwg_ptr->ymin = ymin;
    qtwg_ptr->xmin = xmin;
    qtwg_ptr->ymax = ymax;
    qtwg_ptr->xmax = xmax;
    qtwg_ptr->y = y;
    qtwg_ptr->x = x;
    return qtwg_ptr;
}

/*!
 * @brief フロアの所定のマスにオブジェクトを配置する
 * Place the object j_ptr to a grid
 * @param floor 現在フロアへの参照
 * @param item アイテムの参照
 * @param y 配置先Y座標
 * @param x 配置先X座標
 * @return エラーコード
 */
static void drop_here(FloorType &floor, ItemEntity &&item, POSITION y, POSITION x)
{
    const auto item_idx = floor.pop_empty_index_item();
    auto &dropped_item = *floor.o_list[item_idx];
    dropped_item = std::move(item);
    dropped_item.iy = y;
    dropped_item.ix = x;
    dropped_item.held_m_idx = 0;
    auto &grid = floor.grid_array[y][x];
    grid.o_idx_list.add(floor, item_idx);
}

static void generate_artifact(PlayerType *player_ptr, qtwg_type *qtwg_ptr, const FixedArtifactId fa_id)
{
    if (fa_id == FixedArtifactId::NONE) {
        return;
    }

    if (!ArtifactRecords::get_instance().get_generated(fa_id) && create_named_art(player_ptr, fa_id, *qtwg_ptr->y, *qtwg_ptr->x)) {
        return;
    }

    ItemEntity item({ ItemKindType::SCROLL, SV_SCROLL_ACQUIREMENT });
    drop_here(*player_ptr->current_floor_ptr, std::move(item), *qtwg_ptr->y, *qtwg_ptr->x);
}

static void parse_qtw_D(PlayerType *player_ptr, qtwg_type *qtwg_ptr, char *s)
{
    *qtwg_ptr->x = qtwg_ptr->xmin;
    auto &floor = *player_ptr->current_floor_ptr;
    int len = strlen(s);
    auto &monraces = MonraceList::get_instance();
    const auto &dungeon = floor.get_dungeon_definition();
    for (auto i = 0; ((*qtwg_ptr->x < qtwg_ptr->xmax) && (i < len)); (*qtwg_ptr->x)++, s++, i++) {
        auto &grid = floor.grid_array[*qtwg_ptr->y][*qtwg_ptr->x];
        int idx = s[0];
        const auto item_index = letter[idx].object;
        auto monster_index = letter[idx].monster;
        const auto random = letter[idx].random;
        grid.feat = dungeon.convert_terrain_id(letter[idx].feature);
        if (init_flags & INIT_ONLY_FEATURES) {
            continue;
        }

        grid.info = letter[idx].cave_info;
        if (random & RANDOM_MONSTER) {
            floor.monster_level = floor.base_level + monster_index;
            place_random_monster(player_ptr, *qtwg_ptr->y, *qtwg_ptr->x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP | PM_NO_QUEST));
            floor.monster_level = floor.base_level;
        } else if (monster_index) {
            auto clone = false;
            if (monster_index < 0) {
                monster_index = -monster_index;
                clone = true;
            }

            const auto monrace_id = i2enum<MonraceId>(monster_index);
            auto &monrace = monraces.get_monrace(monrace_id);
            const auto old_cur_num = monrace.cur_num;
            const auto old_max_num = monrace.max_num;
            if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
                monrace.reset_current_numbers();
                monrace.max_num = MAX_UNIQUE_NUM;
            } else if (monrace.population_flags.has(MonsterPopulationType::NAZGUL)) {
                if (monrace.cur_num == monrace.max_num) {
                    monrace.max_num++;
                }
            }

            const auto m_idx = place_specific_monster(player_ptr, *qtwg_ptr->y, *qtwg_ptr->x, monrace_id, (PM_ALLOW_SLEEP | PM_NO_KAGE));
            if (clone && m_idx) {
                floor.m_list[*m_idx].mflag2.set(MonsterConstantFlagType::CLONED);
                monrace.cur_num = old_cur_num;
                monrace.max_num = old_max_num;
            }
        }

        if ((random & RANDOM_OBJECT) && (random & RANDOM_TRAP)) {
            floor.object_level = floor.base_level + item_index;

            /*
             * Random trap and random treasure defined
             * 25% chance for trap and 75% chance for object
             */
            const Pos2D pos(*qtwg_ptr->y, *qtwg_ptr->x);
            if (evaluate_percent(75)) {
                place_object(player_ptr, pos, 0);
            } else {
                floor.place_trap_at(pos);
            }

            floor.object_level = floor.base_level;
        } else if (random & RANDOM_OBJECT) {
            floor.object_level = floor.base_level + item_index;
            const Pos2D pos(*qtwg_ptr->y, *qtwg_ptr->x);
            if (evaluate_percent(75)) {
                place_object(player_ptr, pos, 0);
            } else if (evaluate_percent(80)) {
                place_object(player_ptr, pos, AM_GOOD);
            } else {
                place_object(player_ptr, pos, AM_GOOD | AM_GREAT);
            }

            floor.object_level = floor.base_level;
        } else if (random & RANDOM_TRAP) {
            const Pos2D pos(*qtwg_ptr->y, *qtwg_ptr->x);
            floor.place_trap_at(pos);
        } else if (letter[idx].trap) {
            grid.mimic = grid.feat;
            grid.feat = dungeon.convert_terrain_id(letter[idx].trap);
        } else if (item_index) {
            ItemEntity item(item_index);
            if (item.bi_key.tval() == ItemKindType::GOLD) {
                item = floor.make_gold(item.bi_key);
            }

            ItemMagicApplier(player_ptr, &item, floor.base_level, AM_NO_FIXED_ART | AM_GOOD).execute();
            drop_here(floor, std::move(item), *qtwg_ptr->y, *qtwg_ptr->x);
        }

        generate_artifact(player_ptr, qtwg_ptr, letter[idx].artifact);
        grid.special = letter[idx].special;
    }
}

static bool parse_qtw_P(PlayerType *player_ptr, qtwg_type *qtwg_ptr)
{
    if (qtwg_ptr->buf[0] != 'P') {
        return false;
    }

    if ((init_flags & INIT_CREATE_DUNGEON) == 0) {
        return true;
    }

    const auto tokens = tokenize(qtwg_ptr->buf + 2, 2);
    if (tokens.size() != 2) {
        return true;
    }

    int panels_y = (*qtwg_ptr->y / SCREEN_HGT);
    if (*qtwg_ptr->y % SCREEN_HGT) {
        panels_y++;
    }

    auto &floor = *player_ptr->current_floor_ptr;
    floor.height = panels_y * SCREEN_HGT;
    int panels_x = (*qtwg_ptr->x / SCREEN_WID);
    if (*qtwg_ptr->x % SCREEN_WID) {
        panels_x++;
    }

    floor.width = panels_x * SCREEN_WID;
    panel_row_min = floor.height;
    panel_col_min = floor.width;
    if (floor.is_in_quest()) {
        Pos2D p_pos(std::stoi(tokens[0]), std::stoi(tokens[1]));
        player_ptr->set_position(p_pos);
        delete_monster(player_ptr, player_ptr->get_position());
        return true;
    }

    if (!player_ptr->oldpx && !player_ptr->oldpy) {
        player_ptr->oldpy = std::stoi(tokens[0]);
        player_ptr->oldpx = std::stoi(tokens[1]);
    }

    return true;
}

/*!
 * @brief 固定マップ (クエスト＆街＆広域マップ)をフロアに生成する
 * Parse a sub-file of the "extra info"
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param buf 文字列
 * @param ymin 詳細不明
 * @param xmin 詳細不明
 * @param ymax 詳細不明
 * @param xmax 詳細不明
 * @param y 詳細不明
 * @param x 詳細不明
 * @return エラーコード
 * @todo クエスト情報のみを読み込む手段と実際にフロアデータまで読み込む処理は分離したい
 */
parse_error_type generate_fixed_map_floor(PlayerType *player_ptr, qtwg_type *qtwg_ptr, process_dungeon_file_pf parse_fixed_map)
{
    if (!qtwg_ptr->buf[0]) {
        return PARSE_ERROR_NONE;
    }

    if (iswspace(qtwg_ptr->buf[0])) {
        return PARSE_ERROR_NONE;
    }

    if (qtwg_ptr->buf[0] == '#') {
        return PARSE_ERROR_NONE;
    }

    if (qtwg_ptr->buf[1] != ':') {
        return PARSE_ERROR_GENERIC;
    }

    if (qtwg_ptr->buf[0] == '%') {
        return (*parse_fixed_map)(player_ptr, qtwg_ptr->buf + 2, qtwg_ptr->ymin, qtwg_ptr->xmin, qtwg_ptr->ymax, qtwg_ptr->xmax);
    }

    /* Process "F:<letter>:<terrain>:<cave_info>:<monster>:<object>:<ego>:<artifact>:<trap>:<special>" -- info for dungeon grid */
    if (qtwg_ptr->buf[0] == 'F') {
        return parse_line_feature(*player_ptr->current_floor_ptr, qtwg_ptr->buf);
    }

    if (qtwg_ptr->buf[0] == 'D') {
        char *s = qtwg_ptr->buf + 2;
        if (init_flags & INIT_ONLY_BUILDINGS) {
            return PARSE_ERROR_NONE;
        }

        parse_qtw_D(player_ptr, qtwg_ptr, s);
        (*qtwg_ptr->y)++;
        return PARSE_ERROR_NONE;
    }

    if (qtwg_ptr->buf[0] == 'W') {
        const Pos2D pos_initial(*qtwg_ptr->y, *qtwg_ptr->x);
        const auto pos = parse_line_wilderness(qtwg_ptr->buf, qtwg_ptr->xmin, qtwg_ptr->xmax, pos_initial);
        if (pos) {
            *qtwg_ptr->y = pos->y;
            *qtwg_ptr->x = pos->x;
        }

        return pos.error_or(PARSE_ERROR_NONE);
    }

    if (parse_qtw_P(player_ptr, qtwg_ptr)) {
        return PARSE_ERROR_NONE;
    }

    if (qtwg_ptr->buf[0] == 'B') {
        return parse_line_building(qtwg_ptr->buf);
    }

    // 荒野の広さを表すタグ。初期化時に読み込むのでそれ以降は無視する.
    if (qtwg_ptr->buf[0] == 'M') {
        return PARSE_ERROR_NONE;
    }

    return PARSE_ERROR_GENERIC;
}

/*!
 * @brief startVariants から現在の退出クエストに合う開始位置を選ぶ
 */
static const QuestStartPosition *select_quest_start(const QuestFixedMap &fixed_map)
{
    if (fixed_map.starts.empty()) {
        return nullptr;
    }

    const auto current_leaving = enum2i(leaving_quest);
    const QuestStartPosition *fallback = nullptr;
    for (const auto &start : fixed_map.starts) {
        if (!start.leaving_quest) {
            fallback = &start;
            continue;
        }

        if (*start.leaving_quest == current_leaving) {
            return &start;
        }
    }

    // 条件付きのみで一致が無い場合は配置しない (旧挙動: 該当 ?: が無ければ P: を実行しない)
    return fallback;
}

parse_error_type generate_quest_floor_from_json(PlayerType *player_ptr, QuestType &quest, const QuestFixedMap &fixed_map)
{
    // 1. 共通ベース凡例 (X/./%/D/< など) を letter[] へ適用 (旧 QuestPreferences.txt、起動時に読込済み)
    for (const auto &[symbol, cell] : QuestFixedMapList::get_instance().get_base_legend()) {
        letter[static_cast<uint8_t>(symbol)] = cell.grid;
    }

    // 2. このクエスト固有の凡例を letter[] に上書き (報酬は受託時 resolve_quest_reward で解決済み)
    for (const auto &[symbol, cell] : fixed_map.legend) {
        auto grid = cell.grid;
        if (cell.object_is_quest_reward && quest.has_reward() && !quest.is_reward_instant_artifact()) {
            grid.object = quest.get_reward_bi_id();
        }
        if (cell.artifact_is_quest_reward) {
            grid.artifact = quest.get_reward().value_or(FixedArtifactId::NONE);
        }

        letter[static_cast<uint8_t>(symbol)] = grid;
    }

    if (!fixed_map.has_map()) {
        return PARSE_ERROR_NONE;
    }

    // 3. マップバリアント選択: 旧 $RANDOMn は seed_town % n で決まる (乱数ではなくキャラ毎に固定)
    const auto variant_count = fixed_map.maps.size();
    const auto variant_index = (variant_count <= 1) ? 0 : (AngbandSystem::get_instance().get_seed_town() % variant_count);
    const auto &rows = fixed_map.maps[variant_index];

    // 4. 既存のセル配置ロジックでグリッドを敷く
    auto y = 0;
    auto x = 0;
    qtwg_type qg;
    initialize_quest_generator_type(&qg, 0, 0, MAX_HGT, MAX_WID, &y, &x);
    for (const auto &row : rows) {
        std::string buffer(row);
        parse_qtw_D(player_ptr, &qg, buffer.data());
        (*qg.y)++;
    }

    // 5. 開始位置とフロアサイズ (parse_qtw_P を "P:y:x" で再利用)
    if (const auto *start = select_quest_start(fixed_map); start != nullptr) {
        std::string p_line = "P:" + std::to_string(start->y) + ":" + std::to_string(start->x);
        qg.buf = p_line.data();
        parse_qtw_P(player_ptr, &qg);
    }

    return PARSE_ERROR_NONE;
}
