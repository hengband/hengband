/*!
 * @brief モンスター種族の集合論的処理実装
 * @author Hourier
 * @date 2024/12/03
 */

#include "system/monrace/monrace-list.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monster-race-info.h"
#include "system/redrawing-flags-updater.h"
#include "tracking/lore-tracker.h"
#include "util/probability-table.h"
#include <algorithm>

std::map<MonraceId, MonraceDefinition> monraces_info;

const std::map<MonraceId, std::set<MonraceId>> MonraceList::unified_uniques = {
    { MonraceId::BANORLUPART, { MonraceId::BANOR, MonraceId::LUPART } },
};

MonraceList MonraceList::instance{};

bool MonraceList::is_valid(MonraceId monrace_id)
{
    return monrace_id != MonraceId::PLAYER;
}

const std::map<MonraceId, std::set<MonraceId>> &MonraceList::get_unified_uniques()
{
    return unified_uniques;
}

MonraceList &MonraceList::get_instance()
{
    return instance;
}

/*!
 * @brief どのモンスター種族でもない事を意味する MonraceId を返す
 * @details 実態は MonraceId::PLAYER だが、この値は実際にプレイヤーとしての意味として使われる場合
 * （召喚主がプレイヤーの場合やマップ上の表示属性情報等）とどのモンスターでもない意味として使われる場合があるので、
 * 後者ではこれを使用することでコード上の意図をわかりやすくする。
 *
 * @return (どのモンスター種族でもないという意味での) MonraceId::PLAYER を返す
 */
MonraceId MonraceList::empty_id()
{
    return MonraceId::PLAYER;
}

bool MonraceList::is_tsuchinoko(MonraceId monrace_id)
{
    return monrace_id == MonraceId::TSUCHINOKO;
}

std::map<MonraceId, MonraceDefinition>::iterator MonraceList::begin()
{
    return monraces_info.begin();
}

std::map<MonraceId, MonraceDefinition>::const_iterator MonraceList::begin() const
{
    return monraces_info.cbegin();
}

std::map<MonraceId, MonraceDefinition>::iterator MonraceList::end()
{
    return monraces_info.end();
}

std::map<MonraceId, MonraceDefinition>::const_iterator MonraceList::end() const
{
    return monraces_info.cend();
}

std::map<MonraceId, MonraceDefinition>::reverse_iterator MonraceList::rbegin()
{
    return monraces_info.rbegin();
}

std::map<MonraceId, MonraceDefinition>::const_reverse_iterator MonraceList::rbegin() const
{
    return monraces_info.crbegin();
}

std::map<MonraceId, MonraceDefinition>::reverse_iterator MonraceList::rend()
{
    return monraces_info.rend();
}

std::map<MonraceId, MonraceDefinition>::const_reverse_iterator MonraceList::rend() const
{
    return monraces_info.crend();
}

size_t MonraceList::size() const
{
    return monraces_info.size();
}

MonraceDefinition &MonraceList::emplace(MonraceId monrace_id)
{
    return monraces_info.emplace_hint(monraces_info.end(), monrace_id, MonraceDefinition{})->second;
}

std::map<MonraceId, MonraceDefinition> &MonraceList::get_raw_map()
{
    return monraces_info;
}

/*!
 * @brief モンスター定義を種族IDから直接得る (非const版)
 * @param monrace_id モンスター種族ID
 * @return モンスター定義への参照
 * @details モンスター実体からモンスター定義を得るためには使用しないこと
 */
MonraceDefinition &MonraceList::get_monrace(MonraceId monrace_id)
{
    return monraces_info.at(monrace_id);
}

/*!
 * @brief モンスター定義を種族IDから直接得る (const版)
 * @param monrace_id モンスター種族ID
 * @return モンスター定義への参照
 * @details モンスター実体からモンスター定義を得るためには使用しないこと
 */
const MonraceDefinition &MonraceList::get_monrace(MonraceId monrace_id) const
{
    return monraces_info.at(monrace_id);
}

const std::vector<MonraceId> &MonraceList::get_valid_monrace_ids() const
{
    static std::vector<MonraceId> valid_monraces;
    if (!valid_monraces.empty()) {
        return valid_monraces;
    }

    std::transform(++monraces_info.begin(), monraces_info.end(), std::back_inserter(valid_monraces), [](auto &x) { return x.first; });
    return valid_monraces;
}

//!< @todo ややトリッキーだが、元のmapでMonsterRaceInfo をshared_ptr で持つようにすればかなりスッキリ書けるはず.
const std::vector<std::pair<MonraceId, const MonraceDefinition *>> &MonraceList::get_sorted_monraces() const
{
    static std::vector<std::pair<MonraceId, const MonraceDefinition *>> sorted_monraces;
    if (!sorted_monraces.empty()) {
        return sorted_monraces;
    }

    for (const auto &pair : monraces_info) {
        if (pair.second.is_valid()) {
            sorted_monraces.emplace_back(pair.first, &pair.second);
        }
    }

    std::stable_sort(sorted_monraces.begin(), sorted_monraces.end(), [](const auto &pair1, const auto &pair2) {
        return pair2.second->order_level_strictly(*pair1.second);
    });
    return sorted_monraces;
}

