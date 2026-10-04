#include "player/process-name.h"
#include "autopick/autopick-reader-writer.h"
#include "core/asking-player.h"
#include "game-option/birth-options.h"
#include "io/files-util.h"
#include "player/player-personality.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/finalizer.h"
#include "util/string-processor.h"
#include "view/display-player-misc-info.h"
#include "world/world.h"
#include <sstream>
#ifdef SAVEFILE_USE_UID
#include "main-unix/unix-user-ids.h"
#endif

/*!
 * @brief プレイヤー名からファイル名に使う基本名を作る
 * @param name プレイヤー名
 * @return 基本名。PlayerType::base_name に収まる長さまでで、空になる場合は "PLAYER"
 * @details
 * 区切り文字 (PATH_SEP) は '_' に置き換え、印字できない文字は除く。
 * 区切り文字の直後の1文字も除かれるが、基本名はセーブファイルや設定ファイルの名前に使われるため、
 * 既存のファイルとの対応が変わらないようこの挙動は残している。
 */
std::string make_player_base_name(std::string_view name)
{
    constexpr auto max_length = sizeof(PlayerType::base_name) - 1;
    constexpr std::string_view path_sep(PATH_SEP);
    std::string base_name;
    for (size_t i = 0; i < name.length(); i++) {
        const auto c = static_cast<unsigned char>(name[i]);
#ifdef JP
        if (iskanji(c)) {
            if (i + 1 >= name.length()) {
                break;
            }

            base_name.append(name.substr(i, 2));
            i++;
            continue;
        }
#endif

#if defined(JP) && defined(SJIS)
        if (iskana(c)) {
            base_name.push_back(name[i]);
            continue;
        }
#endif

        if (name.substr(i).starts_with(path_sep)) {
            base_name.push_back('_');
            // ループの i++ と合わせて、区切り文字の直後の1文字も読み飛ばす (従来の挙動)
            i += path_sep.length();
            continue;
        }

#ifdef _WIN32
        if (angband_strchr("\"*,/:;<>?\\|", name[i])) {
            base_name.push_back('_');
            continue;
        }
#endif

        if (isprint(c)) {
            base_name.push_back(name[i]);
        }
    }

    if (base_name.empty()) {
        return "PLAYER";
    }

    return str_substr(std::move(base_name), 0, max_length);
}

/*!
 * @brief 条件式の $PLAYER で使うプレイヤー名を作る
 * @param name プレイヤー名
 * @return 空白と「[」「]」を「_」に置き換えた名前 (例: "[ Temp ]" なら "__Temp__")
 * @details
 * これらの文字は条件式の区切りに使われるため置き換える。
 * 2バイト文字の後半バイトは置き換えない (Shift_JIS では後半バイトが「[」「]」と同じ値になる文字がある)。
 */
std::string make_player_name_for_expression(std::string_view name)
{
    auto result = str_replace(name, " ", "_");
    result = str_replace(result, "[", "_");
    return str_replace(result, "]", "_");
}

/*!
 * @brief プレイヤーの名前をチェックして修正する
 * Process the player name.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param sf セーブファイル名に合わせた修正を行うならばTRUE
 * @details
 * Extract a clean "base name".
 * Build the savefile name if needed.
 */
void process_player_name(PlayerType *player_ptr, bool is_new_savefile)
{
    const auto &world = AngbandWorld::get_instance();
    char old_player_base[32] = "";
    if (world.character_generated) {
        strcpy(old_player_base, player_ptr->base_name);
    }

    const std::string_view name(player_ptr->name);
    for (size_t i = 0; i < name.length(); i++) {
#ifdef JP
        if (iskanji(name[i])) {
            i++;
            continue;
        }
#endif

        if (iscntrl(static_cast<unsigned char>(name[i]))) {
            quit_fmt(_("'%s' という名前は不正なコントロールコードを含んでいます。", "The name '%s' contains control chars!"), player_ptr->name);
        }
    }

    angband_strcpy(player_ptr->base_name, make_player_base_name(name), sizeof(player_ptr->base_name));

    auto is_modified = false;
    if (is_new_savefile && (savefile.empty() || !keep_savefile)) {
        std::stringstream ss;

#ifdef SAVEFILE_USE_UID
        ss << UnixUserIds::get_instance().get_user_id();
        ss << '.' << player_ptr->base_name;
#else
        ss << player_ptr->base_name;
#endif
        savefile = path_build(ANGBAND_DIR_SAVE, ss.str());
        is_modified = true;
    }

    if (is_modified || savefile_base.empty()) {
#ifdef SAVEFILE_USE_UID
        const auto savefile_str = savefile.filename().string();
        const auto split = str_split(savefile_str, '.');
        savefile_base = split[1];
#else
        savefile_base = savefile.filename();
#endif
    }

    if (world.character_generated && !streq(old_player_base, player_ptr->base_name)) {
        autopick_load_pref(player_ptr, false);
    }
}

/*!
 * @brief プレイヤーの名前を変更する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details PlayerType::name は32バイトで定義されているが、
 * スコアファイル（lib/apex/scores.raw）に保存されているプレイヤー名が最大16バイト (ヌル文字含)となっている
 * このため最大値を16バイトに制限する
 */
void get_name(PlayerType *player_ptr)
{
    const auto finalizer = util::make_finalizer([player_ptr]() {
        display_player_misc_info(player_ptr);
    });

    std::string initial_name(player_ptr->name);
    const auto max_name_size = 15;
    const auto copy_size = sizeof(player_ptr->name);
    constexpr auto prompt = _("キャラクターの名前を入力して下さい: ", "Enter a name for your character: ");
    const auto name = input_string(prompt, max_name_size, initial_name);
    if (name && !name->empty()) {
        angband_strcpy(player_ptr->name, *name, copy_size);
        return;
    }

    if (initial_name.empty()) {
        angband_strcpy(player_ptr->name, "PLAYER", copy_size);
    }
}
