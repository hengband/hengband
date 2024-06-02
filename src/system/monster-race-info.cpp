#include "system/monster-race-info.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-indice-types.h"
#include "monster/horror-descriptions.h"
#include "util/probability-table.h"
#include "world/world.h"
#include <algorithm>

MonsterRaceInfo::MonsterRaceInfo()
    : idx(MonsterRaceId::PLAYER)
{
}

/*!
 * @brief 正当なモンスター (実際に存在するモンスター種族IDである)かどうかを調べる
 * @details モンスター種族IDが MonsterRaceDefinitions に実在するもの(MonsterRaceId::PLAYERは除く)であるかどうかの用途の他、
 * m_list 上の要素などの r_idx にMonsterRaceId::PLAYER を入れることで死亡扱いとして使われるのでその判定に使用する事もある
 * @return 正当なものであれば true、そうでなければ false
 * @todo 将来的に定義側のIDが廃止されたら有効フラグのフィールド変数を代わりに作る.
 */
bool MonsterRaceInfo::is_valid() const
{
    return this->idx != MonsterRaceId::PLAYER;
}

/*!
 * @brief エルドリッチホラーの形容詞種別を決める
 * @return エルドリッチホラーの形容詞
 */
const std::string &MonsterRaceInfo::decide_horror_message() const
{
    const int horror_desc_common_size = horror_desc_common.size();
    auto horror_num = randint0(horror_desc_common_size + horror_desc_evil.size());
    if (horror_num < horror_desc_common_size) {
        return horror_desc_common[horror_num];
    }

    if (this->kind_flags.has(MonsterKindType::EVIL)) {
        return horror_desc_evil[horror_num - horror_desc_common_size];
    }

    return horror_desc_neutral[horror_num - horror_desc_common_size];
}

/*!
 * @brief モンスターが生命体かどうかを返す
 * @return 生命体ならばtrue
 */
bool MonsterRaceInfo::has_living_flag() const
{
    return this->kind_flags.has_none_of({ MonsterKindType::DEMON, MonsterKindType::UNDEAD, MonsterKindType::NONLIVING });
}

/*!
 * @brief モンスターが自爆するか否か
 * @return 自爆するならtrue
 */
bool MonsterRaceInfo::is_explodable() const
{
    return std::any_of(std::begin(this->blows), std::end(this->blows),
        [](const auto &blow) { return blow.method == RaceBlowMethodType::EXPLODE; });
}

/*!
 * @brief モンスターを撃破した際の述語メッセージを返す
 * @return 撃破されたモンスターの述語
 */
std::string MonsterRaceInfo::get_died_message() const
{
    const auto is_explodable = this->is_explodable();
    if (this->has_living_flag()) {
        return is_explodable ? _("は爆発して死んだ。", " explodes and dies.") : _("は死んだ。", " dies.");
    }

    return is_explodable ? _("は爆発して粉々になった。", " explodes into tiny shreds.") : _("を倒した。", " is destroyed.");
}

std::optional<bool> MonsterRaceInfo::order_pet(const MonsterRaceInfo &other) const
{
    if (this->kind_flags.has(MonsterKindType::UNIQUE) && other.kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return true;
    }

    if (this->kind_flags.has_not(MonsterKindType::UNIQUE) && other.kind_flags.has(MonsterKindType::UNIQUE)) {
        return false;
    }

    if (this->level > other.level) {
        return true;
    }

    if (this->level < other.level) {
        return false;
    }

    return std::nullopt;
}

/*!
 * @brief ユニークモンスターの撃破状態を更新する
 * @todo 状態変更はモンスター「定義」ではないので将来的に別クラスへ分離する
 */
void MonsterRaceInfo::kill_unique()
{
    this->max_num = 0;
    this->r_pkills++;
    this->r_akills++;
    if (this->r_tkills < MAX_SHORT) {
        this->r_tkills++;
    }
}

std::string MonsterRaceInfo::get_pronoun_of_summoned_kin() const
{
    if (this->kind_flags.has(MonsterKindType::UNIQUE)) {
        return _("手下", "minions");
    }
    switch (this->idx) {
    case MonsterRaceId::LAFFEY_II:
        return _("ウサウサストライカー", "Bunbun Strikers");
    default:
        return _("仲間", "kin");
    }
}

/*!
 * @brief 進化先モンスターを返す. 進化しなければプレイヤー (無効値の意)
 * @return 進化先モンスター
 */
const MonsterRaceInfo &MonsterRaceInfo::get_next() const
{
    return MonraceList::get_instance()[this->next_r_idx];
}

/*!
 * @brief モンスター種族が賞金首の対象かどうかを調べる。日替わり賞金首は対象外。
 * @param unachieved_only true の場合未達成の賞金首のみを対象とする。false の場合達成未達成に関わらずすべての賞金首を対象とする。
 * @return モンスター種族が賞金首の対象ならば true、そうでなければ false
 */
