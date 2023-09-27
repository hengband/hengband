#include "system/monster-race-info.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-indice-types.h"
#include "monster/horror-descriptions.h"
#include <algorithm>

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

const std::map<MonsterRaceId, std::set<MonsterRaceId>> MonraceList::unified_uniques = {
    { MonsterRaceId::BANORLUPART, { MonsterRaceId::BANOR, MonsterRaceId::LUPART } },
};

MonraceList MonraceList::instance{};

MonraceList &MonraceList::get_instance()
{
    return instance;
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
    auto kill_monrace = [](MonsterRaceId r_idx) {
        auto &monrace = monraces_info[r_idx];
        monrace.max_num = 0;
        monrace.r_pkills++;
        monrace.r_akills++;
        if (monrace.r_tkills < MAX_SHORT) {
            monrace.r_tkills++;
        }
    };

    const auto it_unique = unified_uniques.find(r_idx);
    if (it_unique != unified_uniques.end()) {
        kill_monrace(it_unique->first);
        for (const auto separate : it_unique->second) {
            kill_monrace(separate);
        }

        return;
    }

    for (const auto &[unified_unique, separates] : unified_uniques) {
        const auto it_separate = separates.find(r_idx);
        if (it_separate != separates.end()) {
            kill_monrace(*it_separate);
            kill_monrace(unified_unique);
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

    return std::all_of(it->second.begin(), it->second.end(), [](const auto x) { return monraces_info[x].cur_num == 0; });
}
