/*!
 * @brief アイテム情報をセーブデータから読み込むクラスを選択するファクトリクラス
 * @date 2021/10/16
 * @author Hourier
 */

#include "load/item/item-loader-factory.h"
#include "load/item/item-loader-base.h"
#include "load/item/item-loader-version-types.h"
#include "load/load-util.h"
#include "load/old/item-loader-savefile50.h"

/*!
 * @brief アイテム読み込みクラスを返却する.
 * @return アイテム読み込みクラスへの参照ポインタ.
 * @details ItemLoaderBaseは純粋仮想関数を含むので参照を返す必要がある.
 * (値を返す設計はコンパイルエラー)
 */
std::shared_ptr<ItemLoaderBase> ItemLoaderFactory::create_loader()
{
    auto version = get_version();
    switch (version) {
    case ItemLoaderVersionType::LOAD50:
        return std::make_shared<ItemLoader50>();
    case ItemLoaderVersionType::LOAD51:
        // dummy yet.
    default:
        throw("Invalid loader version was specified!");
    }
}

/*!
 * @brief ItemLoaderのバージョン切り替え.
 * @return セーブファイルバージョン群の中で互換性のある最古のバージョン.
 * @details (備忘録)例えばバージョン15で更に変更された場合、以下のように書き換えること.
 *
 * if (loading_savefile_version_is_older_than(15)) {
 *   return ItemLoaderVersionType::LOAD11;
 * } else if (loading_savefile_version_is_older_than(11)) {
 *   return ItemLoaderVersionType::LOAD10;
 * } else {
 *   return ItemLoaderVersionType::LOAD15;
 * }
 */
ItemLoaderVersionType ItemLoaderFactory::get_version()
{
    if (loading_savefile_version_is_older_than(51)) {
        return ItemLoaderVersionType::LOAD50;
    } else {
        return ItemLoaderVersionType::LOAD51;
    }
}
