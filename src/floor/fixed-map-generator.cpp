#include "floor/fixed-map-generator.h"
#include "artifact/fixed-art-generator.h"
#include "artifact/fixed-art-types.h"
#include "dungeon/quest.h"
#include "floor/floor-object.h"
#include "floor/floor-town.h"
#include "floor/wild.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "grid/object-placer.h"
#include "grid/trap.h"
#include "info-reader/general-parser.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/random-grid-effect-types.h"
#include "io/tokenizer.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-util.h"
#include "monster/smart-learn-types.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/trg-types.h"
#include "object/object-info.h"
#include "object/object-kind-hook.h"
#include "room/rooms-vault.h"
#include "sv-definition/sv-scroll-types.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "window/main-window-util.h"
#include "world/world-object.h"
#include "world/world.h"

// PARSE_ERROR_MAXが既にあり扱い辛いのでここでconst宣言.
static const int PARSE_CONTINUE = 255;

qtwg_type *initialize_quest_generator_type(qtwg_type *qtwg_ptr, char *buf, int ymin, int xmin, int ymax, int xmax, int *y, int *x)
{
    qtwg_ptr->buf = buf;
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
 * @param floor_ptr 現在フロアへの参照ポインタ
 * @param j_ptr オブジェクト構造体の参照ポインタ
 * @param y 配置先Y座標
 * @param x 配置先X座標
 * @return エラーコード
 */
static void drop_here(FloorType *floor_ptr, ItemEntity *j_ptr, POSITION y, POSITION x)
{
    OBJECT_IDX o_idx = o_pop(floor_ptr);
    ItemEntity *o_ptr;
    o_ptr = &floor_ptr->o_list[o_idx];
    o_ptr->copy_from(j_ptr);
    o_ptr->iy = y;
    o_ptr->ix = x;
    o_ptr->held_m_idx = 0;
    auto *g_ptr = &floor_ptr->grid_array[y][x];
    g_ptr->o_idx_list.add(floor_ptr, o_idx);
}

static void generate_artifact(PlayerType *player_ptr, qtwg_type *qtwg_ptr, const FixedArtifactId a_idx)
{
    if (a_idx == FixedArtifactId::NONE) {
        return;
    }

    const auto &artifact = ArtifactsInfo::get_instance().get_artifact(a_idx);
    if (!artifact.is_generated && create_named_art(player_ptr, a_idx, *qtwg_ptr->y, *qtwg_ptr->x)) {
        return;
    }

    const auto bi_id = lookup_baseitem_id({ ItemKindType::SCROLL, SV_SCROLL_ACQUIREMENT });
    ItemEntity forge;
    auto *q_ptr = &forge;
    q_ptr->prep(bi_id);
    drop_here(player_ptr->current_floor_ptr, q_ptr, *qtwg_ptr->y, *qtwg_ptr->x);
}

static void parse_qtw_D(PlayerType *player_ptr, qtwg_type *qtwg_ptr, char *s)
{
    *qtwg_ptr->x = qtwg_ptr->xmin;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    int len = strlen(s);
    for (int i = 0; ((*qtwg_ptr->x < qtwg_ptr->xmax) && (i < len)); (*qtwg_ptr->x)++, s++, i++) {
        auto *g_ptr = &floor_ptr->grid_array[*qtwg_ptr->y][*qtwg_ptr->x];
        int idx = s[0];
        OBJECT_IDX object_index = letter[idx].object;
        MONSTER_IDX monster_index = letter[idx].monster;
        int random = letter[idx].random;
        g_ptr->feat = conv_dungeon_feat(floor_ptr, letter[idx].feature);
        if (init_flags & INIT_ONLY_FEATURES) {
            continue;
        }

        g_ptr->info = letter[idx].cave_info;
        if (random & RANDOM_MONSTER) {
            floor_ptr->monster_level = floor_ptr->base_level + monster_index;

            place_random_monster(player_ptr, *qtwg_ptr->y, *qtwg_ptr->x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP | PM_NO_QUEST));

            floor_ptr->monster_level = floor_ptr->base_level;
        } else if (monster_index) {
            int old_cur_num, old_max_num;
            bool clone = false;

            if (monster_index < 0) {
                monster_index = -monster_index;
                clone = true;
            }

            const auto r_idx = i2enum<MonsterRaceId>(monster_index);
            auto &r_ref = monraces_info[r_idx];

            old_cur_num = r_ref.cur_num;
            old_max_num = r_ref.max_num;

            if (r_ref.kind_flags.has(MonsterKindType::UNIQUE)) {
                r_ref.cur_num = 0;
                r_ref.max_num = 1;
            } else if (r_ref.population_flags.has(MonsterPopulationType::NAZGUL)) {
                if (r_ref.cur_num == r_ref.max_num) {
                    r_ref.max_num++;
                }
            }

            place_specific_monster(player_ptr, 0, *qtwg_ptr->y, *qtwg_ptr->x, r_idx, (PM_ALLOW_SLEEP | PM_NO_KAGE));
            if (clone) {
                floor_ptr->m_list[hack_m_idx_ii].mflag2.set(MonsterConstantFlagType::CLONED);
                r_ref.cur_num = old_cur_num;
                r_ref.max_num = old_max_num;
            }
        }

        if ((random & RANDOM_OBJECT) && (random & RANDOM_TRAP)) {
            floor_ptr->object_level = floor_ptr->base_level + object_index;

            /*
             * Random trap and random treasure defined
             * 25% chance for trap and 75% chance for object
             */
            if (randint0(100) < 75) {
                place_object(player_ptr, *qtwg_ptr->y, *qtwg_ptr->x, 0L);
            } else {
                place_trap(floor_ptr, *qtwg_ptr->y, *qtwg_ptr->x);
            }

            floor_ptr->object_level = floor_ptr->base_level;
        } else if (random & RANDOM_OBJECT) {
            floor_ptr->object_level = floor_ptr->base_level + object_index;
            if (randint0(100) < 75) {
                place_object(player_ptr, *qtwg_ptr->y, *qtwg_ptr->x, 0L);
            } else if (randint0(100) < 80) {
                place_object(player_ptr, *qtwg_ptr->y, *qtwg_ptr->x, AM_GOOD);
            } else {
                place_object(player_ptr, *qtwg_ptr->y, *qtwg_ptr->x, AM_GOOD | AM_GREAT);
            }

            floor_ptr->object_level = floor_ptr->base_level;
        } else if (random & RANDOM_TRAP) {
            place_trap(floor_ptr, *qtwg_ptr->y, *qtwg_ptr->x);
        } else if (letter[idx].trap) {
            g_ptr->mimic = g_ptr->feat;
            g_ptr->feat = conv_dungeon_feat(floor_ptr, letter[idx].trap);
        } else if (object_index) {
            ItemEntity tmp_object;
            auto *o_ptr = &tmp_object;
            o_ptr->prep(object_index);
            if (o_ptr->bi_key.tval() == ItemKindType::GOLD) {
                coin_type = object_index - OBJ_GOLD_LIST;
                make_gold(player_ptr, o_ptr);
                coin_type = 0;
            }

            ItemMagicApplier(player_ptr, o_ptr, floor_ptr->base_level, AM_NO_FIXED_ART | AM_GOOD).execute();
            drop_here(floor_ptr, o_ptr, *qtwg_ptr->y, *qtwg_ptr->x);
        }

        generate_artifact(player_ptr, qtwg_ptr, letter[idx].artifact);
        g_ptr->special = letter[idx].special;
    }
}

