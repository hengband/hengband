/*!
 * @brief ダンジョンにおける集合論的モデルの実装
 * @author Hourier
 * @date 2024/12/01
 */

#include "system/dungeon/dungeon-list.h"
#include "system/angband-exceptions.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "util/enum-converter.h"
#include <fmt/format.h>

DungeonList DungeonList::instance{};

DungeonList &DungeonList::get_instance()
{
    return instance;
}

const DungeonDefinition &DungeonList::get_dungeon(DungeonId dungeon_id) const
{
    this->validate_dungeon_id(dungeon_id);
    return *this->dungeons.at(dungeon_id);
}

const std::shared_ptr<const DungeonDefinition> DungeonList::get_dungeon_shared(DungeonId dungeon_id) const
{
    this->validate_dungeon_id(dungeon_id);
    return this->dungeons.at(dungeon_id);
}

void DungeonList::emplace(DungeonId dungeon_id, DungeonDefinition &&definition)
{
    this->dungeons.emplace(dungeon_id, std::make_shared<DungeonDefinition>(std::move(definition)));
}

/*!
 * @brief ダンジョン情報の読み込みが終わった後の設定を行う
 */
void DungeonList::retouch()
{
    for (auto &[_, dungeon] : this->dungeons) {
        dungeon->set_guardian_flag();
        dungeon->set_no_vault_flag_if_smallest();
    }
}

void DungeonList::validate_dungeon_id(DungeonId dungeon_id) const
{
    if ((dungeon_id < DungeonId::WILDERNESS) || dungeon_id >= DungeonId::MAX) {
        THROW_EXCEPTION(std::out_of_range, fmt::format("Invalid Dungeon ID: {}", enum2i(dungeon_id)));
    }
}
