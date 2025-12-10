#include "system/floor/floor-info.h"
#include "dungeon/quest.h"
#include "floor/geometry.h"
#include "game-option/birth-options.h"
#include "locale/language-switcher.h"
#include "monster/monster-timed-effects.h"
#include "object-enchant/item-apply-magic.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem/baseitem-allocation.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/enums/grid-count-kind.h"
#include "system/enums/monrace/monrace-hook-types.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/wilderness-grid.h"
#include "system/gamevalue.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/services/dungeon-monrace-service.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-range.h"
#include "util/point-2d.h"
#include "world/world.h"
#include <range/v3/algorithm.hpp>

FloorType::FloorType()
    : grid_array(MAX_HGT, std::vector<Grid>(MAX_WID))
    , o_list(MAX_FLOOR_ITEMS)
    , m_list(MAX_FLOOR_MONSTERS)
    , quest_number(QuestId::NONE)
{
    ranges::generate(this->o_list, [] { return std::make_shared<ItemEntity>(); });

    for (const auto mte : MONSTER_TIMED_EFFECT_RANGE) {
        this->mproc_list[mte] = std::vector<short>(MAX_FLOOR_MONSTERS, {});
        this->mproc_max[mte] = 0;
    }
}

int FloorType::get_level() const
{
    return this->dun_level;
}

Grid &FloorType::get_grid(const Pos2D &pos)
{
    return this->grid_array[pos.y][pos.x];
}

const Grid &FloorType::get_grid(const Pos2D &pos) const
{
    return this->grid_array[pos.y][pos.x];
}

Rect2D FloorType::get_area(FloorBoundary fb) const
{
    switch (fb) {
    case FloorBoundary::OUTER_WALL_EXCLUSIVE:
        return Rect2D(1, 1, this->height - 2, this->width - 2);
    case FloorBoundary::OUTER_WALL_INCLUSIVE:
        return Rect2D(0, 0, this->height - 1, this->width - 1);
    default:
        THROW_EXCEPTION(std::logic_error, fmt::format("Invalid FloorBoundary is specified!: {}", enum2i(fb)));
    }
}

bool FloorType::is_entering_dungeon() const
{
    return this->entering_dungeon;
}

bool FloorType::is_leaving_dungeon() const
{
    return this->leaving_dungeon;
}

bool FloorType::is_underground() const
{
    return this->dun_level > 0;
}

bool FloorType::is_in_quest() const
{
    return this->quest_number != QuestId::NONE;
}

void FloorType::set_dungeon_index(DungeonId id)
{
    this->dungeon_id = id;
}

void FloorType::reset_dungeon_index()
{
    this->set_dungeon_index(DungeonId::WILDERNESS);
}

const DungeonDefinition &FloorType::get_dungeon_definition() const
{
    return DungeonList::get_instance().get_dungeon(this->dungeon_id);
}

/*!
 * @brief 新しく入ったダンジョンの階層に固定されているランダムクエストを探し出しIDを返す。
 * @param level 検索対象になる階
 * @return クエストIDを返す。該当がない場合0を返す。
 */
QuestId FloorType::get_random_quest_id(tl::optional<int> level_opt) const
{
    if (this->dungeon_id != DungeonId::ANGBAND) {
        return QuestId::NONE;
    }

    const auto level = level_opt.value_or(this->dun_level);
    const auto &quests = QuestList::get_instance();
    for (auto quest_id : RANDOM_QUEST_ID_RANGE) {
        const auto &quest = quests.get_quest(quest_id);
        auto is_random_quest = (quest.type == QuestKindType::RANDOM);
        is_random_quest &= (quest.status == QuestStatusType::TAKEN);
        is_random_quest &= (quest.level == level);
        is_random_quest &= (quest.dungeon == DungeonId::ANGBAND);
        if (is_random_quest) {
            return quest_id;
        }
    }

    return QuestId::NONE;
}

/*!
 * @brief 新しく入ったダンジョンの階層に固定されている一般のクエストを探し出しIDを返す.
 * @param bonus 検索対象になる階へのボーナス。通常0
 * @return クエストIDを返す。該当がない場合0を返す。
 */