/*!
 * @brief 合体/分離ユニーク判定
 * @param r_idx 調査対象のモンスター種族ID
 * @return 合体/分離ユニークか否か
 * @details 合体/分離ユニークは、賞金首にもランダムクエスト討伐対象にもならない.
 */
bool MonraceList::can_unify_separate(const MonraceId r_idx) const
{
    if (unified_uniques.contains(r_idx)) {
        return true;
    }

    return std::any_of(unified_uniques.begin(), unified_uniques.end(), [&r_idx](const auto &x) { return x.second.contains(r_idx); });
}

/*!
 * @brief 合体ユニークの死亡処理
 * @details 分離/合体が A = B + C + D という図式の時、Aが死亡した場合BとCとDも死亡処理を行う。
 * B・C・Dのいずれかが死亡した場合、その死亡したユニークに加えてAの死亡処理も行う。
 * v3.0.0 α89現在は、分離後のユニーク数は2のみ。3以上は将来の拡張。
 * @param r_idx 実際に死亡したモンスターの種族ID
 */
void MonraceList::kill_unified_unique(const MonraceId r_idx)
{
    const auto it_unique = unified_uniques.find(r_idx);
    if (it_unique != unified_uniques.end()) {
        this->get_monrace(it_unique->first).kill_unique();
        for (const auto separate : it_unique->second) {
            this->get_monrace(separate).kill_unique();
        }

        return;
    }

    for (const auto &[unified_unique, separates] : unified_uniques) {
        const auto it_separate = separates.find(r_idx);
        if (it_separate != separates.end()) {
            this->get_monrace(*it_separate).kill_unique();
            this->get_monrace(unified_unique).kill_unique();
            return;
        }
    }
}

/*!
 * @brief 合体ユニークの生成可能確認
 * @param r_idx 生成しようとしているモンスターの種族ID
 * @return 合体後ユニークが生成可能か否か
 * @details 分離も合体もしないならば常にtrue
 * 分離ユニークもtrueだが、通常レアリティ255のためこのメソッドとは別処理で生成不能
 * 分離/合体が A = B + C + D という図式の時、B・C・Dのいずれか1体がフロア内に生成済の場合、Aの生成を抑制する
 */
bool MonraceList::is_selectable(const MonraceId r_idx) const
{
    const auto it = unified_uniques.find(r_idx);
    if (it == unified_uniques.end()) {
        return true;
    }

    return std::all_of(it->second.begin(), it->second.end(), [this](const auto x) { return !this->get_monrace(x).has_entity(); });
}

/*!
 * @brief 合体ユニークが撃破済の状態でフロアから離脱した時に、各分離ユニークも撃破済状態へと変更する
 */
void MonraceList::defeat_separated_uniques()
{
    for (const auto &[unified_unique, separates] : unified_uniques) {
        if (this->get_monrace(unified_unique).max_num > 0) {
            continue;
        }

        for (const auto separate : separates) {
            auto &monrace = this->get_monrace(separate);
            if (monrace.max_num == 0) {
                continue;
            }

            monrace.kill_unique();
        }
    }
}

bool MonraceList::is_unified(const MonraceId r_idx) const
{
    return unified_uniques.contains(r_idx);
}

/*!
 * @brief 合体ユニークの各分離ユニークが全員フロアにいるかをチェックする
 * @param r_idx 合体ユニークの種族ID
 * @return 全員が現在フロアに生成されているか
 */
bool MonraceList::exists_separates(const MonraceId r_idx) const
{
    const auto &separates = unified_uniques.at(r_idx);
    return std::all_of(separates.begin(), separates.end(), [this](const auto x) { return this->get_monrace(x).has_entity(); });
}

/*!
 * @brief 与えられたIDが分離ユニークのいずれかに一致するかをチェックする
 * @param r_idx 調査対象のモンスター種族ID
 */
bool MonraceList::is_separated(const MonraceId r_idx) const
{
    if (unified_uniques.contains(r_idx)) {
        return false;
    }

    return std::any_of(unified_uniques.begin(), unified_uniques.end(), [&r_idx](const auto &x) { return x.second.contains(r_idx); });
}

/*!
 * @brief 合体ユニークが分離魔法を唱えられるかをチェックする
 * @param monrace_id 分離ユニークの種族ID
 * @param hp 分離ユニークの現在HP
 * @param maxhp 分離ユニークの最大HP (衰弱を含)
 */
