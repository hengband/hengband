#include "definitions-reader/spell-reader.h"
#include "definitions-reader/json-reader-util.h"
#include "definitions-reader/parse-error-types.h"
#include "player-info/class-info.h"
#include "system/spell-info-list.h"
#include "system/system-variables.h"
#include "view/display-messages.h"
#include <limits>
#include <map>
#include <nlohmann/json.hpp>
#include <string_view>
#include <utility>
#include <vector>

namespace {
//! 基準の消費MP・初回経験値倍率の上限。職業魔法 (ClassMagicDefinitions) の上限と同じ値にする。
constexpr auto MAX_SPELL_BASE_VALUE = std::numeric_limits<short>::max() / 16;
}

SpellReader::SpellReader(const nlohmann::json &realm_data, SpellInfoList &spell_info_list, Session &session)
    : realm_data(realm_data)
    , spell_info_list(spell_info_list)
    , session(session)
{
}

/*!
 * @brief 呪文情報(SpellDefinitions)のパース関数
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> SpellReader::read() const
{

    const auto realm_result = this->read_realm();
    if (!realm_result) {
        msg_print(_("領域名読込失敗。", "Failed to load realm name."));
        return tl::make_unexpected(realm_result.error());
    }

    const auto realm_id = *realm_result;
    std::vector<SpellInfo> spells;
    if (const auto result = this->set_book_data(spells); !result) {
        msg_print(_("呪文詳細読込失敗。", "Failed to load spell data."));
        return result;
    }

    // レコード全体の検証が済むまでは、呪文表もセッション状態も変更しない。
    const auto previous = this->session.realms.find(realm_id);
    auto staged = previous == this->session.realms.end() ? Session::RealmSpells{} : previous->second;
    const auto realm_name = get_json_value(this->realm_data, "name").get<std::string>();
    for (const auto &info : spells) {
        if (!staged.ids.insert(info.idx).second) {
            msg_print(_("呪文ID重複。領域: '{}'。呪文ID: '{}'。", "Duplicate spell ID. Realm: '{}'. Spell ID: '{}'."), realm_name, info.idx);
            return tl::make_unexpected(ParseErrorType::NON_SEQUENTIAL_RECORDS);
        }
        if (!staged.tags.insert(info.tag).second) {
            msg_print(_("呪文タグ重複。領域: '{}'。呪文タグ: '{}'。", "Duplicate spell tag. Realm: '{}'. Spell tag: '{}'."), realm_name, info.tag);
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
    }

    this->session.realms[realm_id] = std::move(staged);
    for (auto &info : spells) {
        this->spell_info_list.set_spell_info(realm_id, info.idx, std::move(info));
    }

    return {};
}

/*!
 * @brief JSON Objectから呪文領域IDを取得する
 * @return 呪文領域、失敗時はエラーコード
 */