QuestId FloorType::get_quest_id(int bonus) const
{
    const auto &quests = QuestList::get_instance();
    if (this->is_in_quest()) {
        return this->quest_number;
    }

    const auto level = this->dun_level + bonus;
    for (const auto &[quest_id, quest] : quests) {
        if (quest.status != QuestStatusType::TAKEN) {
            continue;
        }

        auto depth_quest = (quest.type == QuestKindType::KILL_LEVEL);
        depth_quest &= !(quest.flags & QUEST_FLAG_PRESET);
        depth_quest &= (quest.level == level);
        depth_quest &= (quest.dungeon == this->dungeon_id);
        if (depth_quest) {
            return quest_id;
        }
    }

    return this->get_random_quest_id(level);
}

/*
 * @brief 与えられた座標のグリッドがLOSフラグを持つかを調べる
 * @param pos 座標
 * @return LOSフラグを持つか否か
 */
bool FloorType::has_los_at(const Pos2D &pos) const
{
    return this->get_grid(pos).has_los();
}

bool FloorType::has_los_terrain_at(const Pos2D &pos) const
{
    return this->get_grid(pos).has_los_terrain();
}

bool FloorType::has_terrain_characteristics(const Pos2D &pos, TerrainCharacteristics tc) const
{
    return this->get_grid(pos).has(tc);
}

/*!
 * @brief 特別なフロアにいるかを判定する
 * @return 固定クエスト、アリーナ、モンスター闘技場のいずれかならばtrue
 */
bool FloorType::is_special() const
{
    auto is_in_fixed_quest = this->is_in_quest();
    is_in_fixed_quest &= !inside_quest(this->get_random_quest_id());
    return is_in_fixed_quest || this->inside_arena || AngbandSystem::get_instance().is_phase_out();
}

/*!
 * @brief テレポート・レベル無効フロアの判定
 * @param to_player プレイヤーを対象としているか否か
 * @return テレポート・レベルが不可能ならばtrue
 */
bool FloorType::can_teleport_level(bool to_player) const
{
    auto is_invalid_floor = to_player;
    is_invalid_floor &= inside_quest(this->get_quest_id()) || (this->dun_level >= this->get_dungeon_definition().maxdepth);
    is_invalid_floor &= this->dun_level >= 1;
    is_invalid_floor &= ironman_downward;
    return this->is_special() || is_invalid_floor;
}

bool FloorType::has_marked_grid_at(const Pos2D &pos) const
{
    return this->get_grid(pos).is_mark();
}

bool FloorType::has_closed_door_at(const Pos2D &pos, bool is_mimic) const
{
    return this->get_grid(pos).is_closed_door(is_mimic);
}

bool FloorType::has_trap_at(const Pos2D &pos) const
{
    return this->get_grid(pos).has(TerrainCharacteristics::TRAP);
}

/*!
 * @brief プレイヤーの周辺9マスに該当する地形がいくつあるかを返す
 * @param p_pos プレイヤーの現在位置
 * @param gck 判定条件
 * @param under TRUEならばプレイヤーの直下の座標も走査対象にする
 * @return 該当する地形の数と、該当する地形の中から1つの方向
 */
std::pair<int, Direction> FloorType::count_doors_traps(const Pos2D &p_pos, GridCountKind gck, bool under) const
{
    auto count = 0;
    auto dir = Direction::none();
    for (const auto &d : under ? Direction::directions() : Direction::directions_8()) {
        const auto pos_neighbor = p_pos + d.vec();
        if (!this->has_marked_grid_at(pos_neighbor)) {
            continue;
        }

        if (!this->check_terrain_state(pos_neighbor, gck)) {
            continue;
        }

        ++count;
        dir = d;
    }

    return { count, dir };
}

