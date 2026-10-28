#include "info-reader/definition-hash-data.h"
#include "util/enum-range.h"

DefinitionHashData DefinitionHashData::instance{};

DefinitionHashData::DefinitionHashData()
{
    for (const auto type : EnumRange<DefinitionHashDataType>(DefinitionHashDataType::ARTIFACTS, DefinitionHashDataType::MAX)) {
        this->hash_data[type] = {};
    }
}

DefinitionHashData &DefinitionHashData::get_instance()
{
    return instance;
}

util::SHA256::Digest DefinitionHashData::get_digest(DefinitionHashDataType type) const
{
    return this->hash_data.at(type);
}

void DefinitionHashData::set_digest(DefinitionHashDataType type, const util::SHA256::Digest &digest)
{
    this->hash_data.at(type) = digest;
}

std::string DefinitionHashData::get_check_sum() const
{
    util::SHA256 sha256;
    for (const auto &[_, hash_datum] : this->hash_data) {
        sha256.update(hash_datum.data(), hash_datum.size());
    }

    return util::to_string(sha256.digest());
}
