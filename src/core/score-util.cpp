#include "core/score-util.h"
#include "player/player-status.h"
#include "system/angband-version.h"
#include "system/dungeon/dungeon-record.h"
#include "system/floor/floor-info.h"
#include "system/inner-game-data.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "world/world.h"
#include <algorithm>
#include <cstdlib>
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

/*!
 * @brief プレイヤーの今の状態からスコアの記録を作る
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return スコアの記録。日付と死因は呼び出し元で設定する
 */
HighScore HighScore::create(PlayerType *player_ptr)
{
    HighScore score;
    score.version = fmt::format("{}.{}.{}", H_VER_MAJOR, H_VER_MINOR, H_VER_PATCH);
    score.points = calc_score(player_ptr);
    score.gold = player_ptr->au;
    score.turns = InnerGameData::get_instance().get_real_turns(AngbandWorld::get_instance().game_turn);
    score.name = player_ptr->name;
#ifdef SET_UID
    score.uid = UnixUserIds::get_instance().get_user_id();
#endif
    score.sex = player_ptr->psex ? "m" : "f";
    score.race = std::min(enum2i(player_ptr->prace), MAX_RACES);
    score.pclass = enum2i(std::min(player_ptr->pclass, PlayerClassType::MAX));
    score.personality = static_cast<int>(std::min(player_ptr->ppersonality, MAX_PERSONALITIES));
    score.level = std::min<int>(player_ptr->lev, 999);
    const auto &floor = *player_ptr->current_floor_ptr;
    score.dungeon_level = floor.dun_level;
    score.max_level = std::min<int>(player_ptr->max_plv, 999);
    score.max_dungeon_level = DungeonRecords::get_instance().get_record(floor.dungeon_id).get_max_level();
    return score;
}

/*!
 * @brief スコアの記録を scores.raw の 1 件の形にする
 * @param score スコアの記録
 * @return scores.raw の 1 件。数値の欄は右寄せで空白を詰め、欄に入りきらない文字列は切る
 */
high_score high_score::from_score(const HighScore &score)
{
    high_score raw{};
    angband_strcpy(raw.what, score.version, sizeof(raw.what));
    angband_strcpy(raw.pts, fmt::format("{:9}", score.points), sizeof(raw.pts));
    angband_strcpy(raw.gold, fmt::format("{:9}", score.gold), sizeof(raw.gold));
    angband_strcpy(raw.turns, fmt::format("{:9}", score.turns), sizeof(raw.turns));
    angband_strcpy(raw.day, score.date, sizeof(raw.day));

    // 15 バイトで切ると 2 バイト文字が途中で切れるときは、前半の 1 バイトの代わりに空白を置く
    auto name = str_substr(score.name, 0, 15);
    if (name.length() < std::min<size_t>(score.name.length(), 15)) {
        name.push_back(' ');
    }

    std::copy_n(name.begin(), name.length(), raw.who);
    const auto uid = fmt::format("{:7}", static_cast<unsigned int>(score.uid));
    std::copy_n(uid.begin(), uid.length(), raw.uid);
    if (!score.sex.empty()) {
        raw.sex[0] = score.sex.front();
    }

    const auto copy_number = [](int value, size_t width, char *field) {
        const auto str = fmt::format("{:{}}", value, width);
        std::copy_n(str.begin(), str.length(), field);
    };
    copy_number(score.race, 2, raw.p_r);
    copy_number(score.pclass, 2, raw.p_c);
    copy_number(score.personality, 2, raw.p_a);
    copy_number(score.level, 3, raw.cur_lev);
    copy_number(score.dungeon_level, 3, raw.cur_dun);
    copy_number(score.max_level, 3, raw.max_lev);
    copy_number(score.max_dungeon_level, 3, raw.max_dun);

    if (score.died_from.size() >= sizeof(raw.how)) {
#ifdef JP
        angband_strcpy(raw.how, score.died_from, sizeof(raw.how) - 2);
        angband_strcat(raw.how, "…", sizeof(raw.how));
#else
        angband_strcpy(raw.how, score.died_from, sizeof(raw.how) - 3);
        angband_strcat(raw.how, "...", sizeof(raw.how));
#endif
    } else {
        angband_strcpy(raw.how, score.died_from, sizeof(raw.how));
    }

    return raw;
}

/*!
 * @brief scores.raw の 1 件をスコアの記録にする
 * @return スコアの記録。数値の欄は先頭の空白を除いて読む
 */
HighScore high_score::to_score() const
{
    const auto field = []<size_t N>(const char(&chars)[N]) {
        return std::string(chars, std::find(chars, chars + N, '\0'));
    };
    const auto number = [&field]<size_t N>(const char(&chars)[N]) {
        return std::atoll(field(chars).data());
    };
    const auto small_number = [&number]<size_t N>(const char(&chars)[N]) {
        return static_cast<int>(number(chars));
    };

    HighScore score;
    score.version = field(this->what);
    score.points = number(this->pts);
    score.gold = number(this->gold);
    score.turns = number(this->turns);
    score.date = field(this->day);
    score.name = field(this->who);
    score.uid = number(this->uid);
    score.sex = field(this->sex);
    score.race = small_number(this->p_r);
    score.pclass = small_number(this->p_c);
    score.personality = small_number(this->p_a);
    score.level = small_number(this->cur_lev);
    score.dungeon_level = small_number(this->cur_dun);
    score.max_level = small_number(this->max_lev);
    score.max_dungeon_level = small_number(this->max_dun);
    score.died_from = field(this->how);
    return score;
}