bool FloorType::check_terrain_state(const Pos2D &pos, GridCountKind gck) const
{
    const auto &grid = this->get_grid(pos);
    switch (gck) {
    case GridCountKind::OPEN: {
        const auto is_open_grid = grid.is_open();
        const auto is_open_dungeon = this->get_dungeon_definition().is_open(grid.get_terrain_id(TerrainKind::MIMIC));
        return is_open_grid && is_open_dungeon;
    }
    case GridCountKind::CLOSED_DOOR:
        return grid.is_closed_door(true);
    case GridCountKind::TRAP:
        return grid.has(TerrainCharacteristics::TRAP);
    default:
        THROW_EXCEPTION(std::logic_error, format("Invalid GridCountKind is Specified! %d", enum2i(gck)));
    }
}

/*!
 * @brief 魔法の笛でペットを呼び寄せる順番を決める
 * @param index1 フロア内のモンスターインデックス1
 * @param index2 フロア内のモンスターインデックス2
 * @return index1の優先度が高いならtrue、低いならfalse
 */
bool FloorType::order_pet_whistle(short index1, short index2) const
{
    const auto &monster1 = this->m_list[index1];
    const auto &monster2 = this->m_list[index2];
    const auto is_ordered = monster1.order_pet_whistle(monster2);
    if (is_ordered) {
        return *is_ordered;
    }

    return index1 < index2;
}

/*!
 * @brief ペットを開放する順番を決める
 * @param index1 フロア内のモンスターインデックス1
 * @param index2 フロア内のモンスターインデックス2
 * @return index1の優先度が高いならtrue、低いならfalse
 */
bool FloorType::order_pet_dismission(short index1, short index2, short riding_index) const
{
    const auto &monster1 = this->m_list.at(index1);
    const auto &monster2 = this->m_list.at(index2);
    if (index1 == riding_index) {
        return true;
    }

    if (index2 == riding_index) {
        return false;
    }

    const auto is_ordered = monster1.order_pet_dismission(monster2);
    if (is_ordered) {
        return *is_ordered;
    }

    return index1 < index2;
}

bool FloorType::contains(const Pos2D &pos, FloorBoundary fb) const
{
    switch (fb) {
    case FloorBoundary::OUTER_WALL_EXCLUSIVE:
        return (pos.y > 0) && (pos.x > 0) && (pos.y < this->height - 1) && (pos.x < this->width - 1);
    case FloorBoundary::OUTER_WALL_INCLUSIVE:
        return (pos.y >= 0) && (pos.x >= 0) && (pos.y < this->height) && (pos.x < this->width);
    default:
        THROW_EXCEPTION(std::logic_error, fmt::format("Invalid LocationDecision is specified! {}", enum2i(fb)));
    }
}

bool FloorType::is_empty_at(const Pos2D &pos) const
{
    return this->get_grid(pos).is_empty();
}

bool FloorType::can_generate_monster_at(const Pos2D &pos) const
{
    return this->get_grid(pos).can_generate_monster();
}

bool FloorType::can_block_disintegration_at(const Pos2D &pos) const
{
    return this->get_grid(pos).can_block_disintegration();
}

bool FloorType::can_drop_item_at(const Pos2D &pos) const
{
    return this->get_grid(pos).can_drop_item();
}

/*!
 * @brief 指定された座標のマスが現在照らされているかを調べる.
 * @param p_pos プレイヤーの現在位置
 * @param pos 指定座標
 * @return 指定された座標に照明がかかっているならTRUEを返す.
 */
bool FloorType::is_illuminated_at(const Pos2D &p_pos, const Pos2D &pos) const
{
    const auto yy = (pos.y < p_pos.y) ? (pos.y + 1) : (pos.y > p_pos.y) ? (pos.y - 1)
                                                                        : pos.y;
    const auto xx = (pos.x < p_pos.x) ? (pos.x + 1) : (pos.x > p_pos.x) ? (pos.x - 1)
                                                                        : pos.x;
    const auto &grid_yyxx = this->get_grid({ yy, xx });
    const auto &grid_yxx = this->get_grid({ pos.y, xx });
    const auto &grid_yyx = this->get_grid({ yy, pos.x });
    auto is_illuminated = grid_yyxx.has_los_terrain(TerrainKind::MIMIC) && any_bits(grid_yyxx.info, CAVE_GLOW);
    is_illuminated |= grid_yxx.has_los_terrain(TerrainKind::MIMIC) && any_bits(grid_yxx.info, CAVE_GLOW);
    is_illuminated |= grid_yyx.has_los_terrain(TerrainKind::MIMIC) && any_bits(grid_yyx.info, CAVE_GLOW);
    return is_illuminated;
}

