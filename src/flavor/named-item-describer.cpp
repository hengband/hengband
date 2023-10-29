#include "flavor/named-item-describer.h"
#include "artifact/fixed-art-types.h"
#include "flavor/flavor-util.h"
#include "flavor/object-flavor-types.h"
#include "flavor/tval-description-switcher.h"
#include "game-option/text-display-options.h"
#include "locale/english.h"
#include "mind/mind-weaponsmith.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "perception/object-perception.h"
#include "system/artifact-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#ifdef JP
#else
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "object/tval-types.h"
#include "system/monster-race-info.h"
#endif
#include <sstream>

static std::string get_fullname_if_set(const ItemEntity &item, const describe_option_type &opt)
{
    if (!opt.aware || item.get_flags().has_not(TR_FULL_NAME)) {
        return "";
    }

    const auto is_known_artifact = opt.known && item.is_fixed_artifact() && none_bits(opt.mode, OD_BASE_NAME);
    return is_known_artifact ? item.get_fixed_artifact().name : item.get_baseitem().name;
}

#ifdef JP
/*!
 * @brief アイテムの個数を記述する
 *
 * @param item アイテムへの参照
 * @param opt 記述オプション
 * @return アイテムの個数を記述した文字列
 */
static std::string describe_item_count_ja(const ItemEntity &item, const describe_option_type &opt)
{
    if (any_bits(opt.mode, OD_OMIT_PREFIX) || (item.number <= 1)) {
        return "";
    }

    return describe_count_with_counter_suffix(item) + "の ";
}

/*!
 * @brief アーティファクトであるマークを記述する
 *
 * 英語の場合アーティファクトは The が付くので分かるが、日本語では分からないので
 * 固定アーティファクトなら「★」、ランダムアーティファクトなら「☆」マークをつける.
 *
 * @param item アイテムへの参照
 * @param opt 記述オプション
 */
static std::string describe_artifact_mark_ja(const ItemEntity &item, const describe_option_type &opt)
{
    if (!opt.known || any_bits(opt.mode, OD_BASE_NAME)) {
        return "";
    }

    if (item.is_fixed_artifact()) {
        return "★";
    } else if (item.is_random_artifact()) {
        return "☆";
    }

    return "";
}

/*!
 * @brief アイテムの固有名称（アイテム本体の手前に表記するもの）を記述する
 *
 * 例）
 * 固定/ランダムアーティファクト: ★リリアのダガー → "リリアの"
 * エゴアイテム: (聖戦者)ロング・ソード → "(聖戦者)"
 *
 * @param item アイテムへの参照
 * @param opt 記述オプション
 * @return 記述した文字列
 */
static std::string describe_unique_name_before_body_ja(const ItemEntity &item, const describe_option_type &opt)
{
    if (!opt.known || any_bits(opt.mode, OD_BASE_NAME)) {
        return "";
    }

    if (item.is_random_artifact()) {
        const std::string_view name_sv = *item.randart_name;

        /* '『' から始まらない伝説のアイテムの名前は最初に付加する */
        /* 英語版のセーブファイルから来た 'of XXX' は,「XXXの」と表示する */
        if (name_sv.starts_with("of ")) {
            std::stringstream ss;
            ss << name_sv.substr(3) << "の";
            return ss.str();
        } else if (!name_sv.starts_with("『") && !name_sv.starts_with("《") && !name_sv.starts_with('\'')) {
            return *item.randart_name;
        }
    }

    if (item.is_fixed_artifact() && item.get_flags().has_not(TR_FULL_NAME)) {
        const auto &artifact = item.get_fixed_artifact();
        /* '『' から始まらない伝説のアイテムの名前は最初に付加する */
        if (artifact.name.find("『", 0, 2) != 0) {
            return artifact.name;
        }

        return "";
    }

    if (item.is_ego()) {
        return item.get_ego().name;
    }

    return "";
}

static std::optional<std::string> describe_random_artifact_name_after_body_ja(const ItemEntity &item)
{
    if (!item.is_random_artifact()) {
        return std::nullopt;
    }

    const std::string_view name_sv = *item.randart_name;
    if (name_sv.starts_with("『") || name_sv.starts_with("《")) {
        return item.randart_name;
    }

    if (!name_sv.starts_with('\'')) {
        return "";
    }

    // "'foobar'" の foobar の部分を取り出し『foobar』と表記する
    // (英語版のセーブファイルのランダムアーティファクトを考慮)
    return format("『%s』", name_sv.substr(1, name_sv.length() - 2).data());
}

