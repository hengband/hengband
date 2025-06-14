/*!
 * @brief ダンジョンの定義と記録に関するサービス処理定義
 * @author Hourier
 * @date 2024/12/28
 * @details サービスクラスはステートレスであるべきなのでメソッドは全てstatic である
 */

#pragma once

#include <memory>
#include <string>
#include <tl/optional.hpp>
#include <vector>

enum class DungeonMessageFormat {
    DUMP,
    KNOWLEDGE,
    RECALL,
};

enum class DungeonId;
class DungeonDefinition;
class DungeonRecord;
class DungeonService {
public:
    DungeonService() = delete;
    static int decide_gradiator_level();
    static int find_max_level();
    static tl::optional<std::string> check_first_entrance(DungeonId dungeon_id);
    static std::vector<std::string> build_known_dungeons(DungeonMessageFormat dmf);
};