/*!
 * @brief 特定の財宝を生成する。指定がない場合、生成階に応じたランダムな財宝を生成する。
 * @param bi_key 財宝を固定生成する場合のBaseitemKey
 * @return 財宝データで初期化したアイテム
 * @details 生成レベルが0になったら1に補正する
 */
ItemEntity FloorType::make_gold(tl::optional<BaseitemKey> bi_key) const
{
    ItemEntity item;
    if (bi_key) {
        item = ItemEntity(*bi_key);
    } else {
        const auto level = this->object_level <= 0 ? 1 : this->object_level;
        item = ItemEntity(this->select_baseitem_id(level, AM_GOLD));
    }

    const auto base = item.get_baseitem_cost();
    item.pval = 3 * base + randint1(6 * base);
    return item;
}

/*!
 * @brief INSTA_ART型の固定アーティファクトの生成を確率に応じて試行する
 * @return 生成したアイテム (失敗したらnullopt)
 * @details 地上生成は禁止、個々のアーティファクト生成条件及び生成確率を潜り抜けなければ生成失敗とする
 * 最初に潜り抜けたINSTA_ART型の固定アーティファクトを生成し、以後はチェックせずスキップする
 */
tl::optional<ItemEntity> FloorType::try_make_instant_artifact() const
{
    if (!this->is_underground()) {
        return tl::nullopt;
    }

    return ArtifactList::get_instance().try_make_instant_artifact(this->object_level);
}

/*!
 * @brief アイテム生成テーブルからベースアイテムIDを取得する
 * @param level_initial 生成基準階層 (天界や地獄の階層もそのまま渡ってくる)
 * @param mode 生成モード
 * @return 選ばれたベースアイテムID
 */
short FloorType::select_baseitem_id(int level_initial, uint32_t mode) const
{
    auto level = level_initial;
    if (level > MAX_DEPTH - 1) {
        level = MAX_DEPTH - 1;
    }

    if ((level > 0) && this->get_dungeon_definition().flags.has_not(DungeonFeatureType::BEGINNER)) {
        if (one_in_(CHANCE_BASEITEM_LEVEL_BOOST)) {
            level = 1 + (level * MAX_DEPTH / randint1(MAX_DEPTH));
        }
    }

    const auto &table = BaseitemAllocationTable::get_instance();
    return table.draw_lottery(level, mode, decide_selection_count());
}

bool FloorType::filter_monrace_terrain(MonraceId monrace_id, MonraceHookTerrain hook) const
{
    const auto is_suitable_for_dungeon = !this->is_underground() || DungeonMonraceService::is_suitable_for_dungeon(this->dungeon_id, monrace_id);
    const auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);
    switch (hook) {
    case MonraceHookTerrain::NONE:
        return true;
    case MonraceHookTerrain::FLOOR:
        return monrace.is_suitable_for_floor();
    case MonraceHookTerrain::SHALLOW_WATER:
        return is_suitable_for_dungeon && monrace.is_suitable_for_shallow_water();
    case MonraceHookTerrain::DEEP_WATER:
        return is_suitable_for_dungeon && monrace.is_suitable_for_deep_water();
    case MonraceHookTerrain::TRAPPED_PIT:
        return is_suitable_for_dungeon && monrace.is_suitable_for_special_room() && monrace.is_suitable_for_trapped_pit();
    case MonraceHookTerrain::LAVA:
        return is_suitable_for_dungeon && monrace.is_suitable_for_lava();
    default:
        THROW_EXCEPTION(std::logic_error, format("Invalid monrace hook type is specified! %d", enum2i(hook)));
    }
}

/*!
 * @brief 基本トラップをランダムに選択する
 * @return 選択したトラップのタグ (トラップドアでないならばそのタグ)
 * @details トラップドアは、アリーナ・クエスト・ダンジョンの最下層には設置しない.
 */