bool MonsterRaceInfo::is_bounty(bool unachieved_only) const
{
    const auto end = std::end(w_ptr->bounties);
    const auto it = std::find_if(std::begin(w_ptr->bounties), end,
        [this](const auto &bounty) { return bounty.r_idx == this->idx; });
    if (it == end) {
        return false;
    }

    return !unachieved_only || !it->is_achieved;
}

const std::map<MonsterRaceId, std::set<MonsterRaceId>> MonraceList::unified_uniques = {
    { MonsterRaceId::BANORLUPART, { MonsterRaceId::BANOR, MonsterRaceId::LUPART } },
};

MonraceList MonraceList::instance{};

bool MonraceList::is_valid(MonsterRaceId monrace_id)
{
    return monrace_id != MonsterRaceId::PLAYER;
}

const std::map<MonsterRaceId, std::set<MonsterRaceId>> &MonraceList::get_unified_uniques()
{
    return unified_uniques;
}

MonraceList &MonraceList::get_instance()
{
    return instance;
}

/*!
 * @brief どのモンスター種族でもない事を意味する MonsterRaceId を返す
 * @details 実態は MonsterRaceId::PLAYER だが、この値は実際にプレイヤーとしての意味として使われる場合
 * （召喚主がプレイヤーの場合やマップ上の表示属性情報等）とどのモンスターでもない意味として使われる場合があるので、
 * 後者ではこれを使用することでコード上の意図をわかりやすくする。
 *
 * @return (どのモンスター種族でもないという意味での) MonsterRaceId::PLAYER を返す
 */
MonsterRaceId MonraceList::empty_id()
{
    return MonsterRaceId::PLAYER;
}

/*!
 * @bried モンスター定義を種族IDから直接得る
 * @param モンスター種族ID
 * @return モンスター定義への参照
 * @details モンスター実体からモンスター定義を得るためには使用しないこと
 */
MonsterRaceInfo &MonraceList::operator[](const MonsterRaceId r_idx)
{
    return monraces_info.at(r_idx);
}

/*!
 * @bried モンスター定義を種族IDから直接得る
 * @param モンスター種族ID
 * @return モンスター定義への参照
 * @details モンスター実体からモンスター定義を得るためには使用しないこと
 */
const MonsterRaceInfo &MonraceList::operator[](const MonsterRaceId r_idx) const
{
    return monraces_info.at(r_idx);
}

std::map<MonsterRaceId, MonsterRaceInfo>::iterator MonraceList::begin()
{
    return monraces_info.begin();
}

std::map<MonsterRaceId, MonsterRaceInfo>::const_iterator MonraceList::begin() const
{
    return monraces_info.begin();
}

std::map<MonsterRaceId, MonsterRaceInfo>::iterator MonraceList::end()
{
    return monraces_info.end();
}

std::map<MonsterRaceId, MonsterRaceInfo>::const_iterator MonraceList::end() const
{
    return monraces_info.end();
}

std::map<MonsterRaceId, MonsterRaceInfo>::reverse_iterator MonraceList::rbegin()
{
    return monraces_info.rbegin();
}

std::map<MonsterRaceId, MonsterRaceInfo>::const_reverse_iterator MonraceList::rbegin() const
{
    return monraces_info.rbegin();
}

std::map<MonsterRaceId, MonsterRaceInfo>::reverse_iterator MonraceList::rend()
{
    return monraces_info.rend();
}

std::map<MonsterRaceId, MonsterRaceInfo>::const_reverse_iterator MonraceList::rend() const
{
    return monraces_info.rend();
}

MonsterRaceInfo &MonraceList::get_monrace(MonsterRaceId monrace_id)
{
    return monraces_info.at(monrace_id);
}

const MonsterRaceInfo &MonraceList::get_monrace(MonsterRaceId monrace_id) const
{
    return monraces_info.at(monrace_id);
}

const std::vector<MonsterRaceId> &MonraceList::get_valid_monrace_ids() const
{
    static std::vector<MonsterRaceId> valid_monraces;
    if (!valid_monraces.empty()) {
        return valid_monraces;
    }

    std::transform(++monraces_info.begin(), monraces_info.end(), std::back_inserter(valid_monraces), [](auto &x) { return x.first; });
    return valid_monraces;
}

/*!
 * @brief 合体/分離ユニーク判定
 * @param r_idx 調査対象のモンスター種族ID
 * @return 合体/分離ユニークか否か
 * @details 合体/分離ユニークは、賞金首にもランダムクエスト討伐対象にもならない.
 */
