/*!
 * @file unix-music.cpp
 * @brief unix用のBGM処理
 * @details "./playmusic.sh"に音楽のファイルパスを渡し､signalを送ることで制御する｡
 *  シェルは/hengband/直下を想定｡
 *
 *  playmusic.shの実装例(ffmpeg)
 *  #!/usr/bin/bash
 *
 *  FILE="$1"
 *  # ファイルがあるか確認
 *  if [ ! -f "$FILE" ]; then
 *      echo "Error: File '$FILE' not found."
 *      exit 1
 *  fi
 *
 *  #BGMをループ再生
 *  ffplay -nodisp -autoexit -loop 0 -volume 100 "$FILE" >/dev/null 2>&1 &
 *  #プロセスIDを取得
 *  PID="$!"
 *  # signalのスクリプト側処理
 *  trap "kill $PID" SIGINT SIGTERM
 *  trap "kill -STOP $PID" SIGSTOP
 *  trap "kill -CONT $PID" SIGCONT
 *  # BGMが止まるまで待機
 *  wait "$PID"
 */

#include "main-unix/unix-music.h"
#include "dungeon/quest.h"
#include "main-unix/unix-cfg-reader.h"
#include "main/music-definitions-table.h"
#include "main/scene-table.h"
#include "main/sound-of-music.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/floor/town-list.h"
#include "system/monrace/monrace-list.h"
#include "term/z-term.h"
#include "util/angband-files.h"
#include "world/world.h"
#include <algorithm>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <range/v3/all.hpp>
#include <simpleini/SimpleIni.h>
#include <spawn.h>
#include <string>
#include <sys/types.h>
#include <tl/optional.hpp>
#include <unistd.h>
#include <vector>

#include <sys/wait.h>
extern char **environ;

static int current_music_type = TERM_XTRA_MUSIC_MUTE;
static int current_music_id = 0;
// current filename being played
static std::filesystem::path current_music_path;

static pid_t music_player_pid = 0;

/*
 * Directory name
 */
static std::filesystem::path ANGBAND_DIR_XTRA_MUSIC;

/*
 * "music.cfg" data
 */
static tl::optional<CfgData> music_cfg_data;