TerrainTag FloorType::select_random_trap() const
{
    const auto &terrains = TerrainList::get_instance();
    while (true) {
        const auto tag = terrains.select_normal_trap();
        if (terrains.get_terrain(tag).flags.has_not(TerrainCharacteristics::MORE)) {
            return tag;
        }

        if (this->inside_arena || inside_quest(this->get_quest_id())) {
            continue;
        }

        if (this->dun_level >= this->get_dungeon_definition().maxdepth) {
            continue;
        }

        return tag;
    }
}

MonraceHook FloorType::get_monrace_hook() const
{
    if (this->is_underground()) {
        return MonraceHook::DUNGEON;
    }

    return WildernessGrids::get_instance().get_monrace_hook();
}

/*!
 * @brief 指定された広域マップ座標の地勢を元にモンスターの生成条件関数を返す
 * @return 地勢にあったモンスターの生成条件関数
 */
MonraceHookTerrain FloorType::get_monrace_hook_terrain_at(const Pos2D &pos) const
{
    const auto &terrain = this->get_grid(pos).get_terrain();
    if (terrain.flags.has(TerrainCharacteristics::WATER)) {
        return terrain.flags.has(TerrainCharacteristics::DEEP) ? MonraceHookTerrain::DEEP_WATER : MonraceHookTerrain::SHALLOW_WATER;
    }

    if (terrain.flags.has(TerrainCharacteristics::LAVA)) {
        return MonraceHookTerrain::LAVA;
    }

    return MonraceHookTerrain::FLOOR;
}

void FloorType::enter_dungeon(bool state)
{
    this->entering_dungeon = state;
}

void FloorType::leave_dungeon(bool state)
{
    this->leaving_dungeon = state;
}

/*!
 * @brief モンスターの時限ステータスリストを初期化する
 * @details リストは逆順に走査し、死んでいるモンスターは初期化対象外とする
 */
void FloorType::reset_mproc()
{
    this->reset_mproc_max();
    for (short i = this->m_max - 1; i >= 1; i--) {
        const auto &monster = this->m_list[i];
        if (!monster.is_valid()) {
            continue;
        }

        for (const auto mte : MONSTER_TIMED_EFFECT_RANGE) {
            if (monster.mtimed.at(mte) > 0) {
                this->add_mproc(i, mte);
            }
        }
    }
}

void FloorType::reset_mproc_max()
{
    for (const auto mte : MONSTER_TIMED_EFFECT_RANGE) {
        this->mproc_max[mte] = 0;
    }
}

/*!
 * @brief モンスターの時限ステータスを取得する
 * @param m_idx モンスターの参照ID
 * @param mte モンスターの時限ステータスID
 * @return 残りターン値
 */
tl::optional<int> FloorType::get_mproc_index(short m_idx, MonsterTimedEffect mte)
{
    const auto &cur_mproc_list = this->mproc_list[mte];
    for (auto i = this->mproc_max[mte] - 1; i >= 0; i--) {
        if (cur_mproc_list[i] == m_idx) {
            return i;
        }
    }

    return tl::nullopt;
}

/*!
 * @brief モンスターの時限ステータスリストを追加する
 * @param m_idx モンスターの参照ID
 * @return mte 追加したいモンスターの時限ステータスID
 */
void FloorType::add_mproc(short m_idx, MonsterTimedEffect mte)
{
    if (this->mproc_max[mte] < MAX_FLOOR_MONSTERS) {
        this->mproc_list[mte][this->mproc_max[mte]++] = m_idx;
    }
}

/*!
 * @brief モンスターの時限ステータスリストを削除
 * @return m_idx モンスターの参照ID
 * @return mte 削除したいモンスターの時限ステータスID
 */
void FloorType::remove_mproc(short m_idx, MonsterTimedEffect mte)
{
    const auto mproc_idx = this->get_mproc_index(m_idx, mte);
    if (mproc_idx >= 0) {
        this->mproc_list[mte][*mproc_idx] = this->mproc_list[mte][--this->mproc_max[mte]];
    }
}

