#include "system/floor-type-definition.h"
#include "dungeon/quest.h"
#include "floor/geometry.h"
#include "game-option/birth-options.h"
#include "monster/monster-timed-effects.h"
#include "object-enchant/item-apply-magic.h"
#include "system/alloc-entries.h"
#include "system/angband-system.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/enums/grid-count-kind.h"
#include "system/gamevalue.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/terrain-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-range.h"
#include "util/probability-table.h"
#include "world/world-object.h"

FloorType::FloorType()
    : grid_array(MAX_HGT, std::vector<Grid>(MAX_WID))
    , o_list(MAX_FLOOR_ITEMS)
    , m_list(MAX_FLOOR_MONSTERS)
    , quest_number(QuestId::NONE)
{
    for (const auto mte : MONSTER_TIMED_EFFECT_RANGE) {
        this->mproc_list[mte] = std::vector<short>(MAX_FLOOR_MONSTERS, {});
        this->mproc_max[mte] = 0;
    }
}

Grid &FloorType::get_grid(const Pos2D pos)
{
    return this->grid_array[pos.y][pos.x];
}

const Grid &FloorType::get_grid(const Pos2D pos) const
{
    return this->grid_array[pos.y][pos.x];
}

bool FloorType::is_in_underground() const
{
    return this->dun_level > 0;
}

bool FloorType::is_in_quest() const
{
    return this->quest_number != QuestId::NONE;
}

void FloorType::set_dungeon_index(short dungeon_idx_)
{
    this->dungeon_idx = dungeon_idx_;
}

void FloorType::reset_dungeon_index()
{
    this->set_dungeon_index(0);
}

DungeonDefinition &FloorType::get_dungeon_definition() const
{
    return dungeons_info[this->dungeon_idx];
}

/*!
 * @brief 新しく入ったダンジョンの階層に固定されているランダムクエストを探し出しIDを返す。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param level 検索対象になる階
 * @return クエストIDを返す。該当がない場合0を返す。
 */
QuestId FloorType::get_random_quest_id(std::optional<int> level_opt) const
{
    if (this->dungeon_idx != DUNGEON_ANGBAND) {
        return QuestId::NONE;
    }

    const auto level = level_opt.value_or(this->dun_level);
    const auto &quests = QuestList::get_instance();
    for (auto quest_id : RANDOM_QUEST_ID_RANGE) {
        const auto &quest = quests.get_quest(quest_id);
        auto is_random_quest = (quest.type == QuestKindType::RANDOM);
        is_random_quest &= (quest.status == QuestStatusType::TAKEN);
        is_random_quest &= (quest.level == level);
        is_random_quest &= (quest.dungeon == DUNGEON_ANGBAND);
        if (is_random_quest) {
            return quest_id;
        }
    }

    return QuestId::NONE;
}

/*!
 * @brief 新しく入ったダンジョンの階層に固定されている一般のクエストを探し出しIDを返す.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param bonus 検索対象になる階へのボーナス。通常0
 * @return クエストIDを返す。該当がない場合0を返す。
 */