static bool parse_qtw_QQ(QuestType *q_ptr, char **zz, int num)
{
    if (zz[1][0] != 'Q') {
        return false;
    }

    if ((init_flags & INIT_ASSIGN) == 0) {
        return true;
    }

    if (num < 9) {
        return true;
    }

    q_ptr->type = i2enum<QuestKindType>(atoi(zz[2]));
    q_ptr->num_mon = (MONSTER_NUMBER)atoi(zz[3]);
    q_ptr->cur_num = (MONSTER_NUMBER)atoi(zz[4]);
    q_ptr->max_num = (MONSTER_NUMBER)atoi(zz[5]);
    q_ptr->level = (DEPTH)atoi(zz[6]);
    q_ptr->r_idx = i2enum<MonsterRaceId>(atoi(zz[7]));
    const auto a_idx = i2enum<FixedArtifactId>(atoi(zz[8]));
    q_ptr->reward_artifact_idx = a_idx;
    q_ptr->dungeon = (DUNGEON_IDX)atoi(zz[9]);

    if (num > 10) {
        q_ptr->flags = atoi(zz[10]);
    }

    auto &r_ref = monraces_info[q_ptr->r_idx];
    if (r_ref.kind_flags.has(MonsterKindType::UNIQUE)) {
        r_ref.flags1 |= RF1_QUESTOR;
    }

    if (a_idx == FixedArtifactId::NONE) {
        return true;
    }

    auto &artifact = q_ptr->get_reward();
    artifact.gen_flags.set(ItemGenerationTraitType::QUESTITEM);
    return true;
}

/*!
 * @todo 処理がどうなっているのかいずれチェックする
 */