static std::string describe_fake_artifact_name_after_body_ja(const ItemEntity &item)
{
    if (!item.is_inscribed()) {
        return "";
    }

    auto str = item.inscription->data();
    while (*str) {
        if (iskanji(*str)) {
            str += 2;
            continue;
        }

        if (*str == '#') {
            break;
        }

        str++;
    }

    if (*str == '\0') {
        return "";
    }

    auto str_aux = angband_strchr(item.inscription->data(), '#');
    return format("『%s』", str_aux + 1);
}

/*!
 * @brief アイテムの固有名称（アイテム本体の後に表記するもの）を記述する。
 *
 * 例）
 * 固定/ランダムアーティファクト: ★ロング・ソード『リンギル』 → "『リンギル』"
 * 銘刻みによる疑似アーティファクト: ロング・ソード『AIR』 → "『AIR』"
 * エゴアイテムはアイテム本体の後に記述されるタイプのものは存在しない。
 *
 * 銘刻みによる疑似アーティファクトは、銘の最後に #foobar と刻むことにより
 * ゲーム上の表記が「アイテム名『foobar』」のようになる機能。
 *
 * @param item アイテムへの参照
 * @param opt 記述オプション
 * @return 記述した文字列
 */
static std::string describe_unique_name_after_body_ja(const ItemEntity &item, const describe_option_type &opt)
{
    if (!opt.known || any_bits(opt.mode, OD_BASE_NAME)) {
        return "";
    }

    if (auto body = describe_random_artifact_name_after_body_ja(item); body) {
        return *body;
    }

    if (item.is_fixed_artifact()) {
        const auto &artifact = item.get_fixed_artifact();
        if (artifact.name.find("『", 0, 2) == 0) {
            return artifact.name;
        }

        return "";
    }

    return describe_fake_artifact_name_after_body_ja(item);
}
#else

static std::string describe_vowel(const ItemEntity &item, std::string_view basename, std::string_view modstr)
{
    bool vowel;
    switch (basename[0]) {
    case '#':
        vowel = is_a_vowel(modstr[0]);
        break;
    case '%':
        vowel = is_a_vowel(baseitems_info[item.bi_id].name[0]);
        break;
    default:
        vowel = is_a_vowel(basename[0]);
        break;
    }

    return (vowel) ? "an " : "a ";
}

static std::string describe_prefix_en(const ItemEntity &item)
{
    if (item.number <= 0) {
        return "no more ";
    }

    if (item.number == 1) {
        return "";
    }

    return std::to_string(item.number) + ' ';
}

static std::string describe_item_count_or_article_en(const ItemEntity &item, const describe_option_type &opt, std::string_view basename, std::string_view modstr)
{
    if (any_bits(opt.mode, OD_OMIT_PREFIX)) {
        return "";
    }

    if (auto prefix = describe_prefix_en(item); !prefix.empty()) {
        return prefix;
    }

    const auto corpse_r_idx = i2enum<MonsterRaceId>(item.pval);
    auto is_unique_corpse = item.bi_key.tval() == ItemKindType::CORPSE;
    is_unique_corpse &= monraces_info[corpse_r_idx].kind_flags.has(MonsterKindType::UNIQUE);
    if ((opt.known && item.is_fixed_or_random_artifact()) || is_unique_corpse) {
        return "The ";
    }

    return describe_vowel(item, basename, modstr);
}

static std::string describe_item_count_or_definite_article_en(const ItemEntity &item, const describe_option_type &opt)
{
    if (any_bits(opt.mode, OD_OMIT_PREFIX)) {
        return "";
    }

    if (auto prefix = describe_prefix_en(item); !prefix.empty()) {
        return prefix;
    }

    if (opt.known && item.is_fixed_or_random_artifact()) {
        return "The ";
    }

    return "";
}

static std::string describe_unique_name_after_body_en(const ItemEntity &item, const describe_option_type &opt)
{
    if (!opt.known || item.get_flags().has(TR_FULL_NAME) || any_bits(opt.mode, OD_BASE_NAME)) {
        return "";
    }

    std::stringstream ss;

    if (item.is_random_artifact()) {
        ss << ' ' << *item.randart_name;
        return ss.str();
    }

    if (item.is_fixed_artifact()) {
        const auto &artifact = ArtifactsInfo::get_instance().get_artifact(item.fixed_artifact_idx);
        ss << ' ' << artifact.name;
        return ss.str();
    }

    if (item.is_ego()) {
        ss << ' ' << egos_info[item.ego_idx].name;
    }

    if (item.is_inscribed()) {
        if (auto str = angband_strchr(item.inscription->data(), '#'); str != nullptr) {
            ss << ' ' << str + 1;
        }
    }

    return ss.str();
}
#endif

