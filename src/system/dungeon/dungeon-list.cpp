/*!
 * @brief ダンジョンにおける集合論的モデルの実装
 * @author Hourier
 * @date 2024/12/01
 */

#include "system/dungeon/dungeon-list.h"
#include "system/dungeon/dungeon-definition.h"

DungeonList DungeonList::instance{};

DungeonList &DungeonList::get_instance()
{
    return instance;
}

DungeonDefinition &DungeonList::get_dungeon(DungeonId dungeon_id)
{
    return *this->dungeons.at(dungeon_id);
}

const DungeonDefinition &DungeonList::get_dungeon(DungeonId dungeon_id) const
{
    return *this->dungeons.at(dungeon_id);
}

std::shared_ptr<DungeonDefinition> DungeonList::get_dungeon_shared(DungeonId dungeon_id)
{
    return this->dungeons.at(dungeon_id);
}

std::shared_ptr<const DungeonDefinition> DungeonList::get_dungeon_shared(DungeonId dungeon_id) const
{
    return this->dungeons.at(dungeon_id);
}

std::map<DungeonId, std::shared_ptr<DungeonDefinition>>::iterator DungeonList::begin()
{
    return this->dungeons.begin();
}

std::map<DungeonId, std::shared_ptr<DungeonDefinition>>::const_iterator DungeonList::begin() const
{
    return this->dungeons.cbegin();
}

std::map<DungeonId, std::shared_ptr<DungeonDefinition>>::iterator DungeonList::end()
{
    return this->dungeons.end();
}

std::map<DungeonId, std::shared_ptr<DungeonDefinition>>::const_iterator DungeonList::end() const
{
    return this->dungeons.cend();
}

std::map<DungeonId, std::shared_ptr<DungeonDefinition>>::reverse_iterator DungeonList::rbegin()
{
    return this->dungeons.rbegin();
}

std::map<DungeonId, std::shared_ptr<DungeonDefinition>>::const_reverse_iterator DungeonList::rbegin() const
{
    return this->dungeons.crbegin();
}

std::map<DungeonId, std::shared_ptr<DungeonDefinition>>::reverse_iterator DungeonList::rend()
{
    return this->dungeons.rend();
}

std::map<DungeonId, std::shared_ptr<DungeonDefinition>>::const_reverse_iterator DungeonList::rend() const
{
    return this->dungeons.crend();
}

size_t DungeonList::size() const
{
    return this->dungeons.size();
}

bool DungeonList::empty() const
{
    return this->dungeons.empty();
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
    }
}