bool MonraceList::can_select_separate(const MonraceId monrace_id, const int hp, const int maxhp) const
{
    if (unified_uniques.contains(monrace_id)) {
        return false;
    }

    const auto end = unified_uniques.end();
    const auto it = std::find_if(unified_uniques.begin(), end, [monrace_id](const auto &x) { return x.second.contains(monrace_id); });
    if (it == end) {
        return false;
    }

    auto &found_separates = it->second;
    if (hp >= (maxhp / static_cast<int>(found_separates.size()))) {
        return false;
    }

    return std::all_of(found_separates.begin(), found_separates.end(), [this](const auto x) { return this->get_monrace(x).max_num > 0; });
}

bool MonraceList::order(MonraceId id1, MonraceId id2, bool is_detailed) const
{
    const auto &monrace1 = monraces_info[id1];
    const auto &monrace2 = monraces_info[id2];
    if (is_detailed) {
        const auto pkills1 = monrace1.r_pkills;
        const auto pkills2 = monrace2.r_pkills;
        if (pkills1 < pkills2) {
            return true;
        }

        if (pkills1 > pkills2) {
            return false;
        }

        const auto tkills1 = monrace1.r_tkills;
        const auto tkills2 = monrace2.r_tkills;
        if (tkills1 < tkills2) {
            return true;
        }

        if (tkills1 > tkills2) {
            return false;
        }
    }

    const auto level1 = monrace1.level;
    const auto level2 = monrace2.level;
    if (level1 < level2) {
        return true;
    }

    if (level1 > level2) {
        return false;
    }

    const auto exp1 = monrace1.mexp;
    const auto exp2 = monrace2.mexp;
    if (exp1 < exp2) {
        return true;
    }

    if (exp1 > exp2) {
        return false;
    }

    return id1 < id2;
}

bool MonraceList::order_level(MonraceId id1, MonraceId id2) const
{
    const auto &monrace1 = this->get_monrace(id1);
    const auto &monrace2 = this->get_monrace(id2);
    return monrace1.order_level_strictly(monrace2);
}

bool MonraceList::order_level_unique(MonraceId id1, MonraceId id2) const
{
    const auto &monrace1 = this->get_monrace(id1);
    const auto &monrace2 = this->get_monrace(id2);
    const auto order_level = monrace2.order_level(monrace1);
    if (order_level) {
        return *order_level;
    }

    if (monrace1.kind_flags.has_not(MonsterKindType::UNIQUE) && monrace2.kind_flags.has(MonsterKindType::UNIQUE)) {
        return true;
    }

    if (monrace1.kind_flags.has(MonsterKindType::UNIQUE) && monrace2.kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return false;
    }

    return id1 < id2;
}

/*!
 * @brief (MonraceId::PLAYERを除く)実在するすべてのモンスター種族IDから等確率で1つ選択する
 *
 * @return 選択したモンスター種族ID
 */
MonraceId MonraceList::pick_id_at_random() const
{
    static ProbabilityTable<MonraceId> table;
    if (table.empty()) {
        for (const auto &[monrace_id, monrace] : monraces_info) {
            if (monrace.is_valid()) {
                table.entry_item(monrace_id, 1);
            }
        }
    }

    return table.pick_one_at_random();
}

const MonraceDefinition &MonraceList::pick_monrace_at_random() const
{
    return monraces_info.at(this->pick_id_at_random());
}

int MonraceList::calc_defeat_count() const
{
    auto total = 0;
    for (const auto &[_, monrace] : monraces_info) {
        if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            if (monrace.max_num == 0) {
                total++;
            }

            continue;
        }

        if (monrace.r_pkills > 0) {
            total += monrace.r_pkills;
        }
    }

    return total;
}

/*!
 * @brief 現在フロアに存在している1種別辺りのモンスター数を全てリセットする
 * @todo そもそもcur_num はMonsterRaceInfo にいるべきではない、後で分離する
 */
void MonraceList::reset_current_numbers()
{
    for (auto &[_, monrace] : monraces_info) {
        monrace.reset_current_numbers();
    }
}

void MonraceList::reset_all_visuals()
{
    for (auto &[_, monrace] : monraces_info) {
        monrace.symbol_config = monrace.symbol_definition;
    }
}

std::optional<std::string> MonraceList::probe_lore(MonraceId monrace_id)
{
    if (LoreTracker::get_instance().is_tracking(monrace_id)) {
        RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::MONSTER_LORE);
    }

    return this->get_monrace(monrace_id).probe_lore();
}

/*
 * @brief ユニークの死亡処理
 * @param monrace_id 死亡したユニークの種族番号
 */
void MonraceList::kill_unique_monster(MonraceId monrace_id)
{
    this->get_monrace(monrace_id).max_num = 0;
    if (this->can_unify_separate(monrace_id)) {
        this->kill_unified_unique(monrace_id);
    }
}
