/*!
 * @brief ベースアイテム定義の実装
 * @date 2024/11/16
 * @author deskull, Hourier
 */

#include "system/baseitem/baseitem-definition.h"
#include "util/string-processor.h"

BaseitemDefinition::BaseitemDefinition()
    : bi_key(ItemKindType::NONE)
    , symbol_definition(DisplaySymbol(0, '\0'))
    , symbol_config(DisplaySymbol(0, '\0'))
{
}

/*!
 * @brief 正常なベースアイテムかを判定する
 * @return 正常なベースアイテムか否か
 * @details ID 0は「何か」という異常アイテム
 * その他、ベースアイテムIDは歴史的事情により歯抜けが多数あり、それらは名前が空欄になるようにオブジェクトを生成している
 * @todo v3.1以降で歯抜けを埋めるようにベースアイテムを追加していきたい (詳細未定)
 */
bool BaseitemDefinition::is_valid() const
{
    return (this->idx > 0) && !this->name.empty();
}

/*!
 * @brief ベースアイテム名を返す
 * @return ベースアイテム名
 */
std::string BaseitemDefinition::stripped_name() const
{
    const auto tokens = str_split(this->name, ' ');
    std::stringstream ss;
    for (const auto &token : tokens) {
        if (token == "" || token == "~" || token == "&" || token == "#") {
            continue;
        }

        auto offset = 0;
        auto endpos = token.size();
        auto is_kanji = false;
        if (token[0] == '~' || token[0] == '#') {
            offset++;
        }
#ifdef JP
        if (token.size() > 2) {
            is_kanji = iskanji(token[endpos - 2]);
        }

#endif
        if (!is_kanji && (token[endpos - 1] == '~' || token[endpos - 1] == '#')) {
            endpos--;
        }

        ss << token.substr(offset, endpos);
    }

    ss << " ";
    return ss.str();
}

bool BaseitemDefinition::order_cost(const BaseitemDefinition &other) const
{
    return this->cost < other.cost;
}

/*!
 * @brief 最初から簡易な名称が明らかなベースアイテムにその旨のフラグを立てる
 */
void BaseitemDefinition::decide_easy_know()
{
    switch (this->bi_key.tval()) {
    case ItemKindType::LIFE_BOOK:
    case ItemKindType::SORCERY_BOOK:
    case ItemKindType::NATURE_BOOK:
    case ItemKindType::CHAOS_BOOK:
    case ItemKindType::DEATH_BOOK:
    case ItemKindType::TRUMP_BOOK:
    case ItemKindType::ARCANE_BOOK:
    case ItemKindType::CRAFT_BOOK:
    case ItemKindType::DEMON_BOOK:
    case ItemKindType::CRUSADE_BOOK:
    case ItemKindType::MUSIC_BOOK:
    case ItemKindType::HISSATSU_BOOK:
    case ItemKindType::HEX_BOOK:
    case ItemKindType::FLASK:
    case ItemKindType::JUNK:
    case ItemKindType::BOTTLE:
    case ItemKindType::FLAVOR_SKELETON:
    case ItemKindType::SPIKE:
    case ItemKindType::WHISTLE:
    case ItemKindType::FOOD:
    case ItemKindType::POTION:
    case ItemKindType::SCROLL:
    case ItemKindType::ROD:
    case ItemKindType::STATUE:
    case ItemKindType::PARCHMENT:
        this->easy_know = true;
        return;
    default:
        this->easy_know = false;
        return;
    }
}

/*!
 * @brief オブジェクトを試行済にする
 */
void BaseitemDefinition::mark_as_tried()
{
    this->tried = true;
}

void BaseitemDefinition::mark_as_aware()
{
    this->aware = true;
}