/*!
 * @brief モンスター配列の空きを探す
 * @return 使われていないモンスターのフロア内インデックス
 */
short FloorType::pop_empty_index_monster()
{
    /* Normal allocation */
    if (this->m_max < MAX_FLOOR_MONSTERS) {
        const auto i = this->m_max;
        this->m_max++;
        this->m_cnt++;
        return i;
    }

    /* Recycle dead monsters */
    for (short i = 1; i < this->m_max; i++) {
        const auto &monster = this->m_list[i];
        if (monster.is_valid()) {
            continue;
        }

        this->m_cnt++;
        return i;
    }

    if (AngbandWorld::get_instance().character_dungeon) {
        THROW_EXCEPTION(std::runtime_error, _("モンスターが多すぎる！", "Too many monsters!"));
    }

    return 0;
}

/*!
 * @brief アイテム配列から空きを取得する
 * @return 使われていないアイテムのフロア内インデックス
 */
short FloorType::pop_empty_index_item()
{
    if (this->o_list.size() < MAX_FLOOR_ITEMS) {
        this->o_list.push_back(std::make_shared<ItemEntity>());
        return static_cast<short>(this->o_list.size() - 1);
    }

    if (AngbandWorld::get_instance().character_dungeon) {
        THROW_EXCEPTION(std::runtime_error, _("アイテムが多すぎる！", "Too many items!"));
    }

    return 0;
}

/*!
 * @brief ターン経過に伴いグリッドの灯りを消去する
 */
void FloorType::forget_lite()
{
    if (this->lite_n == 0) {
        return;
    }

    for (auto i = 0; i < this->lite_n; i++) {
        const auto y = this->lite_y[i];
        const auto x = this->lite_x[i];
        this->grid_array[y][x].info &= ~(CAVE_LITE);
    }

    this->lite_n = 0;
}

/*!
 * @brief ターン経過に伴いプレイヤーの視界を消去する
 */
void FloorType::forget_view()
{
    if (this->view_n == 0) {
        return;
    }

    for (auto i = 0; i < this->view_n; i++) {
        const auto y = this->view_y[i];
        const auto x = this->view_x[i];
        this->grid_array[y][x].info &= ~(CAVE_VIEW);
    }

    this->view_n = 0;
}

/*!
 * @brief ターン経過に伴いモンスターの灯りを消去する
 */
void FloorType::forget_mon_lite()
{
    for (auto i = 0; i < this->mon_lite_n; i++) {
        const auto y = this->mon_lite_y[i];
        const auto x = this->mon_lite_x[i];
        this->grid_array[y][x].info &= ~(CAVE_MNLT | CAVE_MNDK);
    }

    this->mon_lite_n = 0;
}

void FloorType::reset_mon_lite()
{
    for (auto i = 0; i < this->mon_lite_n; i++) {
        const auto y = this->mon_lite_y[i];
        const auto x = this->mon_lite_x[i];
        auto &grid = this->get_grid({ y, x });
        grid.add_info((grid.info & CAVE_MNLT) ? CAVE_TEMP : CAVE_XTRA);
        grid.info &= ~(CAVE_MNLT | CAVE_MNDK);
    }
}

void FloorType::reset_lite_area()
{
    this->lite_n = 0;
    this->mon_lite_n = 0;
    this->redraw_n = 0;
    this->view_n = 0;
}

void FloorType::set_lite_at(const Pos2D &pos)
{
    if (!this->contains(pos, FloorBoundary::OUTER_WALL_INCLUSIVE)) {
        return;
    }

    auto &grid = this->get_grid(pos);
    if (grid.is_lite() || (this->lite_n >= LITE_MAX)) {
        return;
    }

    grid.info |= CAVE_LITE;
    this->lite_y[this->lite_n] = pos.y;
    this->lite_x[this->lite_n++] = pos.x;
}

void FloorType::set_redraw_at(const Pos2D &pos)
{
    if (!this->contains(pos, FloorBoundary::OUTER_WALL_INCLUSIVE)) {
        return;
    }

    auto &grid = this->get_grid(pos);
    if (grid.is_redraw() || (this->redraw_n >= REDRAW_MAX)) {
        return;
    }

    grid.info |= CAVE_REDRAW;
    this->redraw_y[this->redraw_n] = pos.y;
    this->redraw_x[this->redraw_n++] = pos.x;
}

