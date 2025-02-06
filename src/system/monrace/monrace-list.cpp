/*!
 * @brief モンスター種族の集合論的処理実装
 * @author Hourier
 * @date 2024/12/03
 */

#include "system/monrace/monrace-list.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/redrawing-flags-updater.h"
#include "tracking/lore-tracker.h"
#include "util/probability-table.h"
#include "util/string-processor.h"
#include <algorithm>

namespace {
const std::set<MonraceId> DARK_ELF_RACES = {
    MonraceId::D_ELF,
    MonraceId::D_ELF_MAGE,
    MonraceId::D_ELF_WARRIOR,
    MonraceId::D_ELF_PRIEST,
    MonraceId::D_ELF_LORD,
    MonraceId::D_ELF_WARLOCK,
    MonraceId::D_ELF_DRUID,
    MonraceId::NIGHTBLADE,
    MonraceId::D_ELF_SORC,
    MonraceId::D_ELF_SHADE,
};

const std::set<MonraceId> CHAPEL_RACES = {
    MonraceId::NOV_PRIEST,
    MonraceId::NOV_PALADIN,
    MonraceId::NOV_PRIEST_G,
    MonraceId::NOV_PALADIN_G,
    MonraceId::PRIEST,
    MonraceId::JADE_MONK,
    MonraceId::IVORY_MONK,
    MonraceId::ULTRA_PALADIN,
    MonraceId::EBONY_MONK,
    MonraceId::W_KNIGHT,
    MonraceId::KNI_TEMPLAR,
    MonraceId::PALADIN,
    MonraceId::TOPAZ_MONK,
};

//!< @details 「Aシンボルだが天使ではない」モンスターのリスト.
const std::set<MonraceId> NON_ANGEL_RACES = {
    MonraceId::A_GOLD,
    MonraceId::A_SILVER,
};
}

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

bool MonraceList::is_dark_elf(MonraceId monrace_id)
{
    return DARK_ELF_RACES.contains(monrace_id);
}

bool MonraceList::is_chapel(MonraceId monrace_id)
{
    return CHAPEL_RACES.contains(monrace_id);
}

MonraceDefinition &MonraceList::emplace(MonraceId monrace_id)
{
    return this->monraces.emplace_hint(this->monraces.end(), monrace_id, MonraceDefinition{})->second;
}

/*!
 * @brief モンスター定義を種族IDから直接得る (非const版)
 * @param monrace_id モンスター種族ID
 * @return モンスター定義への参照
 * @details モンスター実体からモンスター定義を得るためには使用しないこと
 */
MonraceDefinition &MonraceList::get_monrace(MonraceId monrace_id)
{
    return this->monraces.at(monrace_id);
}

/*!
 * @brief モンスター定義を種族IDから直接得る (const版)
 * @param monrace_id モンスター種族ID
 * @return モンスター定義への参照
 * @details モンスター実体からモンスター定義を得るためには使用しないこと
 */
const MonraceDefinition &MonraceList::get_monrace(MonraceId monrace_id) const
{
    return this->monraces.at(monrace_id);
}

const std::vector<MonraceId> &MonraceList::get_valid_monrace_ids() const
{
    static std::vector<MonraceId> valid_monraces;
    if (!valid_monraces.empty()) {
        return valid_monraces;
    }

    std::transform(++this->monraces.begin(), this->monraces.end(), std::back_inserter(valid_monraces), [](auto &x) { return x.first; });
    return valid_monraces;
}

/*!
 * @brief モンスターを引数で与えたフィルタ関数で検索する
 *
 * @param filter このフィルタ関数がtrueを返すモンスターを検索する
 * @param is_known_only trueならばプレイヤーが既知のモンスターのみを対象とする。falseならば全てのモンスターを対象とする。
 * @return std::vector<MonraceId> 検索結果のモンスター種族IDリスト
 */
std::vector<MonraceId> MonraceList::search(std::function<bool(const MonraceDefinition &)> filter, bool is_known_only) const
{
    std::vector<MonraceId> result_ids;

    for (const auto &[id, monrace] : this->monraces) {
        if (!monrace.is_valid()) {
            continue;
        }

        if (is_known_only && (monrace.r_sights == 0)) {
            continue;
        }

        if (filter(monrace)) {
            result_ids.push_back(id);
        }
    }

    return result_ids;
}