static bool parse_qtw_QR(QuestType *q_ptr, char **zz, int num)
{
    if (zz[1][0] != 'R') {
        return false;
    }

    if ((init_flags & INIT_ASSIGN) == 0) {
        return true;
    }

    int count = 0;
    FixedArtifactId reward_idx = FixedArtifactId::NONE;
    const auto &artifacts = ArtifactsInfo::get_instance();
    for (auto idx = 2; idx < num; idx++) {
        const auto a_idx = i2enum<FixedArtifactId>(atoi(zz[idx]));
        if (a_idx == FixedArtifactId::NONE) {
            continue;
        }

        if (artifacts.get_artifact(a_idx).is_generated) {
            continue;
        }

        count++;
        if (one_in_(count)) {
            reward_idx = a_idx;
        }
    }

    if (reward_idx != FixedArtifactId::NONE) {
        q_ptr->reward_artifact_idx = reward_idx;
        artifacts.get_artifact(reward_idx).gen_flags.set(ItemGenerationTraitType::QUESTITEM);
    } else {
        q_ptr->type = QuestKindType::KILL_ALL;
    }

    return true;
}

/*!
 * @brief t_info、q_info、w_infoにおけるQトークンをパースする
 * @param qtwg_ptr トークンパース構造体への参照ポインタ
 * @param zz トークン保管文字列
 * @return エラーコード、但しPARSE_CONTINUEの時は処理続行
 */
static int parse_qtw_Q(qtwg_type *qtwg_ptr, char **zz)
{
    if (qtwg_ptr->buf[0] != 'Q') {
        return PARSE_CONTINUE;
    }

#ifdef JP
    if (qtwg_ptr->buf[2] == '$') {
        return PARSE_ERROR_NONE;
    }
#else
    if (qtwg_ptr->buf[2] != '$') {
        return PARSE_ERROR_NONE;
    }
#endif

    int num = tokenize(qtwg_ptr->buf + _(2, 3), 33, zz, 0);
    if (num < 3) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    auto &quest_list = QuestList::get_instance();
    auto *q_ptr = &(quest_list[i2enum<QuestId>(atoi(zz[0]))]);
    if (parse_qtw_QQ(q_ptr, zz, num)) {
        return PARSE_ERROR_NONE;
    }

    if (parse_qtw_QR(q_ptr, zz, num)) {
        return PARSE_ERROR_NONE;
    }

    if (zz[1][0] == 'N') {
        if (init_flags & (INIT_ASSIGN | INIT_SHOW_TEXT | INIT_NAME_ONLY)) {
            q_ptr->name = zz[2];
        }

        return PARSE_ERROR_NONE;
    }

    if (zz[1][0] == 'T') {
        if (init_flags & INIT_SHOW_TEXT) {
            strcpy(quest_text[quest_text_line], zz[2]);
            quest_text_line++;
        }

        return PARSE_ERROR_NONE;
    }

    return PARSE_ERROR_GENERIC;
}

static bool parse_qtw_P(PlayerType *player_ptr, qtwg_type *qtwg_ptr, char **zz)
{
    if (qtwg_ptr->buf[0] != 'P') {
        return false;
    }

    if ((init_flags & INIT_CREATE_DUNGEON) == 0) {
        return true;
    }

    if (tokenize(qtwg_ptr->buf + 2, 2, zz, 0) != 2) {
        return true;
    }

    int panels_y = (*qtwg_ptr->y / SCREEN_HGT);
    if (*qtwg_ptr->y % SCREEN_HGT) {
        panels_y++;
    }

    auto *floor_ptr = player_ptr->current_floor_ptr;
    floor_ptr->height = panels_y * SCREEN_HGT;
    int panels_x = (*qtwg_ptr->x / SCREEN_WID);
    if (*qtwg_ptr->x % SCREEN_WID) {
        panels_x++;
    }

    floor_ptr->width = panels_x * SCREEN_WID;
    panel_row_min = floor_ptr->height;
    panel_col_min = floor_ptr->width;
    if (floor_ptr->is_in_quest()) {
        POSITION py = atoi(zz[0]);
        POSITION px = atoi(zz[1]);
        player_ptr->y = py;
        player_ptr->x = px;
        delete_monster(player_ptr, player_ptr->y, player_ptr->x);
        return true;
    }

    if (!player_ptr->oldpx && !player_ptr->oldpy) {
        player_ptr->oldpy = atoi(zz[0]);
        player_ptr->oldpx = atoi(zz[1]);
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
    char *zz[33];
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
        return parse_line_feature(player_ptr->current_floor_ptr, qtwg_ptr->buf);
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

    parse_error_type parse_result_Q = i2enum<parse_error_type>(parse_qtw_Q(qtwg_ptr, zz));
    if (parse_result_Q != PARSE_CONTINUE) {
        return parse_result_Q;
    }

    if (qtwg_ptr->buf[0] == 'W') {
        return parse_line_wilderness(player_ptr, qtwg_ptr->buf, qtwg_ptr->xmin, qtwg_ptr->xmax, qtwg_ptr->y, qtwg_ptr->x);
    }

    if (parse_qtw_P(player_ptr, qtwg_ptr, zz)) {
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
