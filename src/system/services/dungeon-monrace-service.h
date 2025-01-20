/*!
 * @brief ダンジョンとモンスター種族定義の両方に依存するサービス定義
 * @author Hourier
 * @date 2025/01/11
 */

#pragma once

enum class DungeonId;
enum class MonraceId : short;
class DungeonMonraceService {
public:
    DungeonMonraceService() = delete;
    static bool is_suitable_for_dungeon(DungeonId dungeon_id, MonraceId monrace_id);
};
