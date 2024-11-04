#include "system/floor-type-definition.h"
#include "dungeon/quest.h"
#include "floor/geometry.h"
#include "game-option/birth-options.h"
#include "monster/monster-timed-effects.h"
#include "system/angband-system.h"
#include "system/artifact-type-definition.h"
#include "system/dungeon-info.h"
#include "system/enums/grid-count-kind.h"
#include "system/gamevalue.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/terrain-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-range.h"

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

dungeon_type &FloorType::get_dungeon_definition() const
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
 * @brief 生成階に応じた財宝を生成する.
 * @param initial_offset 財宝を価値の低い順に並べた時の番号 (0スタート、nulloptならば乱数で決定)
 * @return 財宝データで初期化したアイテム
 */
ItemEntity FloorType::make_gold(std::optional<int> initial_offset) const
{
    int offset;
    if (initial_offset) {
        offset = *initial_offset;
    } else {
        offset = ((randint1(this->object_level + 2) + 2) / 2) - 1;
        if (one_in_(CHANCE_BASEITEM_LEVEL_BOOST)) {
            offset += randint1(this->object_level + 1);
        }
    }

    const auto &baseitems = BaseitemList::get_instance();
    const auto num_gold_subtypes = baseitems.calc_num_gold_subtypes();
    if (offset >= num_gold_subtypes) {
        offset = num_gold_subtypes - 1;
    }

    const auto &baseitem = baseitems.lookup_gold(offset);
    const auto base = baseitem.cost;
    ItemEntity item(baseitem.bi_key);
    item.pval = base + (8 * randint1(base)) + randint1(8);
    return item;
}

/*!
 * @brief INSTA_ART型の固定アーティファクトの生成を確率に応じて試行する
 * @param item 生成に割り当てたいアイテムへの参照
 * @return 生成に成功したらTRUEを返す
 */
std::optional<ItemEntity> FloorType::try_make_instant_artifact() const
{
    /*! @note 地上ではキャンセルする / No artifacts in the town */
    if (!this->is_in_underground()) {
        return std::nullopt;
    }

    /*! @note get_obj_index_hookによる指定がある場合は生成をキャンセルする / Themed object */
    if (get_obj_index_hook) {
        return std::nullopt;
    }

    /*! @note 全固定アーティファクト中からIDの若い順に生成対象とその確率を走査する / Check the artifact list (just the "specials") */
    for (const auto &[fa_id, artifact] : ArtifactList::get_instance()) {
        /*! @note 既に生成回数がカウントされたアーティファクト、QUESTITEMと非INSTA_ARTは除外 / Cannot make an artifact twice */
        if (artifact.is_generated) {
            continue;
        }
        if (artifact.gen_flags.has(ItemGenerationTraitType::QUESTITEM)) {
            continue;
        }
        if (!(artifact.gen_flags.has(ItemGenerationTraitType::INSTA_ART))) {
            continue;
        }

        /*! @note アーティファクト生成階が現在に対して足りない場合は高確率で1/(不足階層*2)を満たさないと生成リストに加えられない */
        if (artifact.level > this->object_level) {
            /* @note  / Acquire the "out-of-depth factor". Roll for out-of-depth creation. */
            int d = (artifact.level - this->object_level) * 2;
            if (!one_in_(d)) {
                continue;
            }
        }

        /*! @note 1/(レア度)の確率を満たさないと除外される / Artifact "rarity roll" */
        if (!one_in_(artifact.rarity)) {
            continue;
        }

        /*!
         * @note INSTA_ART型固定アーティファクトのベースアイテムもチェック対象とする。
         * ベースアイテムの生成階層が足りない場合1/(不足階層*5)を満たさないと除外される。
         */
        const auto &baseitems = BaseitemList::get_instance();
        const auto &baseitem = baseitems.lookup_baseitem(artifact.bi_key);
        if (baseitem.level > this->object_level) {
            int d = (baseitem.level - this->object_level) * 5;
            if (!one_in_(d)) {
                continue;
            }
        }

        //<! @note 前述の条件を満たしたら、後のIDのアーティファクトはチェックせずすぐ確定し生成処理に移す.
        ItemEntity instant_artifact(artifact.bi_key);
        instant_artifact.fa_id = fa_id;
        return instant_artifact;
    }

    /*! @note 全INSTA_ART固定アーティファクトを試行しても決まらなかった場合 FALSEを返す / Failure */
    return std::nullopt;
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