bool MonraceList::can_unify_separate(const MonsterRaceId r_idx) const
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
void MonraceList::kill_unified_unique(const MonsterRaceId r_idx)
{
    const auto it_unique = unified_uniques.find(r_idx);
    if (it_unique != unified_uniques.end()) {
        (*this)[it_unique->first].kill_unique();
        for (const auto separate : it_unique->second) {
            (*this)[separate].kill_unique();
        }

        return;
    }

    for (const auto &[unified_unique, separates] : unified_uniques) {
        const auto it_separate = separates.find(r_idx);
        if (it_separate != separates.end()) {
            (*this)[*it_separate].kill_unique();
            (*this)[unified_unique].kill_unique();
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
bool MonraceList::is_selectable(const MonsterRaceId r_idx) const
{
    const auto it = unified_uniques.find(r_idx);
    if (it == unified_uniques.end()) {
        return true;
    }

    return std::all_of(it->second.begin(), it->second.end(), [&](const auto x) { return (*this)[x].cur_num == 0; });
}

/*!
 * @brief 合体ユニークが撃破済の状態でフロアから離脱した時に、各分離ユニークも撃破済状態へと変更する
 */
void MonraceList::defeat_separated_uniques()
{
    for (const auto &[unified_unique, separates] : unified_uniques) {
        if ((*this)[unified_unique].max_num > 0) {
            continue;
        }

        for (const auto separate : separates) {
            auto &monrace = (*this)[separate];
            if (monrace.max_num == 0) {
                continue;
            }

            monrace.kill_unique();
        }
    }
}

bool MonraceList::is_unified(const MonsterRaceId r_idx) const
{
    return unified_uniques.contains(r_idx);
}

/*!
 * @brief 合体ユニークの各分離ユニークが全員フロアにいるかをチェックする
 * @param r_idx 合体ユニークの種族ID
 * @return 全員が現在フロアに生成されているか
 */
bool MonraceList::exists_separates(const MonsterRaceId r_idx) const
{
    const auto &separates = unified_uniques.at(r_idx);
    return std::all_of(separates.begin(), separates.end(), [&](const auto x) { return (*this)[x].cur_num > 0; });
}

/*!
 * @brief 与えられたIDが分離ユニークのいずれかに一致するかをチェックする
 * @param r_idx 調査対象のモンスター種族ID
 */
bool MonraceList::is_separated(const MonsterRaceId r_idx) const
{
    if (unified_uniques.contains(r_idx)) {
        return false;
    }

    return std::any_of(unified_uniques.begin(), unified_uniques.end(), [&r_idx](const auto &x) { return x.second.contains(r_idx); });
}

/*!
 * @brief 合体ユニークが分離魔法を唱えられるかをチェックする
 * @param r_idx 分離ユニークの種族ID
 * @param hp 分離ユニークの現在HP
 * @param maxhp 分離ユニークの最大HP (衰弱を含)
 */
bool MonraceList::can_select_separate(const MonsterRaceId r_idx, const int hp, const int maxhp) const
{
    if (unified_uniques.contains(r_idx)) {
        return false;
    }

    const auto end = unified_uniques.end();
    const auto it = std::find_if(unified_uniques.begin(), end, [&r_idx](const auto &x) { return x.second.contains(r_idx); });
    if (it == end) {
        return false;
    }

    auto &found_separates = it->second;
    if (hp >= (maxhp / static_cast<int>(found_separates.size()))) {
        return false;
    }

    return std::all_of(found_separates.begin(), found_separates.end(), [&](const auto x) { return (*this)[x].max_num > 0; });
}

int MonraceList::calc_figurine_value(const MonsterRaceId r_idx) const
{
    const auto level = (*this)[r_idx].level;
    if (level < 20) {
        return level * 50;
    }

    if (level < 30) {
        return 1000 + (level - 20) * 150;
    }

    if (level < 40) {
        return 2500 + (level - 30) * 350;
    }

    if (level < 50) {
        return 6000 + (level - 40) * 800;
    }

    return 14000 + (level - 50) * 2000;
}

int MonraceList::calc_capture_value(const MonsterRaceId r_idx) const
{
    if (r_idx == MonsterRaceId::PLAYER) {
        return 1000;
    }

    return (*this)[r_idx].level * 50 + 1000;
}

bool MonraceList::order(MonsterRaceId id1, MonsterRaceId id2, bool is_detailed) const
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

bool MonraceList::order_level(MonsterRaceId id1, MonsterRaceId id2) const
{
    const auto &monrace1 = monraces_info[id1];
    const auto &monrace2 = monraces_info[id2];
    if (monrace1.level < monrace2.level) {
        return true;
    }

    if (monrace1.level > monrace2.level) {
        return false;
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
 * @brief (MonsterRaceId::PLAYERを除く)実在するすべてのモンスター種族IDから等確率で1つ選択する
 *
 * @return 選択したモンスター種族ID
 */
MonsterRaceId MonraceList::pick_id_at_random() const
{
    static ProbabilityTable<MonsterRaceId> table;
    if (table.empty()) {
        for (const auto &[monrace_id, monrace] : monraces_info) {
            if (monrace.is_valid()) {
                table.entry_item(monrace_id, 1);
            }
        }
    }

    return table.pick_one_at_random();
}

const MonsterRaceInfo &MonraceList::pick_monrace_at_random() const
{
    return monraces_info.at(this->pick_id_at_random());
}

void MonraceList::reset_all_visuals()
{
    for (auto &[_, monrace] : monraces_info) {
        monrace.symbol_config = monrace.symbol_definition;
    }
}