QuestId FloorType::get_quest_id(const int bonus) const
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
        depth_quest &= (quest.dungeon == this->dungeon_idx);
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
bool FloorType::has_los(const Pos2D &pos) const
{
    return this->get_grid(pos).has_los();
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

bool FloorType::is_mark(const Pos2D &pos) const
{
    return this->get_grid(pos).is_mark();
}

bool FloorType::is_closed_door(const Pos2D &pos, bool is_mimic) const
{
    const auto &grid = this->get_grid(pos);
    if (is_mimic) {
        return grid.get_terrain_mimic().is_closed_door();
    }

    return grid.get_terrain().is_closed_door();
}

bool FloorType::is_trap(const Pos2D &pos) const
{
    return this->get_grid(pos).get_terrain().is_trap();
}

/*!
 * @brief プレイヤーの周辺9マスに該当する地形がいくつあるかを返す
 * @param p_pos プレイヤーの現在位置
 * @param gck 判定条件
 * @param under TRUEならばプレイヤーの直下の座標も走査対象にする
 * @return 該当する地形の数と、該当する地形の中から1つの座標
 */
std::pair<int, Pos2D> FloorType::count_doors_traps(const Pos2D &p_pos, GridCountKind gck, bool under) const
{
    auto count = 0;
    Pos2D pos(0, 0);
    for (auto d = 0; d < 9; d++) {
        if ((d == 8) && !under) {
            continue;
        }

        Pos2D pos_neighbor = p_pos + Pos2DVec(ddy_ddd[d], ddx_ddd[d]);
        if (!this->is_mark(pos_neighbor)) {
            continue;
        }

        if (!this->check_terrain_state(pos_neighbor, gck)) {
            continue;
        }

        ++count;
        pos = pos_neighbor;
    }

    return { count, pos };
}

bool FloorType::check_terrain_state(const Pos2D &pos, GridCountKind gck) const
{
    const auto &grid = this->get_grid(pos);
    switch (gck) {
    case GridCountKind::OPEN: {
        const auto is_open_grid = grid.get_terrain_mimic().is_open();
        const auto is_open_dungeon = this->get_dungeon_definition().is_open(grid.get_feat_mimic());
        return is_open_grid && is_open_dungeon;
    }
    case GridCountKind::CLOSED_DOOR:
        return grid.get_terrain_mimic().is_closed_door();
    case GridCountKind::TRAP:
        return grid.get_terrain_mimic().is_trap();
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
    const auto &monster1 = this->m_list[index1];
    const auto &monster2 = this->m_list[index2];
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

/*!
 * @brief 特定の財宝を生成する。指定がない場合、生成階に応じたランダムな財宝を生成する。
 * @param bi_key 財宝を固定生成する場合のBaseitemKey
 * @return 財宝データで初期化したアイテム
 * @details 生成レベルが0になったら1に補正する
 */
ItemEntity FloorType::make_gold(std::optional<BaseitemKey> bi_key) const
{
    ItemEntity item;
    if (bi_key) {
        item = ItemEntity(*bi_key);
    } else {
        const auto level = this->object_level <= 0 ? 1 : this->object_level;
        item = ItemEntity(this->get_obj_index(level, AM_GOLD));
    }

    const auto base = item.get_baseitem_cost();
    item.pval = 3 * base + randint1(6 * base);
    return item;
}

/*!
 * @brief INSTA_ART型の固定アーティファクトの生成を確率に応じて試行する
 * @return 生成したアイテム (失敗したらnullopt)
 * @details 地上生成は禁止、生成制限がある場合も禁止、個々のアーティファクト生成条件及び生成確率を潜り抜けなければ生成失敗とする
 * 最初に潜り抜けたINSTA_ART型の固定アーティファクトを生成し、以後はチェックせずスキップする
 */
std::optional<ItemEntity> FloorType::try_make_instant_artifact() const
{
    if (!this->is_in_underground() || (get_obj_index_hook != nullptr)) {
        return std::nullopt;
    }

    return ArtifactList::get_instance().try_make_instant_artifact(this->object_level);
}

/*!
 * @brief アイテム生成テーブルからベースアイテムIDを取得する
 * @param level 生成基準階層
 * @param mode 生成モード
 * @return 選ばれたベースアイテムID
 */
short FloorType::get_obj_index(int level, uint32_t mode) const
{
    if (level > MAX_DEPTH - 1) {
        level = MAX_DEPTH - 1;
    }

    if ((level > 0) && this->get_dungeon_definition().flags.has_not(DungeonFeatureType::BEGINNER)) {
        if (one_in_(CHANCE_BASEITEM_LEVEL_BOOST)) {
            level = 1 + (level * MAX_DEPTH / randint1(MAX_DEPTH));
        }
    }

    // 候補の確率テーブル生成
    const auto &table = BaseitemAllocationTable::get_instance();
    ProbabilityTable<int> prob_table;
    for (size_t i = 0; i < table.size(); i++) {
        const auto &entry = table.get_entry(i);
        if (entry.level > level) {
            break;
        }

        if (any_bits(mode, AM_FORBID_CHEST) && entry.is_chest()) {
            continue;
        }

        if (any_bits(mode, AM_GOLD) && !entry.is_gold()) {
            continue;
        }

        if (none_bits(mode, AM_GOLD) && entry.is_gold()) {
            continue;
        }

        prob_table.entry_item(i, entry.prob2);
    }

    // 候補なし
    if (prob_table.empty()) {
        return 0;
    }

    const auto count = decide_selection_count();
    std::vector<int> result;
    ProbabilityTable<int>::lottery(std::back_inserter(result), prob_table, count);
    const auto it = std::max_element(result.begin(), result.end(), [&table](int a, int b) { return table.order_level(a, b); });
    return table.get_entry(*it).index;
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
std::optional<int> FloorType::get_mproc_index(short m_idx, MonsterTimedEffect mte)
{
    const auto &cur_mproc_list = this->mproc_list[mte];
    for (auto i = this->mproc_max[mte] - 1; i >= 0; i--) {
        if (cur_mproc_list[i] == m_idx) {
            return i;
        }
    }

    return std::nullopt;
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
