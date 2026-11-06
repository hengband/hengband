#pragma once

#include "util/abstract-map-wrapper.h"
#include "util/sha256.h"

enum class DefinitionHashDataType {
    ARTIFACTS, /*!< 固定アーティファクト情報のハッシュ値 */
    BASEITEMS, /*!< ベースアイテム情報のハッシュ値 */
    CLASS_MAGICS, /*!< プレイヤー職業魔法情報のハッシュ値 */
    CLASS_SKILLS, /*!< プレイヤー職業技能情報のハッシュ値 */
    DUNGEONS, /*!< ダンジョン情報のハッシュ値 */
    EGOS, /*!< アイテムエゴ情報のハッシュ値 */
    MONRACES, /*!< モンスター種族情報のハッシュ値 */
    MONSTER_MESSAGES, /*!< モンスターメッセージのハッシュ値 */
    SPELLS, /*!< 呪文情報のハッシュ値 */
    TERRAINS, /*!< 地形情報のハッシュ値 */
    VAULTS, /*!< Vault情報のハッシュ値 */
    MAX, /*!< ハッシュ値の種類の数 */
};

class DefinitionHashData : public util::AbstractMapWrapper<DefinitionHashDataType, util::SHA256::Digest> {
public:
    DefinitionHashData(const DefinitionHashData &) = delete;
    DefinitionHashData(DefinitionHashData &&) = delete;
    DefinitionHashData &operator=(const DefinitionHashData &) = delete;
    DefinitionHashData &operator=(DefinitionHashData &&) = delete;
    ~DefinitionHashData() = default;

    static DefinitionHashData &get_instance();

    util::SHA256::Digest get_digest(DefinitionHashDataType type) const;
    void set_digest(DefinitionHashDataType type, const util::SHA256::Digest &digest);

    std::string get_check_sum() const;

private:
    DefinitionHashData();
    static DefinitionHashData instance;

    std::map<DefinitionHashDataType, util::SHA256::Digest> hash_data{};

    std::map<DefinitionHashDataType, util::SHA256::Digest> &get_inner_container() override
    {
        return this->hash_data;
    }
};