/*!
 * @brief モンスターを名前で検索する
 *
 * 引数で与えた名前を含む(部分一致)モンスターを検索する。
 *
 * @param name 検索するモンスターの名前
 * @param is_known_only trueならばプレイヤーが既知のモンスターのみを対象とする。falseならば全てのモンスターを対象とする。
 * @return std::vector<MonraceId> 検索結果のモンスター種族IDリスト
 */
std::vector<MonraceId> MonraceList::search_by_name(std::string_view name, bool is_known_only) const
{
    std::vector<MonraceId> result_ids;
    const auto lowered_search_name = str_tolower(name);

    auto filter = [&](const MonraceDefinition &monrace) {
        const auto lowered_en_name = str_tolower(monrace.name.en_string());

#ifdef JP
        return str_find(lowered_en_name, lowered_search_name) || str_find(monrace.name.string(), lowered_search_name);
#else
        return str_find(lowered_en_name, lowered_search_name);
#endif
    };

    return this->search(std::move(filter), is_known_only);
}

/*!
 * @brief モンスターのシンボルで検索する
 *
 * @param symbol 検索するモンスターのシンボル
 * @param is_known_only trueならばプレイヤーが既知のモンスターのみを対象とする。falseならば全てのモンスターを対象とする。
 * @return std::vector<MonraceId> 検索結果のモンスター種族IDリスト
 */
std::vector<MonraceId> MonraceList::search_by_symbol(char symbol, bool is_known_only) const
{
    auto filter = [&](const MonraceDefinition &monrace) {
        return monrace.symbol_char_is_any_of(std::string(1, symbol));
    };

    return this->search(std::move(filter), is_known_only);
}

bool MonraceList::is_angel(MonraceId monrace_id) const
{
    const auto &monrace = this->get_monrace(monrace_id);
    auto is_angel = monrace.is_angel_superficially();
    is_angel &= !NON_ANGEL_RACES.contains(monrace_id);
    return is_angel;
}

/*!
 * @brief 合体/分離ユニーク判定
 * @param monrace_id 調査対象のモンスター種族ID
 * @return 合体/分離ユニークか否か
 * @details 合体/分離ユニークは、賞金首にもランダムクエスト討伐対象にもならない.
 */
bool MonraceList::can_unify_separate(MonraceId monrace_id) const
{
    if (unified_uniques.contains(monrace_id)) {
        return true;
    }

    return std::any_of(unified_uniques.begin(), unified_uniques.end(), [monrace_id](const auto &x) { return x.second.contains(monrace_id); });
}

/*!
 * @brief 合体ユニークの死亡処理
 * @details 分離/合体が A = B + C + D という図式の時、Aが死亡した場合BとCとDも死亡処理を行う。
 * B・C・Dのいずれかが死亡した場合、その死亡したユニークに加えてAの死亡処理も行う。
 * v3.0.0 α89現在は、分離後のユニーク数は2のみ。3以上は将来の拡張。
 * @param monrace_id 実際に死亡したモンスターの種族ID
 */
void MonraceList::kill_unified_unique(MonraceId monrace_id)
{
    const auto it_unique = unified_uniques.find(monrace_id);
    if (it_unique != unified_uniques.end()) {
        for (const auto separate : it_unique->second) {
            this->get_monrace(separate).kill_unique();
        }

        return;
    }

    for (const auto &[unified_unique, separates] : unified_uniques) {
        if (separates.contains(monrace_id)) {
            this->get_monrace(unified_unique).kill_unique();
            return;
        }
    }
}

/*!
 * @brief 合体ユニークの生成可能確認
 * @param monrace_id 生成しようとしているモンスターの種族ID
 * @return 合体後ユニークが生成可能か否か
 * @details 分離も合体もしないならば常にtrue
 * 分離ユニークもtrueだが、通常レアリティ255のためこのメソッドとは別処理で生成不能
 * 分離/合体が A = B + C + D という図式の時、B・C・Dのいずれか1体がフロア内に生成済の場合、Aの生成を抑制する
 */
bool MonraceList::is_selectable(MonraceId monrace_id) const
{
    const auto it = unified_uniques.find(monrace_id);
    if (it == unified_uniques.end()) {
        return true;
    }

    return std::all_of(it->second.begin(), it->second.end(), [this](const auto x) { return !this->get_monrace(x).has_entity(); });
}

