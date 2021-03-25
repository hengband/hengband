#include "main/sound-of-music.h"
#include "game-option/disturbance-options.h"
#include "game-option/special-options.h"
#include "main/scene-table.h"
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

/*!
 * @brief 音を鳴らす
 * @todo intをsound_typeに差し替える
 */
void sound(int val)
{
    if (!use_sound)
        return;

    term_xtra(TERM_XTRA_SOUND, val);
}

/*!
 * @brief Hack -- Play a music
 */
errr play_music(int type, int val)
{
    if (!use_music)
        return 1;

    return term_xtra(type, val);
}

/*!
 * @brief シチュエーションに合ったBGM選曲
 * @param player_ptr プレーヤーへの参照ポインタ
 * @details 設定がない場合はミュートする。
 */
void select_floor_music(player_type *player_ptr)
{
    if (!use_music)
        return;

    refresh_scene_table(player_ptr);
    play_music(TERM_XTRA_SCENE, 0);
}
