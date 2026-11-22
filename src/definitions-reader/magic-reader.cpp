#include "definitions-reader/magic-reader.h"
#include "definitions-reader/json-reader-util.h"
#include "definitions-reader/magic-tokens-table.h"
#include "definitions-reader/parse-error-types.h"
#include "locale/language-switcher.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/class-types.h"
#include "player/player-realm.h"
#include "system/spell-info-list.h"
#include "system/system-variables.h"
#include "util/enum-converter.h"
#include "util/enum-range.h"
#include "view/display-messages.h"
#include <algorithm>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>

namespace {
//! 装備重量閾値の上限。
constexpr auto MAX_CLASS_MAGIC_VALUE = std::numeric_limits<short>::max() / 16;

//! 得意さの段階の範囲
constexpr auto MIN_APTITUDE = -9;
constexpr auto MAX_APTITUDE = 9;

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

/*!
 * @brief 全職業の職業魔法が定義されたかを確かめる
 * @param session 読み込みに使った ID の検査
 * @return 全職業のレコードがあれば空の値。抜けがあれば TOO_FEW_ARGUMENTS
 * @details 定義されなかった職業は、魔法を使えない初期値のまま黙って扱われてしまうので、読み込みの最後に確かめる。
 */
tl::expected<void, ParseErrorType> MagicReader::validate_all_defined(const Session &session)
{
    for (const auto pclass : EnumRange(PlayerClassType::WARRIOR, PlayerClassType::MAX)) {
        if (session.is_defined(enum2i(pclass))) {
            continue;
        }

        const auto it = std::find_if(magic_class_list.begin(), magic_class_list.end(), [pclass](const auto &entry) { return entry.second == pclass; });
        const auto name = (it != magic_class_list.end()) ? std::string(it->first) : std::to_string(enum2i(pclass));
        msg_print(_("職業 '{}' の職業魔法が定義されていません。", "Class magic of '{}' is not defined."), name);
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }
    return {};
}

MagicReader::Session MagicReader::create_session(tl::optional<int> last_id)
{
    return Session(DefinitionIdOrder::STRICTLY_INCREASING, { 0, static_cast<int>(class_magics_info.size()) - 1 }, _("職業魔法", "class magic"), last_id);
}

/*!
 * @brief ClassMagicDefinitions のルートの版と、段階 1 つあたりの動き (aptitude_steps) を読む
 * @param root ClassMagicDefinitions のルートの JSON Object
 * @return 段階 1 つあたりの動き。版が 2 でないか、aptitude_steps が無いか範囲外ならエラーコード
 */
tl::expected<AptitudeSteps, ParseErrorType> MagicReader::read_aptitude_steps(const nlohmann::json &root)
{
    if (const auto result = info_validate_version(root, 2); !result) {
        msg_print(_("職業魔法の定義形式の版が不正です。", "Invalid version of class magic definitions."));
        return tl::make_unexpected(result.error());
    }

    const auto &steps_obj = get_json_value(root, "aptitude_steps");
    if (steps_obj.is_null()) {
        msg_print(_("aptitude_steps がありません。", "aptitude_steps is missing."));
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }
    if (!steps_obj.is_object()) {
        msg_print(_("aptitude_steps はオブジェクトである必要があります。", "aptitude_steps must be an object."));
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    AptitudeSteps steps;
    if (const auto result = info_set_integer(get_json_value(steps_obj, "learn_level_percent"), steps.learn_level_percent, true, Range(0, 100)); !result) {
        msg_print(_("段階 1 つあたりの習得レベルの動き読込失敗。", "Failed to load aptitude_steps.learn_level_percent."));
        return tl::make_unexpected(result.error());
    }
    if (const auto result = info_set_integer(get_json_value(steps_obj, "mana_cost_percent"), steps.mana_cost_percent, true, Range(0, 50)); !result) {
        msg_print(_("段階 1 つあたりの消費MPの動き読込失敗。", "Failed to load aptitude_steps.mana_cost_percent."));
        return tl::make_unexpected(result.error());
    }
    if (const auto result = info_set_integer(get_json_value(steps_obj, "difficulty"), steps.difficulty, true, Range(0, 20)); !result) {
        msg_print(_("段階 1 つあたりの難易度の動き読込失敗。", "Failed to load aptitude_steps.difficulty."));
        return tl::make_unexpected(result.error());
    }
    return steps;
}

/*!
 * @brief コンストラクタ
 * @param class_data 職業 1 つの JSON Object
 * @param session 職業IDの並びを確かめるセッション
 * @param steps 段階 1 つあたりの動き (省略時はすべて 0 で、どの段階でも基準値のまま)
 */
MagicReader::MagicReader(const nlohmann::json &class_data, Session &session, const AptitudeSteps &steps)
    : class_data(class_data)
    , session(session)
    , steps(steps)
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
    if (const auto result = this->set_realm_data(magics_info, i2enum<PlayerClassType>(class_id)); !result) {
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
 * @brief 職業魔法の定義に書かれた魔法の名前を読み、書いてよい魔法かを確かめる
 * @param spell_tag_obj 魔法の名前 (呪文タグ)
 * @param max_book 領域の習得できる巻数
 * @param realm 呪文の領域
 * @param listed 領域の魔法ごとに、unlearnable か overrides に書かれたかどうか (読んだ魔法の分を立てる)
 * @return 呪文ID。名前が無い・知らない名前ならそのエラー、同じ魔法を 2 度書いたか習得できる巻数より後ろの魔法なら INVALID_VALUE
 */
tl::expected<int, ParseErrorType> MagicReader::read_listed_spell(const nlohmann::json &spell_tag_obj, int max_book, RealmType realm, SpellListedFlags &listed)
{
    const auto spell_id = info_read_token(spell_tag_obj, [realm](const std::string &name) { return SpellInfoList::get_instance().get_spell_id(realm, name); });
    if (!spell_id) {
        if (spell_id.error() == ParseErrorType::INVALID_FLAG) {
            msg_print(_("魔法 '{}' は領域の魔法の定義にありません。", "Spell '{}' is not defined in the realm."), spell_tag_obj.get_ref<const std::string &>());
        }
        return tl::make_unexpected(spell_id.error());
    }

    const auto &tag = spell_tag_obj.get_ref<const std::string &>();
    if (listed[*spell_id]) {
        msg_print(_("魔法 '{}' が 2 度書かれています。", "Spell '{}' is listed twice."), tag);
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
    // 習得できる巻数より後ろの巻の魔法は、書かなくても習得できない
    if (book_of_spell(*spell_id) > max_book) {
        msg_print(_("魔法 '{}' は習得できる巻数より後ろの巻にあります。", "Spell '{}' is in a book after max_book."), tag);
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }

    listed[*spell_id] = true;
    return static_cast<int>(*spell_id);
}

/*!
 * @brief JSON Arrayから習得できない魔法の一覧を読む
 * @param unlearnable_obj 習得できない魔法の名前 (呪文タグ) の配列。省略してよい
 * @param max_book 領域の習得できる巻数
 * @param realm 呪文の領域
 * @param listed 領域の魔法ごとに、unlearnable か overrides に書かれたかどうか (読んだ魔法の分を立てる)
 * @param unlearnable 領域の魔法ごとに、習得できない魔法かどうか (読んだ魔法の分を立てる)
 * @return 成功時は空の値、失敗時はエラーコード
 */
tl::expected<void, ParseErrorType> MagicReader::read_unlearnable_spells(const nlohmann::json &unlearnable_obj, int max_book, RealmType realm, SpellListedFlags &listed, SpellListedFlags &unlearnable)
{
    if (unlearnable_obj.is_null()) {
        return {};
    }
    if (!unlearnable_obj.is_array()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }
    for (const auto &spell_tag_obj : unlearnable_obj) {
        const auto spell_id = read_listed_spell(spell_tag_obj, max_book, realm, listed);
        if (!spell_id) {
            return tl::make_unexpected(spell_id.error());
        }
        unlearnable[*spell_id] = true;
    }
    return {};
}

/*!
 * @brief JSON Arrayから魔法ごとの段階の上書きを読む
 * @param overrides_obj 上書き (呪文タグと段階) の配列。省略してよい
 * @param max_book 領域の習得できる巻数
 * @param realm 呪文の領域
 * @param aptitude 領域の段階 (第一領域として選んだとき)
 * @param aptitude_as_second 領域の段階 (第二領域として選んだとき)
 * @param listed 領域の魔法ごとに、unlearnable か overrides に書かれたかどうか (読んだ魔法の分を立てる)
 * @param overrides 領域の魔法ごとの上書きの段階 (読んだ魔法の分を書く)
 * @return 成功時は空の値、失敗時はエラーコード
 * @details 習得できない魔法への上書きは、unlearnable と同じ魔法を 2 度書いたものとしてエラーにする。
 * 第一・第二の両方の段階と同じ段階の上書きは、書かなくても同じ値になるのでエラーにする。
 */
tl::expected<void, ParseErrorType> MagicReader::read_overrides(const nlohmann::json &overrides_obj, int max_book, RealmType realm, int aptitude, int aptitude_as_second, SpellListedFlags &listed, SpellAptitudes &overrides)
{
    if (overrides_obj.is_null()) {
        return {};
    }
    return info_read_object_array(overrides_obj, [max_book, realm, aptitude, aptitude_as_second, &listed, &overrides](const nlohmann::json &override_obj) -> tl::expected<void, ParseErrorType> {
        const auto &spell_tag_obj = get_json_value(override_obj, "spell_tag");
        const auto spell_id = read_listed_spell(spell_tag_obj, max_book, realm, listed);
        if (!spell_id) {
            return tl::make_unexpected(spell_id.error());
        }

        auto spell_aptitude = 0;
        if (const auto result = info_set_integer(get_json_value(override_obj, "aptitude"), spell_aptitude, true, Range(MIN_APTITUDE, MAX_APTITUDE)); !result) {
            msg_print(_("魔法ごとの段階読込失敗。", "Failed to load spell aptitude."));
            return result;
        }
        if ((spell_aptitude == aptitude) && (spell_aptitude == aptitude_as_second)) {
            msg_print(_("魔法 '{}' の段階は領域の段階と同じです (書かない)。", "Aptitude of spell '{}' is the same as the realm (omit it)."), spell_tag_obj.get_ref<const std::string &>());
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }

        overrides[*spell_id] = spell_aptitude;
        return {};
    });
}

/*!
 * @brief 職業が選べる魔法領域が、過不足なく定義に書かれているかを確かめる
 * @param pclass 職業
 * @param listed 領域ごとに、定義に書かれたかどうか
 * @param has_second 領域ごとに、aptitude_as_second が書かれたかどうか
 * @return 過不足が無ければ空の値。選べる領域が無ければ TOO_FEW_ARGUMENTS、選べない領域があるか、
 * 第一・第二の両方には選べない領域に aptitude_as_second があれば INVALID_VALUE
 * @details 全領域を使う職業 (ソーサラー・赤魔道師) は魔法の 10 領域すべてを書く。技能領域は書かない。
 */
tl::expected<void, ParseErrorType> MagicReader::check_realm_choices(PlayerClassType pclass, const RealmListedFlags &listed, const RealmListedFlags &has_second)
{
    const auto realm1_choices = PlayerRealm::get_realm1_choices(pclass);
    const auto realm2_choices = PlayerRealm::get_realm2_choices(pclass);
    const auto is_every_magic = PlayerClass::is_every_magic(pclass);
    for (const auto realm : EnumRange(RealmType::LIFE, RealmType::MAX)) {
        if (!PlayerRealm::is_magic(realm)) {
            if (listed[enum2i(realm)]) {
                msg_print(_("技能領域 '{}' は書きません。", "Technic realm '{}' must not be listed."), PlayerRealm::get_name(realm).string());
                return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
            }
            continue;
        }

        const auto is_choosable = is_every_magic || realm1_choices.has(realm) || realm2_choices.has(realm);
        if (is_choosable && !listed[enum2i(realm)]) {
            msg_print(_("職業が選べる領域 '{}' が書かれていません。", "Realm '{}' that the class can choose is not listed."), PlayerRealm::get_name(realm).string());
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }
        if (!is_choosable && listed[enum2i(realm)]) {
            msg_print(_("職業が選べない領域 '{}' が書かれています。", "Realm '{}' that the class cannot choose is listed."), PlayerRealm::get_name(realm).string());
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        if (has_second[enum2i(realm)] && !(realm1_choices.has(realm) && realm2_choices.has(realm))) {
            msg_print(_("領域 '{}' は第一・第二の両方には選べないので、第二領域としての段階は書けません。", "Realm '{}' cannot be both the first and the second realm, so aptitude_as_second is not allowed."), PlayerRealm::get_name(realm).string());
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
    }
    return {};
}

/*!
 * @brief JSON Objectから職業毎に定義された各領域の段階を取得する
 * @param magics_info 保存先の魔法情報構造体
 * @param pclass 職業
 * @return 成功時は空の値、失敗時はエラーコード
 */
tl::expected<void, ParseErrorType> MagicReader::set_realm_data(player_magic &magics_info, PlayerClassType pclass) const
{
    RealmListedFlags realm_listed{};
    RealmListedFlags has_second{};
    const auto &realms_data = get_json_value(this->class_data, "realms");
    const auto result = info_read_object_array(realms_data, [this, &magics_info, &realm_listed, &has_second](const nlohmann::json &realm) -> tl::expected<void, ParseErrorType> {
        const auto &realm_name_obj = get_json_value(realm, "name");
        const auto realm_name = info_read_token(realm_name_obj, realms_list);
        if (!realm_name) {
            if (realm_name.error() == ParseErrorType::INVALID_FLAG) {
                msg_print(_("領域 '{}' を知りません。", "Unknown realm '{}'."), realm_name_obj.get_ref<const std::string &>());
            }
            return tl::make_unexpected(realm_name.error());
        }

        const auto realm_id = *realm_name;
        if (realm_listed[enum2i(realm_id)]) {
            msg_print(_("領域 '{}' が 2 度書かれています。", "Realm '{}' is listed twice."), realm_name_obj.get_ref<const std::string &>());
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        realm_listed[enum2i(realm_id)] = true;
        const auto spells_result = this->set_realm_spells(realm, magics_info, realm_id, has_second[enum2i(realm_id)]);
        if (!spells_result) {
            // 領域の中の診断 (段階、巻数、魔法の名前など) がどの領域のものかを示す
            msg_print(_("領域 '{}' の読込失敗。", "Failed to load realm '{}'."), realm_name_obj.get_ref<const std::string &>());
        }
        return spells_result;
    });
    if (!result) {
        return result;
    }
    return check_realm_choices(pclass, realm_listed, has_second);
}

/*!
 * @brief 領域の段階と例外を読み、職業別の魔法の値を基準値から求める
 * @param realm_data 領域の JSON Object
 * @param magics_info 保存先の魔法情報構造体
 * @param realm 呪文の領域
 * @param has_second aptitude_as_second が書かれていれば true にする
 * @return 成功時は空の値、失敗時はエラーコード
 * @details 第一領域用の表は aptitude、第二領域用の表は aptitude_as_second (省略時は aptitude) の段階で求める。
 * 魔法ごとの上書きは両方の表に効く。習得できる巻数より後ろの巻の魔法と unlearnable の魔法は習得できない。
 */
tl::expected<void, ParseErrorType> MagicReader::set_realm_spells(const nlohmann::json &realm_data, player_magic &magics_info, RealmType realm, bool &has_second) const
{
    // 省略時は全巻を習得できる
    auto &max_book = magics_info.max_book[enum2i(realm)];
    max_book = MAX_BOOKS_IN_REALM;
    if (const auto result = info_set_integer(get_json_value(realm_data, "max_book"), max_book, false, Range(1, MAX_BOOKS_IN_REALM)); !result) {
        msg_print(_("習得できる巻数読込失敗。", "Failed to load max_book."));
        return result;
    }

    // 省略時は段階 0 (基準値のまま)
    auto aptitude = 0;
    if (const auto result = info_set_integer(get_json_value(realm_data, "aptitude"), aptitude, false, Range(MIN_APTITUDE, MAX_APTITUDE)); !result) {
        msg_print(_("領域の段階読込失敗。", "Failed to load realm aptitude."));
        return result;
    }
    auto aptitude_as_second = aptitude;
    if (const auto &second_obj = get_json_value(realm_data, "aptitude_as_second"); !second_obj.is_null()) {
        if (const auto result = info_set_integer(second_obj, aptitude_as_second, true, Range(MIN_APTITUDE, MAX_APTITUDE)); !result) {
            msg_print(_("第二領域としての段階読込失敗。", "Failed to load aptitude_as_second."));
            return result;
        }
        if (aptitude_as_second == aptitude) {
            msg_print(_("第二領域としての段階が第一領域と同じです (書かない)。", "aptitude_as_second is the same as aptitude (omit it)."));
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        has_second = true;
    }

    SpellListedFlags listed{};
    SpellListedFlags unlearnable{};
    if (const auto result = read_unlearnable_spells(get_json_value(realm_data, "unlearnable"), max_book, realm, listed, unlearnable); !result) {
        msg_print(_("習得できない魔法の一覧読込失敗。", "Failed to load unlearnable spells."));
        return result;
    }
    SpellAptitudes overrides{};
    if (const auto result = read_overrides(get_json_value(realm_data, "overrides"), max_book, realm, aptitude, aptitude_as_second, listed, overrides); !result) {
        msg_print(_("魔法ごとの段階の一覧読込失敗。", "Failed to load spell overrides."));
        return result;
    }

    const auto &spells = SpellInfoList::get_instance();
    for (auto spell_id = 0; spell_id < SPELLS_IN_REALM; spell_id++) {
        auto &first = magics_info.info[enum2i(realm)][spell_id];
        auto &second = magics_info.info_second[enum2i(realm)][spell_id];
        // 読み直したときに前の値が残らないよう、習得できない状態から組み立てる
        first = magic_type{};
        second = magic_type{};
        if ((book_of_spell(spell_id) > max_book) || unlearnable[spell_id]) {
            continue;
        }

        const auto &spell = spells.get_spell_info(realm, spell_id);
        const auto derived_first = derive_magic_by_aptitude(spell.base, this->steps, overrides[spell_id].value_or(aptitude));
        const auto derived_second = derive_magic_by_aptitude(spell.base, this->steps, overrides[spell_id].value_or(aptitude_as_second));
        if (!derived_first || !derived_second) {
            msg_print(_("魔法 '{}' の職業別の値が範囲外です。", "Derived values of spell '{}' are out of range."), spell.tag);
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        first = *derived_first;
        second = *derived_second;
    }
    return {};
}
