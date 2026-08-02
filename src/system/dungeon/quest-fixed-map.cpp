#include "system/dungeon/quest-fixed-map.h"
#include "artifact/fixed-art-types.h"
#include "monster-race/race-kind-flags.h"
#include "monster-race/race-misc-flags.h"
#include "system/artifact/artifact-record.h"
#include "system/dungeon/quest-definition.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/monrace/monrace-definition.h"
#include "term/z-rand.h"
#include "util/enum-converter.h"

QuestFixedMapList QuestFixedMapList::instance{};

QuestFixedMapList &QuestFixedMapList::get_instance()
{
    return instance;
}

QuestFixedMap &QuestFixedMapList::emplace(QuestId id)
{
    return this->maps[id];
}

tl::optional<QuestFixedMap> QuestFixedMapList::find(QuestId id) const
{
    const auto it = this->maps.find(id);
    if (it == this->maps.end()) {
        return tl::nullopt;
    }

    return it->second;
}

void QuestFixedMapList::clear()
{
    this->maps.clear();
}

void QuestFixedMapList::set_base_legend(std::map<char, QuestLegendCell> legend)
{
    this->base_legend = std::move(legend);
}

const std::map<char, QuestLegendCell> &QuestFixedMapList::get_base_legend() const
{
    return this->base_legend;
}

void apply_quest_metadata(const QuestFixedMap &fixed_map, QuestType &quest)
{
    const auto &meta = fixed_map.metadata;
    if (!meta.present) {
        return;
    }

    // type は静的な基本値だが、受託時の resolve_quest_reward が「候補報酬が全て生成済み」の場合に
    // FIND_ARTIFACT を KILL_ALL へ格下げする (実行時の決定)。apply_quest_metadata は init/reset だけ
    // でなくフロア生成でも呼ばれるため、既に受託済みで具体的な type が確定している場合は上書きしない
    // (でないと格下げ後に FIND_ARTIFACT へ戻り、報酬アーティファクト不在の達成不能クエストになる)。
    if ((quest.status == QuestStatusType::UNTAKEN) || (quest.type == QuestKindType::NONE)) {
        quest.type = i2enum<QuestKindType>(meta.type);
    }
    quest.level = meta.level;
    quest.num_mon = meta.num_mon;
    quest.max_num = meta.max_num;
    quest.r_idx = i2enum<MonraceId>(meta.r_idx);
    quest.dungeon = i2enum<DungeonId>(meta.dungeon);
    quest.flags = meta.flags;

    // 旧 parse_qtw_QQ と同じく、UNIQUE のクエスト対象モンスターを QUESTOR にする (クエスト保護・
    // 最終ボス処理のため)。旧 init_dungeon_quests は birth で parse_fixed_map(QUEST_DEFINITION_LIST)
    // を INIT_ASSIGN で全クエスト分パースし、全クエストユニークに QUESTOR を立てていた。よって受託時に
    // 限定せず、init/reset_all で全クエストに設定する (でないとエリック砦等の未受託クエストのユニークが
    // 通常ダンジョンに出現し、そこで生成されるとクエスト内に出せず達成不能になる)。misc_flags の設定は
    // 冪等で、reset_all 直後の monrace リセットは kills/出現数のみ戻し misc_flags は消さないため残る。
    auto &monrace = quest.get_bounty();
    if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
        monrace.misc_flags.set(MonsterMiscType::QUESTOR);
    }
}

void resolve_quest_reward(const QuestFixedMap &fixed_map, QuestType &quest)
{
    const auto &meta = fixed_map.metadata;
    if (!meta.present) {
        return;
    }

    // 旧 parse_qtw_QQ 相当: 固定報酬アーティファクトを予約する。
    if (meta.reward_artifact != 0) {
        quest.set_reward(i2enum<FixedArtifactId>(meta.reward_artifact));
    }

    // 旧 parse_qtw_QR 相当: 未生成の候補アーティファクトから1つを報酬に選び set_reward で予約する。
    // 全て生成済みなら報酬なし + KILL_ALL に格下げする。旧 INIT_ASSIGN と同じく受託時のみ実行する
    // (assign_json_quest_metadata 経由)。受託は「新規キャラ生成時の全リセットより後・クエスト報酬
    // アーティファクトの配置(フロア生成)より前・ライフサイクル中に1回」であり、選んだ報酬の予約が
    // 消えず、生成済みアーティファクトを避けて確定できる。
    if (!fixed_map.reward_artifact_candidates.empty()) {
        const auto &artifact_records = ArtifactRecords::get_instance();
        auto reward_id = FixedArtifactId::NONE;
        auto count = 0;
        for (const auto candidate : fixed_map.reward_artifact_candidates) {
            const auto fa_id = i2enum<FixedArtifactId>(candidate);
            if ((fa_id == FixedArtifactId::NONE) || artifact_records.get_generated(fa_id)) {
                continue;
            }

            count++;
            if (one_in_(count)) {
                reward_id = fa_id;
            }
        }

        if (reward_id != FixedArtifactId::NONE) {
            quest.set_reward(reward_id);
        } else {
            quest.set_reward(FixedArtifactId::NONE);
            quest.type = QuestKindType::KILL_ALL;
        }
    }
}
