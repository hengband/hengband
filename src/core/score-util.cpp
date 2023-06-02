#include "core/score-util.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#ifdef SET_UID
#include "main-unix/unix-user-ids.h"
#endif

/*
 * The "highscore" file descriptor, if available.
 */
int highscore_fd = -1;

/*!
 * @brief i番目のスコア情報にバッファ位置をシークする / Seek score 'i' in the highscore file
 * @param i スコア情報ID
 * @return 問題がなければ0を返す
 */
int highscore_seek(int i)
{
    return fd_seek(highscore_fd, (ulong)(i) * sizeof(high_score));
}

/*!
 * @brief 所定ポインタからスコア情報を読み取る / Read one score from the highscore file
 * @param score スコア情報参照ポインタ
 * @return エラーコード
 */
errr highscore_read(high_score *score)
{
    return fd_read(highscore_fd, (char *)(score), sizeof(high_score));
}

void high_score::copy_info(const PlayerType &player)
{
    snprintf(this->who, sizeof(this->who), "%-.15s", player.name);

#ifdef SET_UID
    const auto tmp_uid = UnixUserIds::get_instance().get_user_id();
#else
    const auto tmp_uid = 0;
#endif
    snprintf(this->uid, sizeof(this->uid), "%7u", tmp_uid);
    snprintf(this->sex, sizeof(this->sex), "%c", (player.psex ? 'm' : 'f'));
    char buf[32];
    snprintf(buf, sizeof(buf), "%2d", std::min(enum2i(player.prace), MAX_RACES));
    memcpy(this->p_r, buf, 3);
    snprintf(buf, sizeof(buf), "%2d", enum2i(std::min(player.pclass, PlayerClassType::MAX)));
    memcpy(this->p_c, buf, 3);
    snprintf(buf, sizeof(buf), "%2d", std::min(player.ppersonality, MAX_PERSONALITIES));
    memcpy(this->p_a, buf, 3);

    snprintf(this->cur_lev, sizeof(this->cur_lev), "%3d", std::min<ushort>(player.lev, 999));
    const auto &floor = *player.current_floor_ptr;
    snprintf(this->cur_dun, sizeof(this->cur_dun), "%3d", floor.dun_level);
    snprintf(this->max_lev, sizeof(this->max_lev), "%3d", std::min<ushort>(player.max_plv, 999));
    snprintf(this->max_dun, sizeof(this->max_dun), "%3d", max_dlv[floor.dungeon_idx]);
}
