#include "system/monster-race-info.h"
#include "monster-race/race-indice-types.h"
#include "monster-race/race-resistance-mask.h"
#include "monster/horror-descriptions.h"
#include "system/redrawing-flags-updater.h"
#include "tracking/lore-tracker.h"
#include "util/probability-table.h"
#include "world/world.h"
#include <algorithm>
#ifndef JP
#include "locale/english.h"
#endif

namespace {
template <class T>
static int count_lore_mflag_group(const EnumClassFlagGroup<T> &flags, const EnumClassFlagGroup<T> &r_flags)
{
    auto result_flags = flags;
    auto num = result_flags.reset(r_flags).count();
    return num;
}
}

std::map<MonsterRaceId, MonsterRaceInfo> monraces_info;

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
 * @brief モンスターのシンボル文字が指定された文字列に含まれるかどうかを返す
 * @param candidate_chars シンボル文字の集合の文字列。"pht" のように複数の文字を指定可能。
 * @return モンスターのシンボル文字が candidate_chars に含まれるならばtrue
 * @note ASCIIのみ対応。マルチバイト文字が指定された場合の動作は未定義。
 */
bool MonsterRaceInfo::symbol_char_is_any_of(std::string_view candidate_chars) const
{
    return candidate_chars.find(this->symbol_definition.character) != std::string_view::npos;
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
    return MonraceList::get_instance().get_monrace(this->next_r_idx);
}

/*!
 * @brief モンスター種族が賞金首の対象かどうかを調べる。日替わり賞金首は対象外。
 * @param unachieved_only true の場合未達成の賞金首のみを対象とする。false の場合達成未達成に関わらずすべての賞金首を対象とする。
 * @return モンスター種族が賞金首の対象ならば true、そうでなければ false
 */
bool MonsterRaceInfo::is_bounty(bool unachieved_only) const
{
    const auto &world = AngbandWorld::get_instance();
    const auto end = std::end(world.bounties);
    const auto it = std::find_if(std::begin(world.bounties), end,
        [this](const auto &bounty) { return bounty.r_idx == this->idx; });
    if (it == end) {
        return false;
    }

    return !unachieved_only || !it->is_achieved;
}

/*!
 * @brief モンスター種族の総合的な強さを計算する。
 * @details 現在はモンスター闘技場でのモンスターの強さの総合的な評価にのみ使用されている。
 * @return 計算した結果のモンスター種族の総合的な強さの値を返す。
 */
int MonsterRaceInfo::calc_power() const
{
    auto power = 0;
    const auto num_resistances = EnumClassFlagGroup<MonsterResistanceType>(this->resistance_flags & RFR_EFF_IMMUNE_ELEMENT_MASK).count();
    if (this->misc_flags.has(MonsterMiscType::FORCE_MAXHP)) {
        power = this->hit_dice.maxroll() * 2;
    } else {
        power = this->hit_dice.floored_expected_value_multiplied_by(2);
    }

    power = power * (100 + this->level) / 100;
    if (this->speed > STANDARD_SPEED) {
        power = power * (this->speed * 2 - 110) / 100;
    }

    if (this->speed < STANDARD_SPEED) {
        power = power * (this->speed - 20) / 100;
    }

    if (num_resistances > 2) {
        power = power * (num_resistances * 2 + 5) / 10;
    } else if (this->ability_flags.has(MonsterAbilityType::INVULNER)) {
        power = power * 4 / 3;
    } else if (this->ability_flags.has(MonsterAbilityType::HEAL)) {
        power = power * 4 / 3;
    } else if (this->ability_flags.has(MonsterAbilityType::DRAIN_MANA)) {
        power = power * 11 / 10;
    }

    if (this->behavior_flags.has(MonsterBehaviorType::RAND_MOVE_25)) {
        power = power * 9 / 10;
    }

    if (this->behavior_flags.has(MonsterBehaviorType::RAND_MOVE_50)) {
        power = power * 9 / 10;
    }

    if (this->resistance_flags.has(MonsterResistanceType::RESIST_ALL)) {
        power *= 100000;
    }

    if (this->arena_ratio) {
        power = power * this->arena_ratio / 100;
    }

    return power;
}

int MonsterRaceInfo::calc_figurine_value() const
{
    const auto figurine_level = this->level;
    if (figurine_level < 20) {
        return figurine_level * 50;
    }

    if (figurine_level < 30) {
        return 1000 + (figurine_level - 20) * 150;
    }

    if (figurine_level < 40) {
        return 2500 + (figurine_level - 30) * 350;
    }

    if (figurine_level < 50) {
        return 6000 + (figurine_level - 40) * 800;
    }

    return 14000 + (figurine_level - 50) * 2000;
}

