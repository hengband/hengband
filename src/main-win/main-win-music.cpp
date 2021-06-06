﻿/*!
 * @file main-win-music.cpp
 * @brief Windows版固有実装(BGM)
 */

#include "main-win/main-win-music.h"
#include "dungeon/quest.h"
#include "floor/floor-town.h"
#include "main-win/audio-win.h"
#include "main-win/main-win-define.h"
#include "main-win/main-win-file-utils.h"
#include "main-win/main-win-tokenizer.h"
#include "main/scene-table.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race.h"
#include "term/z-term.h"
#include "util/angband-files.h"
#include "world/world.h"

bool use_pause_music_inactive = false;
static int current_music_type = TERM_XTRA_MUSIC_MUTE;
static int current_music_id = 0;
// current filename being played
static char current_music_path[MAIN_WIN_MAX_PATH];

/*
 * Directory name
 */
concptr ANGBAND_DIR_XTRA_MUSIC;

/*
 * "music.cfg" data
 */
CfgData *music_cfg_data;

namespace main_win_music {

/*!
 * @brief action-valに対応する[Basic]セクションのキー名を取得する
 * @param index "term_xtra()"の第2引数action-valに対応する値
 * @param buf 使用しない
 * @return 対応するキー名を返す
 */
static concptr basic_key_at(int index, char *buf)
{
    (void)buf;

    if (index >= MUSIC_BASIC_MAX)
        return NULL;

    return angband_music_basic_name[index];
}

static inline DUNGEON_IDX get_dungeon_count()
{
    return current_world_ptr->max_d_idx;
}

/*!
 * @brief action-valに対応する[Dungeon]セクションのキー名を取得する
 * @param index "term_xtra()"の第2引数action-valに対応する値
 * @param buf バッファ
 * @return 対応するキー名を返す
 */
static concptr dungeon_key_at(int index, char *buf)
{
    if (index >= get_dungeon_count())
        return NULL;

    sprintf(buf, "dungeon%03d", index);
    return buf;
}

static inline QUEST_IDX get_quest_count()
{
    return max_q_idx;
}

/*!
 * @brief action-valに対応する[Quest]セクションのキー名を取得する
 * @param index "term_xtra()"の第2引数action-valに対応する値
 * @param buf バッファ
 * @return 対応するキー名を返す
 */
static concptr quest_key_at(int index, char *buf)
{
    if (index >= get_quest_count())
        return NULL;

    sprintf(buf, "quest%03d", index);
    return buf;
}

static inline TOWN_IDX get_town_count()
{
    return max_towns;
}

/*!
 * @brief action-valに対応する[Town]セクションのキー名を取得する
 * @param index "term_xtra()"の第2引数action-valに対応する値
 * @param buf バッファ
 * @return 対応するキー名を返す
 */
static concptr town_key_at(int index, char *buf)
{
    if (index >= get_town_count())
        return NULL;

    sprintf(buf, "town%03d", index);
    return buf;
}

static inline MONRACE_IDX get_monster_count()
{
    return max_r_idx;
}

/*!
 * @brief action-valに対応する[Monster]セクションのキー名を取得する
 * @param index "term_xtra()"の第2引数action-valに対応する値
 * @param buf バッファ
 * @return 対応するキー名を返す
 */
static concptr monster_key_at(int index, char *buf)
{
    if (index >= get_monster_count())
        return NULL;

    sprintf(buf, "monster%04d", index);
    return buf;
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

/*
 * Stop a music
 */
errr stop_music(void)
{
    current_music_type = TERM_XTRA_MUSIC_MUTE;
    current_music_id = 0;
    strcpy(current_music_path, "\0");
    stop_music_queue();
    return 0;
}

/*
 * Play a music
 */
errr play_music(int type, int val)
{
    if (!can_audio())
        return 1;

    if (type == TERM_XTRA_MUSIC_MUTE)
        return stop_music();

    if (current_music_type == type && current_music_id == val)
        return 0; // now playing

    concptr filename = music_cfg_data->get_rand(type, val);
    if (!filename)
        return 1; // no setting

    char buf[MAIN_WIN_MAX_PATH];
    path_build(buf, MAIN_WIN_MAX_PATH, ANGBAND_DIR_XTRA_MUSIC, filename);

    if (current_music_type != TERM_XTRA_MUSIC_MUTE)
        if (0 == strcmp(current_music_path, buf))
            return 0; // now playing same file

    current_music_type = type;
    current_music_id = val;
    strcpy(current_music_path, buf);

    if (add_music_queue(buf)) {
        return 0;
    } else {
        return 1;
    }
}

/*
 * Pause a music
 */
void pause_music(void)
{
    pause_music_queue();
}

/*
 * Resume a music
 */
void resume_music(void)
{
    resume_music_queue();
}

/*
 * Play a music matches a situation
 */
errr play_music_scene(int val)
{
    // リストの先頭から順に再生を試み、再生できたら抜ける
    auto &list = get_scene_type_list(val);
    const errr err_sucsess = 0;
    for (auto &item : list) {
        if (play_music(item.type, item.val) == err_sucsess) {
            break;
        }
    }

    return 0;
}

}
