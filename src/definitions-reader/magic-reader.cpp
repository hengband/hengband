#include "definitions-reader/magic-reader.h"
#include "definitions-reader/json-reader-util.h"
#include "definitions-reader/magic-tokens-table.h"
#include "definitions-reader/parse-error-types.h"
#include "locale/language-switcher.h"
#include "player-info/class-info.h"
#include "system/spell-info-list.h"
#include "system/system-variables.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"
#include <algorithm>
#include <iterator>
#include <limits>
#include <nlohmann/json.hpp>
#include <utility>

namespace {
//! 消費MP・初回経験値倍率・装備重量閾値の上限。
constexpr auto MAX_CLASS_MAGIC_VALUE = std::numeric_limits<short>::max() / 16;

//! 魔法書 1 巻に載っている魔法の数
constexpr auto SPELLS_IN_BOOK = 8;

/*!
 * @brief 魔法の載っている巻を返す
 * @param spell_id 呪文ID (0〜31)
 * @return 巻 (1〜4)
 */
int book_of_spell(int spell_id)
{
    return spell_id / SPELLS_IN_BOOK + 1;
}
}

MagicReader::Session MagicReader::create_session(tl::optional<int> last_id)
{
    return Session(DefinitionIdOrder::STRICTLY_INCREASING, { 0, static_cast<int>(class_magics_info.size()) - 1 }, _("職業魔法", "class magic"), last_id);
}

MagicReader::MagicReader(const nlohmann::json &class_data, Session &session)
    : class_data(class_data)
    , session(session)
{
}

/*!
 * @brief 職業魔法情報(ClassMagicDefinitions)のパース関数
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> MagicReader::read() const
{
    if (!this->class_data.is_object()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    const auto class_id_result = this->read_class_id();
    if (!class_id_result) {
        return tl::make_unexpected(class_id_result.error());
    }

    const auto id_result = this->session.check_and_report(*class_id_result);
    if (!id_result) {
        return tl::make_unexpected(id_result.error());
    }
    const auto class_id = *id_result;
    // レコード全体が有効と確定するまで、既存の職業定義を維持する。
    player_magic magics_info = class_magics_info[class_id];

    if (const auto result = this->set_spell_type(magics_info); !result) {
        msg_print(_("呪文タイプ読込失敗。", "Failed to load spell type."));
        return result;
    }
    if (const auto result = this->set_magic_status(magics_info); !result) {
        msg_print(_("呪文行使に使用する能力値読込失敗。", "Failed to load magic status."));
        return result;
    }
    if (const auto result = info_set_bool(get_json_value(this->class_data, "has_glove_mp_penalty"), magics_info.has_glove_mp_penalty, true); !result) {
        msg_print(_("籠手によるMPペナルティ読込失敗。", "Failed to load glove_mp_penalty."));
        return result;
    }
    if (const auto result = info_set_bool(get_json_value(this->class_data, "has_magic_fail_rate_cap"), magics_info.has_magic_fail_rate_cap, true); !result) {
        msg_print(_("最低呪文失率情報読込失敗。", "Failed to load magic_fail_rate_cap."));
        return result;
    }
    if (const auto result = info_set_bool(get_json_value(this->class_data, "is_spell_trainable"), magics_info.is_spell_trainable, true); !result) {
        msg_print(_("呪文訓練可能性読込失敗。", "Failed to load spell_trainability."));
        return result;
    }
    // 魔法を使わない職業は省略する
    magics_info.spell_first = tl::nullopt;
    if (const auto &first_spell_level = get_json_value(this->class_data, "first_spell_level"); !first_spell_level.is_null()) {
        int spell_first{};
        if (const auto result = info_set_integer(first_spell_level, spell_first, true, Range(0, PY_MAX_LEVEL)); !result) {
            msg_print(_("呪文行使開始レベル読込失敗。", "Failed to load spell-first level."));
            return result;
        }
        magics_info.spell_first = spell_first;
    }
    // 魔法書を使う職業は、MP と覚えられる魔法の数を最初の魔法のレベルから求めるので省略できない
    if ((magics_info.spell_book != ItemKindType::NONE) && !magics_info.spell_first) {
        msg_print(_("魔法書を使う職業は呪文行使開始レベルを省略できません。", "A class that uses spellbooks requires first_spell_level."));
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }
    if (const auto result = info_set_integer(get_json_value(this->class_data, "armour_weight_limit"), magics_info.spell_weight, true, Range(0, MAX_CLASS_MAGIC_VALUE)); !result) {
        msg_print(_("MP重量制限値読込失敗。", "Failed to load spell-weight value."));
        return result;
    }
    if (const auto result = this->set_realm_data(magics_info); !result) {
        msg_print(_("呪文データ読込失敗。", "Failed to load spell data."));
        return result;
    }

    class_magics_info[class_id] = std::move(magics_info);
    return {};
}

/*!
 * @brief JSON Objectから職業IDを取得する
 * @return 職業ID、失敗時はエラーコード
 */