bool MonraceList::is_unified(MonraceId monrace_id) const
{
    return unified_uniques.contains(monrace_id);
}

/*!
 * @brief 合体ユニークの各分離ユニークが全員フロアにいるかをチェックする
 * @param monrace_id 合体ユニークの種族ID
 * @return 全員が現在フロアに生成されているか
 */
bool MonraceList::exists_separates(MonraceId monrace_id) const
{
    const auto &separates = unified_uniques.at(monrace_id);
    return std::all_of(separates.begin(), separates.end(), [this](const auto x) { return this->get_monrace(x).has_entity(); });
}

/*!
 * @brief 与えられたIDが分離ユニークのいずれかに一致するかをチェックする
 * @param monrace_id 調査対象のモンスター種族ID
 */
bool MonraceList::is_separated(MonraceId monrace_id) const
{
    if (unified_uniques.contains(monrace_id)) {
        return false;
    }

    return std::any_of(unified_uniques.begin(), unified_uniques.end(), [monrace_id](const auto &x) { return x.second.contains(monrace_id); });
}

/*!
 * @brief 合体ユニークが分離魔法を唱えられるかをチェックする
 * @param monrace_id 分離ユニークの種族ID
 * @param hp 分離ユニークの現在HP
 * @param maxhp 分離ユニークの最大HP (衰弱を含)
 */
bool MonraceList::can_select_separate(MonraceId monrace_id, const int hp, const int maxhp) const
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

    return std::all_of(found_separates.begin(), found_separates.end(), [this](const auto x) { return !this->get_monrace(x).is_dead_unique(); });
}

/*!
 * @brief 合体ユニークの分離先ユニークのいずれかをランダムで1つ選択する
 *
 * @param monrace_id 合体ユニークのモンスター種族ID
 * @return 分離先ユニークのモンスター種族ID。合体ユニークでない場合は monrace_id がそのまま返る
 */
MonraceId MonraceList::select_random_separated_unique_of(MonraceId monrace_id) const
{
    const auto it = unified_uniques.find(monrace_id);
    if (it != unified_uniques.end()) {
        return rand_choice(it->second);
    }

    return monrace_id;
}

bool MonraceList::order(MonraceId id1, MonraceId id2, bool is_detailed) const
{
    const auto &monrace1 = this->monraces.at(id1);
    const auto &monrace2 = this->monraces.at(id2);
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
        for (const auto &[monrace_id, monrace] : this->monraces) {
            if (monrace.is_valid()) {
                table.entry_item(monrace_id, 1);
            }
        }
    }

    return table.pick_one_at_random();
}

const MonraceDefinition &MonraceList::pick_monrace_at_random() const
{
    return this->monraces.at(this->pick_id_at_random());
}

int MonraceList::calc_defeat_count() const
{
    auto total = 0;
    for (const auto &[_, monrace] : this->monraces) {
        if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            if (monrace.is_dead_unique()) {
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

MonraceId MonraceList::select_figurine(int max_level) const
{
    while (true) {
        const auto monrace_id = this->pick_id_at_random();
        const auto &monrace = this->get_monrace(monrace_id);
        if (!monrace.is_suitable_for_figurine() || (monrace_id == MonraceId::TSUCHINOKO)) {
            continue;
        }

        const auto check = (max_level < monrace.level) ? (monrace.level - max_level) : 0;
        if ((monrace.rarity > 100) || (randint0(check) > 0)) {
            continue;
        }

        return monrace_id;
    }
}

/*!
 * @brief 現在フロアに存在している1種別辺りのモンスター数を全てリセットする
 * @todo そもそもcur_num はMonsterRaceInfo にいるべきではない、後で分離する
 */
void MonraceList::reset_current_numbers()
{
    for (auto &[_, monrace] : this->monraces) {
        monrace.reset_current_numbers();
    }
}

void MonraceList::reset_all_visuals()
{
    for (auto &[_, monrace] : this->monraces) {
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
    this->get_monrace(monrace_id).kill_unique();
    if (this->can_unify_separate(monrace_id)) {
        this->kill_unified_unique(monrace_id);
    }
}
