#pragma once
/*!
 * @file quest-fixed-map.h
 * @brief 固定クエストのレイアウト情報 (JSONCから読み込む地形凡例・マップ・説明文・開始位置)
 *
 * QuestType はクエストの実行時状態とメタデータを保持するが、フロアを生成するための
 * 地形レイアウト(legend/map)・段階別説明文・開始位置は保持しない。これらは従来
 * 各クエストの .txt を毎回パースして得ていたものを、起動時に一度だけ JSONC から
 * 読み込んで保持するための構造体群である。
 */

#include "info-reader/general-parser.h" // dungeon_grid
#include <cstdint>
#include <map>
#include <string>
#include <tl/optional.hpp>
#include <vector>

class QuestType;
enum class QuestId : short;
enum class QuestStatusType : short;
enum class QuestKindType : short;

/*!
 * @brief クエストの静的メタデータ (旧 Q:Q)。QuestType::reset() で消えないよう別に保持し、
 * load 時および reset 後に QuestType へ再適用する。
 */
struct QuestMetadata {
    bool present = false; //!< JSONC から読み込まれたか
    int type = 0; //!< QuestKindType
    int level = 0;
    int num_mon = 0;
    int max_num = 0;
    int r_idx = 0; //!< MonraceId
    int dungeon = 0; //!< DungeonId
    uint32_t flags = 0;
    int reward_artifact = 0; //!< FixedArtifactId (0 = なし)
};

/*!
 * @brief 地形凡例の1エントリ (記号 -> グリッド生成テンプレート)
 *
 * dungeon_grid をそのままテンプレートとして流用する。ただしクエスト報酬を指す
 * object/artifact (旧 F: の "!") は生成時に解決するため、フラグで遅延解決を示す。
 */
struct QuestLegendCell {
    dungeon_grid grid{}; //!< feature/monster/object/ego/artifact/trap/cave_info/special/random
    bool object_is_quest_reward = false; //!< object をクエスト報酬アイテムとして生成時に解決する (旧 "!")
    bool artifact_is_quest_reward = false; //!< artifact をクエスト報酬アーティファクトとして解決する (旧 "!")
};

/*!
 * @brief 条件付き説明文ブロック (旧 ?:[...] over $QUESTnn / $QUEST_TYPEnn)
 *
 * status_at_most / status_equals はいずれか一方のみを持つ。quest_type を併せ持つ場合は
 * 「そのクエスト種別のときのみ表示」を追加で要求する (旧 [AND [..$QUESTnn..] [EQU $QUEST_TYPEnn t]])。
 */
struct QuestDescriptionBlock {
    tl::optional<QuestStatusType> status_at_most; //!< [LEQ $QUESTnn s]: status <= s で表示
    tl::optional<QuestStatusType> status_equals; //!< [EQU $QUESTnn s]: status == s で表示
    tl::optional<QuestKindType> quest_type; //!< [EQU $QUEST_TYPEnn t]: さらにこの種別を要求
    std::vector<std::string> lines_ja; //!< 日本語の表示行 (1要素=画面1行)
    std::vector<std::string> lines_en; //!< 英語の表示行
};

/*!
 * @brief プレイヤー開始位置。退出クエスト依存で分岐する場合は複数持つ (旧 $LEAVING_QUEST)。
 */
struct QuestStartPosition {
    tl::optional<int> leaving_quest; //!< $LEAVING_QUEST の一致条件。未設定なら既定(フォールバック)
    int y = 0;
    int x = 0;
};

/*!
 * @brief 1つの固定クエストのレイアウト情報一式
 */
struct QuestFixedMap {
    QuestMetadata metadata; //!< 静的メタデータ (旧 Q:Q)
    std::map<char, QuestLegendCell> legend; //!< 記号 -> グリッドテンプレート (共通ベース凡例に上書き)
    std::vector<std::vector<std::string>> maps; //!< 1個=固定、複数=ランダムバリアント(旧 $RANDOMn) から一様選択
    std::vector<QuestDescriptionBlock> descriptions; //!< 段階別説明文
    std::vector<QuestStartPosition> starts; //!< 開始位置 (1個=無条件、複数=退出クエスト分岐)
    std::vector<int> reward_artifact_candidates; //!< 旧 Q:R: 生成時に未生成のものからランダムに1つ報酬化する候補

    bool has_map() const
    {
        return !this->maps.empty();
    }
};

/*!
 * @brief 固定クエストのレイアウト情報ストア (QuestId -> QuestFixedMap)。起動時に構築。
 */
class QuestFixedMapList {
public:
    QuestFixedMapList(const QuestFixedMapList &) = delete;
    QuestFixedMapList(QuestFixedMapList &&) = delete;
    QuestFixedMapList &operator=(const QuestFixedMapList &) = delete;
    QuestFixedMapList &operator=(QuestFixedMapList &&) = delete;

    static QuestFixedMapList &get_instance();

    QuestFixedMap &emplace(QuestId id); //!< 空エントリを作成/取得 (ローダが読み込み先として使う)
    tl::optional<QuestFixedMap> find(QuestId id) const; //!< 無ければ nullopt
    void clear();

    //!< 全クエスト共通のベース凡例 (旧 QuestPreferences.txt)。各クエスト legend より前に letter[] へ適用する。
    void set_base_legend(std::map<char, QuestLegendCell> legend);
    const std::map<char, QuestLegendCell> &get_base_legend() const;

private:
    QuestFixedMapList() = default;
    static QuestFixedMapList instance;
    std::map<QuestId, QuestFixedMap> maps;
    std::map<char, QuestLegendCell> base_legend;
};

/*!
 * @brief QuestFixedMap の静的メタデータ (type/level/r_idx 等 + QUESTOR) を QuestType へ適用する
 * @details load 時 / reset 後 / フロア生成時に呼ばれる冪等な処理。UNIQUE のクエスト対象は QUESTOR に
 * する (旧 birth の全クエスト INIT_ASSIGN 相当。全クエストで立てないと未受託クエストのユニークが通常
 * 出現し達成不能化する)。報酬 (アーティファクト予約を伴う) は含めない。予約が reset 直後の
 * ArtifactRecords リセットで消えないよう、報酬確定は受託時のみの resolve_quest_reward() で行う。
 */
void apply_quest_metadata(const QuestFixedMap &fixed_map, QuestType &quest);

/*!
 * @brief 受託時 (旧 INIT_ASSIGN) の一度きりの処理: 報酬アーティファクトの確定・予約
 * @details 旧 parse_qtw_QQ (固定報酬) と parse_qtw_QR (候補報酬) 相当。受託時のみ
 * (assign_json_quest_metadata 経由で) 呼ぶこと。初期化・reset・フロア生成では呼ばない
 * (報酬予約タイミングが reset 直後の ArtifactRecords リセットとずれるため)。
 */
void resolve_quest_reward(const QuestFixedMap &fixed_map, QuestType &quest);