int MonsterRaceInfo::calc_capture_value() const
{
    if (!this->is_valid()) {
        return 1000;
    }

    return this->level * 50 + 1000;
}

/*!
 * @brief エルドリッチホラー持ちのモンスターを見た時の反応メッセージを作って返す
 * @param description モンスター表記
 * @return 反応メッセージ
 * @details 実際に見るとは限らない (悪夢モードで宿に泊まった時など)
 */
std::string MonsterRaceInfo::build_eldritch_horror_message(std::string_view description) const
{
    const auto &horror_message = this->decide_horror_message();
    constexpr auto fmt = _("%s%sの顔を見てしまった！", "You behold the %s visage of %s!");
    return format(fmt, horror_message.data(), description.data());
}

bool MonsterRaceInfo::has_reinforce() const
{
    const auto end = this->reinforces.end();
    const auto it = std::find_if(this->reinforces.begin(), end,
        [](const auto &reinforce) { return reinforce.is_valid(); });
    return it != end;
}

std::optional<std::string> MonsterRaceInfo::probe_lore()
{
    auto n = false;
    if (this->r_wake != MAX_UCHAR) {
        n = true;
    }

    if (this->r_ignore != MAX_UCHAR) {
        n = true;
    }

    this->r_wake = MAX_UCHAR;
    this->r_ignore = MAX_UCHAR;
    for (auto i = 0; i < 4; i++) {
        const auto &blow = this->blows[i];
        if ((blow.effect != RaceBlowEffectType::NONE) || (blow.method != RaceBlowMethodType::NONE)) {
            if (this->r_blows[i] != MAX_UCHAR) {
                n = true;
            }

            this->r_blows[i] = MAX_UCHAR;
        }
    }

    using Mdt = MonsterDropType;
    auto num_drops = (this->drop_flags.has(Mdt::DROP_4D2) ? 8 : 0);
    num_drops += (this->drop_flags.has(Mdt::DROP_3D2) ? 6 : 0);
    num_drops += (this->drop_flags.has(Mdt::DROP_2D2) ? 4 : 0);
    num_drops += (this->drop_flags.has(Mdt::DROP_1D2) ? 2 : 0);
    num_drops += (this->drop_flags.has(Mdt::DROP_90) ? 1 : 0);
    num_drops += (this->drop_flags.has(Mdt::DROP_60) ? 1 : 0);
    if (this->drop_flags.has_not(Mdt::ONLY_GOLD)) {
        if (this->r_drop_item != num_drops) {
            n = true;
        }

        this->r_drop_item = num_drops;
    }

    if (this->drop_flags.has_not(Mdt::ONLY_ITEM)) {
        if (this->r_drop_gold != num_drops) {
            n = true;
        }

        this->r_drop_gold = num_drops;
    }

    if (this->r_cast_spell != MAX_UCHAR) {
        n = true;
    }

    this->r_cast_spell = MAX_UCHAR;
    n |= count_lore_mflag_group(this->resistance_flags, this->r_resistance_flags) > 0;
    n |= count_lore_mflag_group(this->ability_flags, this->r_ability_flags) > 0;
    n |= count_lore_mflag_group(this->behavior_flags, this->r_behavior_flags) > 0;
    n |= count_lore_mflag_group(this->drop_flags, this->r_drop_flags) > 0;
    n |= count_lore_mflag_group(this->feature_flags, this->r_feature_flags) > 0;
    n |= count_lore_mflag_group(this->special_flags, this->r_special_flags) > 0;
    n |= count_lore_mflag_group(this->misc_flags, this->r_misc_flags) > 0;

    this->r_resistance_flags = this->resistance_flags;
    this->r_ability_flags = this->ability_flags;
    this->r_behavior_flags = this->behavior_flags;
    this->r_drop_flags = this->drop_flags;
    this->r_feature_flags = this->feature_flags;
    this->r_special_flags = this->special_flags;
    this->r_misc_flags = this->misc_flags;
    if (!this->r_can_evolve) {
        n = true;
    }

    this->r_can_evolve = true;
    if (n == 0) {
        return std::nullopt;
    }

#ifdef JP
    return format("%sについてさらに詳しくなった気がする。", this->name.data());
#else
    const auto nm = pluralize(this->name);
    return format("You now know more about %s.", nm.data());
#endif
}