void FloorType::set_view_at(const Pos2D &pos)
{
    if (!this->contains(pos, FloorBoundary::OUTER_WALL_EXCLUSIVE)) {
        return;
    }

    auto &grid = this->get_grid(pos);
    if (grid.is_view() || (this->view_n >= VIEW_MAX)) {
        return;
    }

    grid.info |= CAVE_VIEW;
    this->view_y[this->view_n] = pos.y;
    this->view_x[this->view_n] = pos.x;
    this->view_n++;
}

void FloorType::set_view()
{
    for (auto i = 0; i < this->view_n; i++) {
        const auto y = this->view_y[i];
        const auto x = this->view_x[i];
        const Pos2D pos(y, x);
        auto &grid = this->get_grid(pos);
        grid.info &= ~(CAVE_XTRA);
        if (grid.info & CAVE_TEMP) {
            continue;
        }

        this->set_note_and_redraw_at(pos);
    }
}

void FloorType::set_note_and_redraw()
{
    for (auto i = 0; i < this->lite_n; i++) {
        const auto y = this->lite_y[i];
        const auto x = this->lite_x[i];
        const Pos2D pos(y, x);
        if (this->get_grid(pos).info & CAVE_TEMP) {
            continue;
        }

        this->set_note_and_redraw_at(pos);
    }
}

std::vector<Pos2D> FloorType::reset_lite()
{
    std::vector<Pos2D> points;
    for (auto i = 0; i < this->lite_n; i++) {
        const auto y = this->lite_y[i];
        const auto x = this->lite_x[i];
        const Pos2D pos(y, x);
        auto &grid = this->get_grid(pos);
        grid.info &= ~(CAVE_LITE);
        grid.info |= CAVE_TEMP;
        points.push_back(pos);
    }

    this->lite_n = 0;
    return points;
}

std::vector<Pos2D> FloorType::reset_view()
{
    std::vector<Pos2D> points;
    for (auto i = 0; i < this->view_n; i++) {
        const auto y = this->view_y[i];
        const auto x = this->view_x[i];
        const Pos2D pos(y, x);
        auto &grid = this->get_grid(pos);
        grid.info &= ~(CAVE_VIEW);
        grid.info |= CAVE_TEMP;
        points.push_back(pos);
    }
    this->view_n = 0;
    return points;
}

std::vector<Pos2D> FloorType::collect_temp_mon_lite()
{
    std::vector<Pos2D> points;
    for (auto i = 0; i < this->mon_lite_n; i++) {
        const auto fx = this->mon_lite_x[i];
        const auto fy = this->mon_lite_y[i];
        const auto pos = Pos2D(fy, fx);
        const auto &grid = this->get_grid(pos);
        if (grid.info & CAVE_TEMP) {
            if ((grid.info & (CAVE_VIEW | CAVE_MNLT)) == CAVE_VIEW) {
                this->set_note_and_redraw_at(pos);
            }
        } else if ((grid.info & (CAVE_VIEW | CAVE_MNDK)) == CAVE_VIEW) {
            this->set_note_and_redraw_at(pos);
        }

        points.push_back(pos);
    }

    this->mon_lite_n = 0;
    return points;
}

std::vector<Pos2D> FloorType::collect_redraw_points()
{
    std::vector<Pos2D> points;
    for (auto i = 0; i < this->redraw_n; i++) {
        const Pos2D pos(this->redraw_y[i], this->redraw_x[i]);
        auto &grid = this->get_grid(pos);
        if (any_bits(grid.info, CAVE_REDRAW)) {
            points.push_back(pos);
        }
    }

    this->redraw_n = 0;
    return points;
}

