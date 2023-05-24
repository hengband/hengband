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

#define RECORD_NAMED_PET_NAME 0
#define RECORD_NAMED_PET_UNNAME 1
#define RECORD_NAMED_PET_DISMISS 2
#define RECORD_NAMED_PET_DEATH 3
#define RECORD_NAMED_PET_MOVED 4
#define RECORD_NAMED_PET_LOST_SIGHT 5
#define RECORD_NAMED_PET_DESTROY 6
#define RECORD_NAMED_PET_EARTHQUAKE 7
#define RECORD_NAMED_PET_GENOCIDE 8
#define RECORD_NAMED_PET_WIZ_ZAP 9
#define RECORD_NAMED_PET_TELE_LEVEL 10
#define RECORD_NAMED_PET_BLAST 11
#define RECORD_NAMED_PET_HEAL_LEPER 12
#define RECORD_NAMED_PET_COMPACT 13
#define RECORD_NAMED_PET_LOSE_PARENT 14

extern bool write_level;

class PlayerType;
enum class QuestId : short;
#ifdef JP
#else
std::string get_ordinal_number_suffix(int num);
#endif
int exe_write_diary_quest(PlayerType *player_ptr, DiaryKind dk, QuestId num);
void exe_write_diary(PlayerType *player_ptr, DiaryKind dk, int num, std::string_view note = "");
