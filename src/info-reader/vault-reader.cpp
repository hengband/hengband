#include "info-reader/vault-reader.h"
#include "floor/floor-base-definitions.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "locale/character-encoding.h"
#include "locale/language-switcher.h"
#include "room/rooms-vault.h"
#include <algorithm>
#include <array>
#include <fmt/format.h>
#include <limits>
#include <utility>

VaultReader::VaultReader(const nlohmann::json &data)
    : data(data)
{
}

const std::optional<VaultReadError> &VaultReader::error() const
{
    return this->diagnostic;
}

int VaultReader::fail(int code, std::string_view path, std::string_view reason)
{
    const auto id = this->data.is_object() && this->data.contains("id") ? this->data["id"].dump() : "<unknown>";
    this->diagnostic = VaultReadError{ id, std::string(path), std::string(reason) };
    return code;
}

int VaultReader::read_integer(std::string_view key, int &value, int minimum, int maximum)
{
    if (const auto err = info_set_integer(this->data[key], value, true, Range(minimum, maximum))) {
        return this->fail(err, fmt::format("$.{}", key), fmt::format(_("{}以上{}以下の整数が必要です", "expected an integer in [{}, {}]"), minimum, maximum));
    }
    return PARSE_ERROR_NONE;
}

int VaultReader::read()
{
    this->diagnostic.reset();
    if (!this->data.is_object()) {
        return this->fail(PARSE_ERROR_INVALID_TYPE, "$", _("オブジェクトが必要です", "expected an object"));
    }
    constexpr auto keys = std::to_array<std::string_view>({ "id", "name", "type", "rating", "height", "width", "layout" });
    for (const auto key : keys) {
        if (!this->data.contains(key)) {
            return this->fail(PARSE_ERROR_TOO_FEW_ARGUMENTS, fmt::format("$.{}", key), _("必須項目がありません", "missing required field"));
        }
    }
    for (const auto &entry : this->data.items()) {
        if (std::find(keys.begin(), keys.end(), entry.key()) == keys.end()) {
            return this->fail(PARSE_ERROR_UNDEFINED_DIRECTIVE, fmt::format("$.{}", entry.key()), _("未知の項目です", "unknown field"));
        }
    }
    int id;
    if (const auto err = this->read_integer("id", id, 0, std::numeric_limits<short>::max())) {
        return err;
    }
    if (id <= error_idx) {
        return this->fail(PARSE_ERROR_NON_SEQUENTIAL_RECORDS, "$.id", _("IDは重複せず昇順に並べてください", "IDs must be unique and in increasing order"));
    }
    vault_type vault;
    vault.idx = static_cast<short>(id);
    if (!this->data["name"].is_string() || this->data["name"].get_ref<const std::string &>().empty()) {
        return this->fail(PARSE_ERROR_INVALID_TYPE, "$.name", _("空でない文字列が必要です", "expected a nonempty string"));
    }
    vault.name = utf8_to_local(this->data["name"].get_ref<const std::string &>());
    int type;
    if (const auto err = this->read_integer("type", type, 0, 255)) {
        return err;
    }
    if (type != 7 && type != 8 && type != 17) {
        return this->fail(PARSE_ERROR_INVALID_VALUE, "$.type", _("Vault種別は7、8、17のいずれかです", "vault type must be 7, 8 or 17"));
    }
    vault.typ = static_cast<uint8_t>(type);
    if (const auto err = this->read_integer("rating", vault.rat, 0, std::numeric_limits<int>::max())) {
        return err;
    }
    if (const auto err = this->read_integer("height", vault.hgt, 1, MAX_HGT)) {
        return err;
    }
    if (const auto err = this->read_integer("width", vault.wid, 1, MAX_WID)) {
        return err;
    }
    const auto &layout = this->data["layout"];
    if (!layout.is_array() || layout.size() != static_cast<size_t>(vault.hgt)) {
        return this->fail(PARSE_ERROR_INVALID_VALUE, "$.layout", _("行数がheightと一致しません", "row count must equal height"));
    }
    for (size_t i = 0; i < layout.size(); ++i) {
        const auto path = fmt::format("$.layout[{}]", i);
        if (!layout[i].is_string()) {
            return this->fail(PARSE_ERROR_INVALID_TYPE, path, _("文字列が必要です", "expected a string"));
        }
        const auto &row = layout[i].get_ref<const std::string &>();
        if (row.size() != static_cast<size_t>(vault.wid)) {
            return this->fail(PARSE_ERROR_INVALID_VALUE, path, _("行のバイト数がwidthと一致しません", "row byte length must equal width"));
        }
        if (std::any_of(row.begin(), row.end(), [](unsigned char c) { return c < 0x20 || c > 0x7e; })) {
            return this->fail(PARSE_ERROR_INVALID_VALUE, path, _("配置図には印字可能なASCII文字を指定してください", "layout must contain printable ASCII characters"));
        }
        vault.text += row;
    }
    // Publish only a fully validated record; failed reads leave global state intact.
    if (static_cast<size_t>(id) >= vaults_info.size()) {
        vaults_info.resize(id + 1);
    }
    vaults_info[id] = std::move(vault);
    error_idx = id;
    return PARSE_ERROR_NONE;
}