tl::expected<int, ParseErrorType> MagicReader::read_class_id() const
{
    const auto &classname_obj = get_json_value(this->class_data, "name");
    const auto player_class = info_read_token(classname_obj, magic_class_list);
    if (!player_class) {
        if (player_class.error() == ParseErrorType::INVALID_FLAG) {
            msg_print(_("未知の職業名 '{}'。", "Unknown class name '{}'."), classname_obj.get<std::string>());
        } else {
            msg_print(_("職業名は文字列が必要です。", "Class name must be a string."));
        }
        return tl::make_unexpected(player_class.error());
    }
    return enum2i(*player_class);
}

/*!
 * @brief JSON Objectから職業で使用する呪文タイプを取得する
 * @param magics_info 保存先の魔法情報構造体
 * @return 成功時は空の値、失敗時はエラーコード
 */
tl::expected<void, ParseErrorType> MagicReader::set_spell_type(player_magic &magics_info) const
{
    const auto &spell_book = get_json_value(this->class_data, "spell_type");
    const auto tval = info_read_token(spell_book, magic_spelltype_to_tval);
    if (!tval) {
        return tl::make_unexpected(tval.error());
    }
    magics_info.spell_book = *tval;
    return {};
}

/*!
 * @brief JSON Objectから職業で呪文行使に使用する能力値を取得する
 * @param magics_info 保存先の魔法情報構造体
 * @return 成功時は空の値、失敗時はエラーコード
 */
tl::expected<void, ParseErrorType> MagicReader::set_magic_status(player_magic &magics_info) const
{
    const auto &magic_status = get_json_value(this->class_data, "magic_status");
    const auto status = info_read_token(magic_status, magic_name_to_stat);
    if (!status) {
        return tl::make_unexpected(status.error());
    }
    magics_info.spell_stat = *status;
    return {};
}

/*!
 * @brief JSON Objectから各呪文の詳細を取得する
 * @param spell_data 各呪文情報の格納されたJSON Object
 * @param magics_info 保存先の魔法情報構造体
 * @param realm 呪文の領域
 * @param listed 領域の魔法ごとに、定義に書かれたかどうか (書かれた魔法の分を立てる)
 * @return 成功時は空の値、失敗時はエラーコード
 */