void MonsterRaceInfo::make_lore_treasure(int num_item, int num_gold)
{
    if (this->r_drop_item < num_item) {
        this->r_drop_item = num_item;
    }

    if (this->r_drop_gold < num_gold) {
        this->r_drop_gold = num_gold;
    }

    if (this->drop_flags.has(MonsterDropType::DROP_GOOD)) {
        this->r_drop_flags.set(MonsterDropType::DROP_GOOD);
    }

    if (this->drop_flags.has(MonsterDropType::DROP_GREAT)) {
        this->r_drop_flags.set(MonsterDropType::DROP_GREAT);
    }
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

Reinforce::Reinforce(MonsterRaceId monrace_id, Dice dice)
    : monrace_id(monrace_id)
    , dice(dice)
{
}

MonsterRaceId Reinforce::get_monrace_id() const
{
    return this->monrace_id;
}

bool Reinforce::is_valid() const
{
    return MonraceList::is_valid(this->monrace_id) && this->dice.is_valid();
}

const MonsterRaceInfo &Reinforce::get_monrace() const
{
    return MonraceList::get_instance().get_monrace(this->monrace_id);
}

std::string Reinforce::get_dice_as_string() const
{
    return this->dice.to_string();
}

int Reinforce::roll_dice() const
{
    return this->dice.roll();
}

int Reinforce::roll_max_dice() const
{
    return this->dice.maxroll();
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

std::map<MonsterRaceId, MonsterRaceInfo>::iterator MonraceList::begin()
{
    return monraces_info.begin();
}

std::map<MonsterRaceId, MonsterRaceInfo>::const_iterator MonraceList::begin() const
{
    return monraces_info.cbegin();
}

std::map<MonsterRaceId, MonsterRaceInfo>::iterator MonraceList::end()
{
    return monraces_info.end();
}

std::map<MonsterRaceId, MonsterRaceInfo>::const_iterator MonraceList::end() const
{
    return monraces_info.cend();
}

std::map<MonsterRaceId, MonsterRaceInfo>::reverse_iterator MonraceList::rbegin()
{
    return monraces_info.rbegin();
}

std::map<MonsterRaceId, MonsterRaceInfo>::const_reverse_iterator MonraceList::rbegin() const
{
    return monraces_info.crbegin();
}

std::map<MonsterRaceId, MonsterRaceInfo>::reverse_iterator MonraceList::rend()
{
    return monraces_info.rend();
}

std::map<MonsterRaceId, MonsterRaceInfo>::const_reverse_iterator MonraceList::rend() const
{
    return monraces_info.crend();
}

size_t MonraceList::size() const
{
    return monraces_info.size();
}

/*!
 * @brief モンスター定義を種族IDから直接得る (非const版)
 * @param monrace_id モンスター種族ID
 * @return モンスター定義への参照
 * @details モンスター実体からモンスター定義を得るためには使用しないこと
 */
MonsterRaceInfo &MonraceList::get_monrace(MonsterRaceId monrace_id)
{
    return monraces_info.at(monrace_id);
}

/*!
 * @brief モンスター定義を種族IDから直接得る (const版)
 * @param monrace_id モンスター種族ID
 * @return モンスター定義への参照
 * @details モンスター実体からモンスター定義を得るためには使用しないこと
 */
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
bool MonraceList::is_selectable(const MonsterRaceId r_idx) const
{
    const auto it = unified_uniques.find(r_idx);
    if (it == unified_uniques.end()) {
        return true;
    }

    return std::all_of(it->second.begin(), it->second.end(), [this](const auto x) { return this->get_monrace(x).cur_num == 0; });
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
    return std::all_of(separates.begin(), separates.end(), [this](const auto x) { return this->get_monrace(x).cur_num > 0; });
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
 * @param monrace_id 分離ユニークの種族ID
 * @param hp 分離ユニークの現在HP
 * @param maxhp 分離ユニークの最大HP (衰弱を含)
 */
bool MonraceList::can_select_separate(const MonsterRaceId monrace_id, const int hp, const int maxhp) const
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

std::optional<std::string> MonraceList::probe_lore(MonsterRaceId monrace_id)
{
    if (LoreTracker::get_instance().is_tracking(monrace_id)) {
        RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::MONSTER_LORE);
    }

    return this->get_monrace(monrace_id).probe_lore();
}