tl::expected<RealmType, ParseErrorType> SpellReader::read_realm() const
{
    if (this->realm_data.is_null()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    const auto &realmname_obj = get_json_value(this->realm_data, "name");
    const auto realm = info_read_token(realmname_obj, realms_list);
    if (!realm) {
        return tl::make_unexpected(realm.error());
    }
    return *realm;
}

/*!
 * @brief JSON Objectから各呪文の詳細を取得する
 * @param spell_data 情報の格納されたJSON Object
 * @param spells 解析結果の一時保存先
 * @return 成功時は空の値、失敗時はエラーコード
 */
tl::expected<void, ParseErrorType> SpellReader::set_spell_data(const nlohmann::json &spell_data, std::vector<SpellInfo> &spells) const
{
    if (spell_data.is_null()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    int spell_id = 0;
    if (const auto result = info_set_integer(get_json_value(spell_data, "spell_id"), spell_id, true, Range(0, 31)); !result) {
        msg_print(_("呪文ID読込失敗。", "Failed to load spell ID."));
        return result;
    }
    auto info = SpellInfo();
    info.idx = static_cast<short>(spell_id);

    const auto &tag_obj = get_json_value(spell_data, "spell_tag");
    if (tag_obj.is_null()) {
        msg_print(_("呪文タグ読込失敗。", "Failed to load spell tag."));
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }
    if (!tag_obj.is_string()) {
        msg_print(_("呪文タグ読込失敗。", "Failed to load spell tag."));
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }
    info.tag = tag_obj.get<std::string>();

    if (const auto result = info_set_string(get_json_value(spell_data, "name"), info.name, true); !result) {
        msg_print(_("呪文名読込失敗。", "Failed to load spell name."));
        return result;
    }
    if (const auto result = info_set_string(get_json_value(spell_data, "description"), info.description, true); !result) {
        msg_print(_("呪文説明読込失敗。", "Failed to load spell description."));
        return result;
    }
    PLAYER_LEVEL learn_level{};
    if (const auto result = info_set_integer(get_json_value(spell_data, "learn_level"), learn_level, true, Range(1, PY_MAX_LEVEL)); !result) {
        msg_print(_("呪文の基準習得レベル読込失敗。", "Failed to load spell base learn_level."));
        return result;
    }
    info.base.set_learn_level(learn_level);
    if (const auto result = info_set_integer(get_json_value(spell_data, "mana_cost"), info.base.smana, true, Range(1, MAX_SPELL_BASE_VALUE)); !result) {
        msg_print(_("呪文の基準コスト読込失敗。", "Failed to load spell base mana_cost."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(spell_data, "difficulty"), info.base.sfail, true, Range(0, 100)); !result) {
        msg_print(_("呪文の基準難易度読込失敗。", "Failed to load spell base difficulty."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(spell_data, "first_cast_exp_rate"), info.base.sexp, true, Range(0, MAX_SPELL_BASE_VALUE)); !result) {
        msg_print(_("呪文の基準詠唱ボーナスEXP読込失敗。", "Failed to load spell base first_cast_exp_rate."));
        return result;
    }
    spells.push_back(std::move(info));

    return {};
}

/*!
 * @brief JSON Objectから各呪文の詳細を呪文書単位で取得する
 * @param spells 解析結果の一時保存先
 * @return 成功時は空の値、失敗時はエラーコード
 * @details 魔法書は領域の 1 冊目から順に書く。魔法がどの魔法書に載るかは呪文ID で決まるので
 * (呪文ID を SPELLS_IN_BOOK で割った商)、書かれた位置と食い違う魔法や、多すぎる魔法書は INVALID_VALUE にする。
 */
tl::expected<void, ParseErrorType> SpellReader::set_book_data(std::vector<SpellInfo> &spells) const
{
    const auto &book_obj = get_json_value(this->realm_data, "books");
    auto book_index = 0;
    return info_read_object_array(book_obj, [this, &spells, &book_index](const nlohmann::json &book) -> tl::expected<void, ParseErrorType> {
        if (book_index >= MAX_BOOKS_IN_REALM) {
            msg_print(_("魔法書が {} 冊より多く書かれています。", "More than {} books are listed."), MAX_BOOKS_IN_REALM);
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }

        const auto first_in_book = spells.size();
        const auto &spells_obj = get_json_value(book, "spells");
        const auto result = info_read_object_array(spells_obj, [this, &spells](const nlohmann::json &spell) -> tl::expected<void, ParseErrorType> {
            return this->set_spell_data(spell, spells);
        });
        if (!result) {
            return result;
        }

        for (auto i = first_in_book; i < spells.size(); i++) {
            if (spells[i].idx / SPELLS_IN_BOOK != book_index) {
                msg_print(_("呪文ID '{}' は {} 冊目の魔法書の魔法ではありません。", "Spell ID '{}' does not belong to book {}."), spells[i].idx, book_index + 1);
                return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
            }
        }
        book_index++;
        return {};
    });
}

/*!
 * @brief 全領域の魔法が抜けなく定義されたかを確かめる
 * @param session 呪文定義の読み込みで登録した呪文ID を持つセッション
 * @return 全領域の全魔法が定義されていれば空の値。抜けがあれば TOO_FEW_ARGUMENTS
 * @details 定義されなかった枠は、全職業で習得できない魔法として黙って扱われてしまうので、読み込みの最後に確かめる。
 */
tl::expected<void, ParseErrorType> SpellReader::validate_all_defined(const Session &session)
{
    // エラーにする領域が実行のたびに変わらないよう、領域の番号の順に見る
    std::map<RealmType, std::string_view> realm_names;
    for (const auto &[name, realm] : realms_list) {
        realm_names.emplace(realm, name);
    }

    for (const auto &[realm, name] : realm_names) {
        const auto it = session.realms.find(realm);
        if (it == session.realms.end()) {
            msg_print(_("領域 '{}' が定義されていません。", "Realm '{}' is not defined."), name);
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }
        for (auto spell_id = 0; spell_id < SPELLS_IN_REALM; spell_id++) {
            if (!it->second.ids.contains(static_cast<short>(spell_id))) {
                msg_print(_("領域 '{}' の呪文ID '{}' が定義されていません。", "Realm '{}': spell ID '{}' is not defined."), name, spell_id);
                return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
            }
        }
    }
    return {};
}
