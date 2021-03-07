#include "main/sound-of-music.h"
#include "dungeon/quest.h"
#include "game-option/disturbance-options.h"
#include "game-option/special-options.h"
#include "main/music-definitions-table.h"
#include "system/floor-type-definition.h"
#include "term/screen-processor.h"

/*
 * Flush the screen, make a noise
 */
void bell(void)
{
    term_fresh();
    if (ring_bell)
        term_xtra(TERM_XTRA_NOISE, 0);

    flush();
}

/*
 * todo intをsound_typeに差し替える
 * @brief 音を鳴らす
 */
void sound(int val)
{
    if (!use_sound)
        return;

    term_xtra(TERM_XTRA_SOUND, val);
}

/*
 * Hack -- Play a music
 */
errr play_music(int type, int val)
{
    if (!use_music)
        return 1;

    return term_xtra(type, val);
}

/*!
 * @brief クエストBGMを選択
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return BGMを鳴らしたらTRUE、それ以外はFALSE
 * @details
 *  v3.0.0現在、フロアクエストとはワーグクエストとランダムクエストのみ該当する。
 *  設定がない場合は鳴らさない。
 */
bool select_quest_music(player_type *player_ptr)
{
    QUEST_IDX quest_id = player_ptr->current_floor_ptr->inside_quest;

    if (quest_id > 0) {
        if (play_music(TERM_XTRA_MUSIC_QUEST, quest_id) == 0)
            return TRUE;
        return play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_QUEST) == 0;
    }

    //ダンジョン内のクエスト
    quest_id = quest_number(player_ptr, player_ptr->current_floor_ptr->dun_level);

    if (quest_id == 0)
        return FALSE;

    if (play_music(TERM_XTRA_MUSIC_QUEST, quest_id) == 0)
        return TRUE;

    return play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_QUEST) == 0;
}

/*!
 * @brief 街と荒野のBGMを選択
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return BGMを鳴らしたらTRUE、それ以外はFALSE
 * @details
 *  街にいるときは街に該当する音楽があれば鳴らす。
 *  なければ基本の街の音楽を鳴らす。
 *  荒野ではレベル45以上に該当する音楽があれば鳴らす。
 *  また25以上に該当する音楽があれば鳴らす。
 *  荒野低レベル時の音楽があれば鳴らすが、なければ鳴らさない。
 */
bool select_wilderness_music(player_type *player_ptr)
{

    if (player_ptr->town_num) {
        if (play_music(TERM_XTRA_MUSIC_TOWN, player_ptr->town_num) == 0)
            return TRUE;
        return play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_TOWN) == 0;
    }

    if (player_ptr->lev >= 45)
        if (play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_FIELD3) == 0)
            return TRUE;
    if (player_ptr->lev >= 25)
        if (play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_FIELD2) == 0)
            return TRUE;

    return play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_FIELD1) == 0;
}

/*!
 * @brief ダンジョンBGMを選択
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return BGMを鳴らしたらTRUE、それ以外はFALSE
 * @details
 *  階の雰囲気に該当する音楽があれば鳴らす。
 *  ダンジョンの種類に該当する音楽があれば鳴らす。
 *  80階以上に該当する音楽があれば鳴らす。
 *  40階以上に該当する音楽があれば鳴らす。
 *  それ以外の場合、浅い階に該当する音楽があれば鳴らし、なければ鳴らさない。
 */
bool select_dungeon_music(player_type *player_ptr)
{

    if (player_ptr->feeling == 2)
        if (play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_DUN_FEEL2) == 0)
            return TRUE;
    if (player_ptr->feeling >= 3 && player_ptr->feeling <= 5)
        if (play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_DUN_FEEL1) == 0)
            return TRUE;

    if (player_ptr->dungeon_idx)
        if (play_music(TERM_XTRA_MUSIC_DUNGEON, player_ptr->dungeon_idx) == 0)
            return TRUE;

    if (player_ptr->current_floor_ptr->dun_level >= 80)
        if (play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_DUN_HIGH) == 0)
            return TRUE;
    if (player_ptr->current_floor_ptr->dun_level >= 40)
        if (play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_DUN_MED) == 0)
            return TRUE;

    return play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_DUN_LOW) == 0;
}

/*!
 * @brief BGMの選択実行部
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return BGMを鳴らしたらTRUE、それ以外はFALSE
 * @details
 *  設定がない場合は鳴らさない。
 *  クエストBGMがない場合は階のBGM。
 */
bool exe_select_floor_music(player_type *player_ptr)
{
    if (player_ptr->ambush_flag)
        return play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_AMBUSH) == 0;

    if (player_ptr->wild_mode)
        return play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_WILD) == 0;

    if (player_ptr->current_floor_ptr->inside_arena)
        return play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_ARENA) == 0;

    if (player_ptr->phase_out)
        return play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_BATTLE) == 0;

    if (select_quest_music(player_ptr))
        return TRUE;

    if (player_ptr->current_floor_ptr->dun_level == 0)
        return select_wilderness_music(player_ptr);

    return select_dungeon_music(player_ptr);
}

/*!
 * @brief BGMの選択
 * @param player_ptr プレーヤーへの参照ポインタ
 * @details 設定がない場合はミュートする。
 */
void select_floor_music(player_type *player_ptr)
{
    if (!use_music)
        return;

    if (exe_select_floor_music(player_ptr))
        return;

    play_music(TERM_XTRA_MUSIC_MUTE, 0);
}
