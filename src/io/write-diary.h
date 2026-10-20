#pragma once

#include <string_view>
#ifdef JP
#else
#include <string>
#endif

enum class DiaryKind {
    DIALY,
    DESCRIPTION,
    ART,
    UNIQUE,
    FIX_QUEST_C,
    FIX_QUEST_F,
    RAND_QUEST_C,
    RAND_QUEST_F,
    MAXDEAPTH,
    TRUMP,
    STAIR,
    RECALL,
    TO_QUEST,
    TELEPORT_LEVEL,
    BUY,
    SELL,
    ARENA,
    FOUND,
    LEVELUP,
    GAMESTART,
    NAMED_PET,
    PAT_TELE,
    ART_SCROLL,
    WIZARD_LOG,
};

constexpr auto RECORD_NAMED_PET_NAME = 0;
constexpr auto RECORD_NAMED_PET_UNNAME = 1;
constexpr auto RECORD_NAMED_PET_DISMISS = 2;
constexpr auto RECORD_NAMED_PET_DEATH = 3;
constexpr auto RECORD_NAMED_PET_MOVED = 4;
constexpr auto RECORD_NAMED_PET_LOST_SIGHT = 5;
constexpr auto RECORD_NAMED_PET_DESTROY = 6;
constexpr auto RECORD_NAMED_PET_EARTHQUAKE = 7;
constexpr auto RECORD_NAMED_PET_GENOCIDE = 8;
constexpr auto RECORD_NAMED_PET_WIZ_ZAP = 9;
constexpr auto RECORD_NAMED_PET_TELE_LEVEL = 10;
constexpr auto RECORD_NAMED_PET_BLAST = 11;
constexpr auto RECORD_NAMED_PET_HEAL_LEPER = 12;
constexpr auto RECORD_NAMED_PET_COMPACT = 13;
constexpr auto RECORD_NAMED_PET_LOSE_PARENT = 14;

extern bool write_level;

class FloorType;
class PlayerType;
enum class QuestId : short;
int exe_write_diary_quest(PlayerType *player_ptr, DiaryKind dk, QuestId quest_id);
void exe_write_diary(const FloorType &floor, DiaryKind dk, int num, std::string_view note = "");