tl::expected<void, ParseErrorType> MagicReader::set_spell_data(const nlohmann::json &spell_data, player_magic &magics_info, RealmType realm, SpellListedFlags &listed) const
{
    const auto spell_id = read_listed_spell(get_json_value(spell_data, "spell_tag"), magics_info, realm, listed);
    if (!spell_id) {
        return tl::make_unexpected(spell_id.error());
    }
    auto &info = magics_info.info[enum2i(realm)][*spell_id];

    PLAYER_LEVEL learn_level{};
    if (const auto result = info_set_integer(get_json_value(spell_data, "learn_level"), learn_level, true, Range(1, PY_MAX_LEVEL)); !result) {
        msg_print(_("呪文学習レベル読込失敗。", "Failed to load spell learn_level."));
        return result;
    }
    info.slevel = learn_level;
    if (const auto result = info_set_integer(get_json_value(spell_data, "mana_cost"), info.smana, true, Range(0, MAX_CLASS_MAGIC_VALUE)); !result) {
        msg_print(_("呪文コスト読込失敗。", "Failed to load spell mana_cost."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(spell_data, "difficulty"), info.sfail, true, Range(0, 100)); !result) {
        msg_print(_("呪文難易度読込失敗。", "Failed to load spell difficulty."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(spell_data, "first_cast_exp_rate"), info.sexp, true, Range(0, MAX_CLASS_MAGIC_VALUE)); !result) {
        msg_print(_("呪文詠唱ボーナスEXP読込失敗。", "Failed to load spell first_cast_exp_rate."));
        return result;
    }

    // 第二領域として選んだときの値は、今は第一領域と同じ (位置ごとの値は定義ファイルにまだ無い)
    magics_info.info_second[enum2i(realm)][*spell_id] = info;
    return {};
}

/*!
 * @brief 職業魔法の定義に書かれた魔法の名前を読み、書いてよい魔法かを確かめる
 * @param spell_tag_obj 魔法の名前 (呪文タグ)
 * @param magics_info 保存先の魔法情報構造体 (習得できる巻数を見る)
 * @param realm 呪文の領域
 * @param listed 領域の魔法ごとに、定義に書かれたかどうか (読んだ魔法の分を立てる)
 * @return 呪文ID。名前が無い・知らない名前ならそのエラー、同じ魔法を 2 度書いたか習得できる巻数より後ろの魔法なら INVALID_VALUE
 */
tl::expected<int, ParseErrorType> MagicReader::read_listed_spell(const nlohmann::json &spell_tag_obj, const player_magic &magics_info, RealmType realm, SpellListedFlags &listed)
{
    const auto spell_id = info_read_token(spell_tag_obj, [realm](const std::string &name) { return SpellInfoList::get_instance().get_spell_id(realm, name); });
    if (!spell_id) {
        return tl::make_unexpected(spell_id.error());
    }

    const auto &tag = spell_tag_obj.get_ref<const std::string &>();
    if (listed[*spell_id]) {
        msg_print(_("魔法 '{}' が 2 度書かれています。", "Spell '{}' is listed twice."), tag);
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
    // 習得できる巻数より後ろの巻の魔法は、書かなくても習得できない
    if (book_of_spell(*spell_id) > magics_info.max_book[enum2i(realm)]) {
        msg_print(_("魔法 '{}' は習得できる巻数より後ろの巻にあります。", "Spell '{}' is in a book after max_book."), tag);
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }

    listed[*spell_id] = true;
    return static_cast<int>(*spell_id);
}

/*!
 * @brief JSON Arrayから習得できない魔法の一覧を読む
 * @param unlearnable_obj 習得できない魔法の名前 (呪文タグ) の配列。省略してよい
 * @param magics_info 保存先の魔法情報構造体
 * @param realm 呪文の領域
 * @param listed 領域の魔法ごとに、定義に書かれたかどうか (読んだ魔法の分を立てる)
 * @return 成功時は空の値、失敗時はエラーコード
 * @details 習得できない魔法の値は、領域を読み始めるときに空 (習得レベルが値なし) にしてあるので、ここでは書き換えない。
 */
tl::expected<void, ParseErrorType> MagicReader::set_unlearnable_spells(const nlohmann::json &unlearnable_obj, const player_magic &magics_info, RealmType realm, SpellListedFlags &listed)
{
    if (unlearnable_obj.is_null()) {
        return {};
    }
    if (!unlearnable_obj.is_array()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }
    for (const auto &spell_tag_obj : unlearnable_obj) {
        if (const auto spell_id = read_listed_spell(spell_tag_obj, magics_info, realm, listed); !spell_id) {
            return tl::make_unexpected(spell_id.error());
        }
    }
    return {};
}

/*!
 * @brief 習得できる巻の魔法がすべて、習得できる魔法か習得できない魔法のどちらかとして書かれているかを確かめる
 * @param realm 呪文の領域
 * @param max_book 習得できる巻数
 * @param listed 領域の魔法ごとに、定義に書かれたかどうか
 * @return すべて書かれていれば空の値、書かれていない魔法があれば TOO_FEW_ARGUMENTS
 * @details 魔法の定義 (SpellDefinitions) に無い枠は数えない。
 */
tl::expected<void, ParseErrorType> MagicReader::check_all_spells_listed(RealmType realm, int max_book, const SpellListedFlags &listed)
{
    const auto &spells = SpellInfoList::get_instance();
    for (auto spell_id = 0; spell_id < SPELLS_IN_REALM; spell_id++) {
        if ((book_of_spell(spell_id) > max_book) || listed[spell_id]) {
            continue;
        }

        const auto &spell = spells.get_spell_info(realm, spell_id);
        if (spell.base.is_learnable()) {
            msg_print(_("魔法 '{}' が spells_info にも unlearnable にもありません。", "Spell '{}' is in neither spells_info nor unlearnable."), spell.tag);
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }
    }
    return {};
}

/*!
 * @brief JSON Objectから職業毎に定義された各呪文の詳細を取得する
 * @param magics_info 保存先の魔法情報構造体
 * @return 成功時は空の値、失敗時はエラーコード
 */
tl::expected<void, ParseErrorType> MagicReader::set_realm_data(player_magic &magics_info) const
{
    const auto &realms_data = get_json_value(this->class_data, "realms");
    return info_read_object_array(realms_data, [this, &magics_info](const nlohmann::json &realm) -> tl::expected<void, ParseErrorType> {
        const auto &realm_name_obj = get_json_value(realm, "name");
        const auto realm_name = info_read_token(realm_name_obj, realms_list);
        if (!realm_name) {
            return tl::make_unexpected(realm_name.error());
        }

        const auto realm_id = *realm_name;
        // 省略時は全巻を習得できる
        auto &max_book = magics_info.max_book[enum2i(realm_id)];
        max_book = MAX_BOOKS_IN_REALM;
        if (const auto result = info_set_integer(get_json_value(realm, "max_book"), max_book, false, Range(1, MAX_BOOKS_IN_REALM)); !result) {
            msg_print(_("習得できる巻数読込失敗。", "Failed to load max_book."));
            return result;
        }

        // 読み直したときに前の値が残らないよう、領域の魔法をすべて習得できない状態にしてから読む
        std::fill(std::begin(magics_info.info[enum2i(realm_id)]), std::end(magics_info.info[enum2i(realm_id)]), magic_type{});
        std::fill(std::begin(magics_info.info_second[enum2i(realm_id)]), std::end(magics_info.info_second[enum2i(realm_id)]), magic_type{});

        SpellListedFlags listed{};
        const auto &spells_info_obj = get_json_value(realm, "spells_info");
        if (const auto result = info_read_object_array(spells_info_obj, [this, &magics_info, realm_id, &listed](const nlohmann::json &spell) -> tl::expected<void, ParseErrorType> {
                return this->set_spell_data(spell, magics_info, realm_id, listed);
            });
            !result) {
            return result;
        }
        if (const auto result = set_unlearnable_spells(get_json_value(realm, "unlearnable"), magics_info, realm_id, listed); !result) {
            msg_print(_("習得できない魔法の一覧読込失敗。", "Failed to load unlearnable spells."));
            return result;
        }

        return check_all_spells_listed(realm_id, max_book, listed);
    });
}