void FloorType::set_mon_lite(const std::vector<Pos2D> &points, size_t end_temp)
{
    for (size_t i = 0; i < end_temp; i++) {
        const auto &pos = points[i];
        const auto &grid = this->get_grid(pos);
        if (grid.info & CAVE_MNLT) {
            if ((grid.info & (CAVE_VIEW | CAVE_TEMP)) == CAVE_VIEW) {
                this->set_note_and_redraw_at(pos);
            }
        } else if ((grid.info & (CAVE_VIEW | CAVE_XTRA)) == CAVE_VIEW) {
            this->set_note_and_redraw_at(pos);
        }

        this->mon_lite_x[this->mon_lite_n] = pos.x;
        this->mon_lite_y[this->mon_lite_n] = pos.y;
        this->mon_lite_n++;
    }

    for (size_t i = end_temp; i < std::size(points); i++) {
        const auto &pos = points[i];
        this->get_grid(pos).info &= ~(CAVE_TEMP | CAVE_XTRA);
    }
}

/*!
 * @brief 指定された座標が地震や階段生成の対象となるマスかを返す。
 * @param pos 指定座標
 * @return 永久地形でなく、かつ該当のマスにアーティファクトが存在しないならばtrue、永久地形かアーティファクトが存在するならばfalse。
 */
bool FloorType::is_grid_changeable(const Pos2D &pos) const
{
    const auto &grid = this->get_grid(pos);
    if (grid.has(TerrainCharacteristics::PERMANENT)) {
        return false;
    }

    for (const auto this_o_idx : grid.o_idx_list) {
        const auto &item = *this->o_list[this_o_idx];
        if (item.is_fixed_or_random_artifact()) {
            return false;
        }
    }

    return true;
}

/*!
 * @brief 所定の位置に上り階段か下り階段を配置する
 * @param pos 配置を試みたいマスの座標
 */
void FloorType::place_random_stairs(const Pos2D &pos)
{
    auto &grid = this->get_grid(pos);
    if (!grid.is_floor() || !grid.o_idx_list.empty()) {
        return;
    }

    auto up_stairs = this->is_underground();
    if (ironman_downward) {
        up_stairs = false;
    }

    auto down_stairs = this->dun_level < this->get_dungeon_definition().maxdepth;
    if (inside_quest(this->get_quest_id()) && (this->dun_level > 1)) {
        down_stairs = false;
    }

    if (down_stairs && up_stairs) {
        if (one_in_(2)) {
            up_stairs = false;
        } else {
            down_stairs = false;
        }
    }

    if (up_stairs) {
        grid.set_terrain_id(TerrainTag::UP_STAIR);
        return;
    }

    if (down_stairs) {
        grid.set_terrain_id(TerrainTag::DOWN_STAIR);
    }
}

void FloorType::set_terrain_id_at(const Pos2D &pos, TerrainTag tag, TerrainKind tk)
{
    this->get_grid(pos).set_terrain_id(tag, tk);
}

void FloorType::set_terrain_id_at(const Pos2D &pos, short terrain_id, TerrainKind tk)
{
    this->get_grid(pos).set_terrain_id(terrain_id, tk);
}

/*!
 * @brief マスにトラップを配置する
 * @param pos 配置したいマスの座標
 */
void FloorType::place_trap_at(const Pos2D &pos)
{
    if (!this->contains(pos, FloorBoundary::OUTER_WALL_EXCLUSIVE)) {
        return;
    }

    auto &grid = this->get_grid(pos);
    if (!grid.is_clean()) {
        return;
    }

    grid.mimic = grid.feat;
    grid.set_terrain_id(this->select_random_trap());
}

/*!
 * @brief アイテムの抽選回数をランダムに決定する
 * @return 抽選回数
 * @details 40 % で1回、50 % で2回、10 % で3回.
 * モンスターも同一ルーチンだが将来に亘って同一である保証はないので、アイテムはアイテムで定義する
 */
int FloorType::decide_selection_count()
{
    auto count = 1;
    const auto p = randint0(100);
    if (p < 60) {
        count++;
    }

    if (p < 10) {
        count++;
    }

    return count;
}

void FloorType::set_note_and_redraw_at(const Pos2D &pos)
{
    this->get_grid(pos).info |= CAVE_NOTE;
    this->set_redraw_at(pos);
}
