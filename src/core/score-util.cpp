#include "core/score-util.h"
#include "system/dungeon/dungeon-record.h"
#include "system/floor/floor-info.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "util/string-processor.h"
#include <algorithm>
#include <fmt/format.h>
#include <string_view>
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
    // 15 バイトで切ると 2 バイト文字が途中で切れるときは、前半の 1 バイトの代わりに空白を置く (z-form の「%-.15s」と同じ)
    auto name = str_substr(player.name, 0, 15);
    if (name.length() < std::min<size_t>(std::string_view(player.name).length(), 15)) {
        name.push_back(' ');
    }

    std::copy_n(name.begin(), name.length(), this->who);

#ifdef SET_UID
    const auto tmp_uid = UnixUserIds::get_instance().get_user_id();
#else
    const auto tmp_uid = 0;
#endif
    const auto uid_str = fmt::format("{:7}", static_cast<unsigned int>(tmp_uid));
    std::copy_n(uid_str.begin(), uid_str.length(), this->uid);
    this->sex[0] = player.psex ? 'm' : 'f';
    const auto prace = fmt::format("{:2}", std::min(enum2i(player.prace), MAX_RACES));
    std::copy_n(prace.begin(), prace.length(), this->p_r);
    const auto pclass = fmt::format("{:2}", enum2i(std::min(player.pclass, PlayerClassType::MAX)));
    std::copy_n(pclass.begin(), pclass.length(), this->p_c);
    const auto ppersonality = fmt::format("{:2}", static_cast<int>(std::min(player.ppersonality, MAX_PERSONALITIES)));
    std::copy_n(ppersonality.begin(), ppersonality.length(), this->p_a);
    const auto current_level = fmt::format("{:3}", std::min<ushort>(player.lev, 999));
    std::copy_n(current_level.begin(), current_level.length(), this->cur_lev);
    const auto &floor = *player.current_floor_ptr;
    const auto current_dungeon = fmt::format("{:3}", floor.dun_level);
    std::copy_n(current_dungeon.begin(), current_dungeon.length(), this->cur_dun);
    const auto max_level = fmt::format("{:3}", std::min<ushort>(player.max_plv, 999));
    std::copy_n(max_level.begin(), max_level.length(), this->max_lev);
    const auto max_dungeon = fmt::format("{:3}", DungeonRecords::get_instance().get_record(floor.dungeon_id).get_max_level());
    std::copy_n(max_dungeon.begin(), max_dungeon.length(), this->max_dun);
}