/*!
 * @brief アイテム本体の名称を記述する
 *
 * 基本的には basename の内容がそのまま記述されるが、basename 上の特定の文字に対して以下の修正が行われる。
 *
 * - '#' が modstr の内容で置き換えられる:
 *   主に未鑑定名やモンスターボール・像・死体などのモンスター名の部分に使用される
 * - '%' が item のベースアイテム名で置き換えられる:
 *   BaseitemDefinition.txt でカテゴリ内における名称のみが記述されているものに使用される。
 *   たとえば薬の場合 basename は「%の薬」となっており、BaseitemDefinition.txt 内で記述されている
 *   "体力回復" により '%' が置き換えられ「体力回復の薬」と表記されるといった具合。
 * - '~' がアイテムが複数の場合複数形表現で置き換えられる（英語版のみ）
 *
 * @param item アイテムへの参照
 * @param opt 記述オプション
 * @param basename アイテム本体のベースとなる文字列
 * @param modstr ベースとなる文字列上の特定の文字を置き換える文字列
 * @return 記述した文字列
 */
static std::string describe_body(const ItemEntity &item, [[maybe_unused]] const describe_option_type &opt, std::string_view basename, std::string_view modstr)
{
#ifndef JP
    auto pluralize = [&opt, &item](auto &ss, auto it) {
        if (none_bits(opt.mode, OD_NO_PLURAL) && (item.number != 1)) {
            char k = *std::next(it, -1);
            if ((k == 's') || (k == 'h')) {
                ss << 'e';
            }
            ss << 's';
        }
    };
#endif
    std::stringstream ss;

    for (auto it = basename.begin(), it_end = basename.end(); it != it_end; ++it) {
        switch (*it) {
        case '#':
            ss << modstr;
            break;

        case '%': {
            const auto &baseitem = item.get_baseitem();
#ifdef JP
            ss << baseitem.name;
#else
            for (auto ib = baseitem.name.begin(), ib_end = baseitem.name.end(); ib != ib_end; ++ib) {
                if (*ib == '~') {
                    pluralize(ss, ib);
                } else {
                    ss << *ib;
                }
            }
#endif
            break;
        }

#ifndef JP
        case '~':
            pluralize(ss, it);
            break;
#endif

        default:
            ss << *it;
#ifdef JP
            if (iskanji(*it)) {
                ++it;
                ss << *it;
            }
#endif
            break;
        }
    }

    return ss.str();
}

/*!
 * @brief アイテム名を記述する
 *
 * @param item アイテムへの参照
 * @param opt 記述オプション
 * @return std::string 記述したアイテム名
 */
std::string describe_named_item(PlayerType *player_ptr, const ItemEntity &item, const describe_option_type &opt)
{
    auto [basename, modstr] = switch_tval_description(item, opt);
    if (auto name = get_fullname_if_set(item, opt); !name.empty()) {
        basename = std::move(name);
    }
    std::string_view basename_sv = basename;
    std::stringstream ss;

#ifdef JP
    if (basename_sv[0] == '&') {
        basename_sv.remove_prefix(2);
    }
    ss << describe_item_count_ja(item, opt)
       << describe_artifact_mark_ja(item, opt);
#else
    if (basename_sv[0] == '&') {
        basename_sv.remove_prefix(2);
        ss << describe_item_count_or_article_en(item, opt, basename_sv, modstr);
    } else {
        ss << describe_item_count_or_definite_article_en(item, opt);
    }
#endif

#ifdef JP
    if (item.is_smith() && none_bits(opt.mode, OD_BASE_NAME)) {
        ss << format("鍛冶師%sの", player_ptr->name);
    }

    ss << describe_unique_name_before_body_ja(item, opt);
#endif
    if (item.is_spell_book()) {
        // svalは0から数えているので表示用に+1している
        ss << format("Lv%d ", *item.bi_key.sval() + 1);
    }

    ss << describe_body(item, opt, basename_sv, modstr);

#ifdef JP
    ss << describe_unique_name_after_body_ja(item, opt);
#else
    if (item.is_smith() && none_bits(opt.mode, OD_BASE_NAME)) {
        ss << format(" of %s the Smith", player_ptr->name);
    }

    ss << describe_unique_name_after_body_en(item, opt);
#endif

    return ss.str();
}