namespace unix_music {

/*!
 * @brief action-valに対応する[Basic]セクションのキー名を取得する
 * @param index "term_xtra()"の第2引数action-valに対応する値
 * @param buf 使用しない
 * @return 対応するキー名を返す
 */
static tl::optional<std::string> basic_key_at(int index)
{
    if (index >= MUSIC_BASIC_MAX) {
        return tl::nullopt;
    }

    return angband_music_basic_name[index];
}

/*!
 * @brief action-valに対応する[Dungeon]セクションのキー名を取得する
 * @param index "term_xtra()"の第2引数action-valに対応する値
 * @param buf バッファ
 * @return 対応するキー名を返す
 */
static tl::optional<std::string> dungeon_key_at(int index)
{
    if (index >= static_cast<int>(DungeonList::get_instance().size())) {
        return tl::nullopt;
    }

    return format("dungeon%03d", index);
}

/*!
 * @brief action-valに対応する[Quest]セクションのキー名を取得する
 * @param index "term_xtra()"の第2引数action-valに対応する値
 * @param buf バッファ
 * @return 対応するキー名を返す
 */
static tl::optional<std::string> quest_key_at(int index)
{
    const auto &quests = QuestList::get_instance();
    if (index > enum2i(quests.rbegin()->first)) {
        return tl::nullopt;
    }

    return format("quest%03d", index);
}

/*!
 * @brief action-valに対応する[Town]セクションのキー名を取得する
 * @param index "term_xtra()"の第2引数action-valに対応する値
 * @param buf バッファ
 * @return 対応するキー名を返す
 */
static tl::optional<std::string> town_key_at(int index)
{
    if (index >= static_cast<int>(TownList::get_instance().size())) {
        return tl::nullopt;
    }

    return format("town%03d", index);
}

/*!
 * @brief action-valに対応する[Monster]セクションのキー名を取得する
 * @param index "term_xtra()"の第2引数action-valに対応する値
 * @param buf バッファ
 * @return 対応するキー名を返す
 */
static tl::optional<std::string> monster_key_at(int index)
{
    if (index >= static_cast<int>(MonraceList::get_instance().size())) {
        return tl::nullopt;
    }

    return format("monster%04d", index);
}

/*!
 * @brief BGMの設定を読み込む。
 * @details
 * "music_debug.cfg"ファイルを優先して読み込む。無ければ"music.cfg"ファイルを読み込む。
 * この処理は複数回実行されることを想定していない。複数回実行した場合には前回読み込まれた設定のメモリは解放されない。
 */
void load_music_prefs()
{
    CfgReader reader(ANGBAND_DIR_XTRA_MUSIC, { "music_debug.cfg", "music.cfg" });

    // clang-format off
    music_cfg_data = reader.read_sections({
        { "Basic", TERM_XTRA_MUSIC_BASIC, basic_key_at },
        { "Dungeon", TERM_XTRA_MUSIC_DUNGEON, dungeon_key_at },
        { "Quest", TERM_XTRA_MUSIC_QUEST, quest_key_at },
        { "Town", TERM_XTRA_MUSIC_TOWN, town_key_at },
        { "Monster", TERM_XTRA_MUSIC_MONSTER, monster_key_at, &has_monster_music }
        });
    // clang-format on

    if (!has_monster_music) {
        int type = TERM_XTRA_MUSIC_BASIC;
        for (int val = MUSIC_BASIC_UNIQUE; val <= MUSIC_BASIC_HIGHER_LEVEL_MONSTER; val++) {
            if (music_cfg_data->has_key(type, val)) {
                has_monster_music = true;
                break;
            }
        }
    }
}

void init_music(std::filesystem::path music_path)
{
    ANGBAND_DIR_XTRA_MUSIC = music_path;
    load_music_prefs();
    return;
}

bool is_music_player_exist()
{
    if (music_player_pid > 0) {
        return kill(music_player_pid, 0) == 0;
    }
    return false;
}

/*
 * Stop a music
 */
void stop_music()
{
    auto status = 0;
    if (!is_music_player_exist()) {
        return;
    }

    kill(music_player_pid, SIGTERM);
    waitpid(music_player_pid, &status, 0);
    music_player_pid = 0;
    current_music_type = TERM_XTRA_MUSIC_MUTE;
    current_music_id = 0;
    current_music_path = "";
}

/*
 * Play a music
 */
bool play_music(int type, int val)
{
    if (type == TERM_XTRA_MUSIC_MUTE) {
        stop_music();
        return true;
    }

    if (current_music_type == type && current_music_id == val) {
        return true;
    } // now playing

    if (!music_cfg_data) {
        return false;
    }

    auto filename = music_cfg_data->get_rand(type, val);
    if (!filename) {
        return false;
    } // no setting

    auto path_music = path_build(ANGBAND_DIR_XTRA_MUSIC, *filename);
    if (current_music_type != TERM_XTRA_MUSIC_MUTE) {
        if (current_music_path == path_music) {
            return true;
        }
    } // now playing same file

    current_music_type = type;
    current_music_id = val;
    current_music_path = path_music;

    // kill the player when change music
    if (music_player_pid) {
        stop_music();
    }

    const std::string_view music_player = "./playmusic.sh";
    auto argv_music_player_buf = music_player | ranges::to_vector;
    argv_music_player_buf.push_back('\0');
    auto argv_path_music_buf = path_music.string() | ranges::to_vector;
    argv_path_music_buf.push_back('\0');
    //  argv must has null end
    char *const argv[] = { argv_music_player_buf.data(), argv_path_music_buf.data(), nullptr };
    // explicitly pass environment
    char **envp = environ;
    auto ret = posix_spawnp(&music_player_pid, music_player.data(), nullptr, nullptr, argv, envp);
    if (ret != 0) { // failed to spawn process
        music_player_pid = 0;
        return false;
    }
    if (kill(music_player_pid, 0) == -1) { // Test signal 0 (no action)
        music_player_pid = 0;
        return false;
    }
    return true;
}

/*
 * Pause a music
 */
void pause_music(void)
{
    if (!is_music_player_exist()) {
        return;
    }
    kill(music_player_pid, SIGSTOP);
}

/*
 * Resume a music
 */
void resume_music(void)
{
    if (!is_music_player_exist()) {
        return;
    }
    kill(music_player_pid, SIGCONT);
}

/*
 * Play a music matches a situation
 */
void play_music_scene(int val)
{
    // リストの先頭から順に再生を試み、再生できたら抜ける
    auto &list = get_scene_type_list(val);
    for (auto &item : list) {
        if (play_music(item.type, item.val)) {
            break;
        }
    }
}
}
