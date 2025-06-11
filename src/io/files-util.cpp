/*!
 * @brief ファイル入出力管理 / Purpose: code dealing with files (and death)
 * @date 2014/01/28
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.\n
 * </pre>
 */

#include "io/files-util.h"
#include "core/asking-player.h"
#include "io-dump/character-dump.h"
#include "io/input-key-acceptor.h"
#include "io/uid-checker.h"
#include "system/angband-exceptions.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/angband-files.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <algorithm>
#ifdef SAVEFILE_USE_UID
#include "main-unix/unix-user-ids.h"
#endif

std::filesystem::path ANGBAND_DIR; //!< Path name: The main "lib" directory This variable is not actually used anywhere in the code
std::filesystem::path ANGBAND_DIR_APEX; //!< High score files (binary) These files may be portable between platforms
std::filesystem::path ANGBAND_DIR_BONE; //!< Bone files for player ghosts (ascii) These files are portable between platforms
std::filesystem::path ANGBAND_DIR_DATA; //!< Binary image files for the "*_info" arrays (binary) These files are not portable between platforms
std::filesystem::path ANGBAND_DIR_EDIT; //!< Textual template files for the "*_info" arrays (ascii) These files are portable between platforms
std::filesystem::path ANGBAND_DIR_SCRIPT; //!< Script files These files are portable between platforms.
std::filesystem::path ANGBAND_DIR_FILE; //!< Various extra files (ascii) These files may be portable between platforms
std::filesystem::path ANGBAND_DIR_HELP; //!< Help files (normal) for the online help (ascii) These files are portable between platforms
std::filesystem::path ANGBAND_DIR_INFO; //!< Help files (spoilers) for the online help (ascii) These files are portable between platforms
std::filesystem::path ANGBAND_DIR_PREF; //!< Default user "preference" files (ascii) These files are rarely portable between platforms
std::filesystem::path ANGBAND_DIR_SAVE; //!< Savefiles for current characters (binary)
std::filesystem::path ANGBAND_DIR_USER; //!< User "preference" files (ascii) These files are rarely portable between platforms
std::filesystem::path ANGBAND_DIR_XTRA; //!< Various extra files (binary) These files are rarely portable between platforms

std::filesystem::path savefile;
std::filesystem::path savefile_base;

/*!
 * @brief プレイヤーステータスをファイルダンプ出力する
 * Hack -- Dump a character description file
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param name 出力ファイル名
 * @return エラーコード
 * @details
 * Allow the "full" flag to dump additional info,
 * and trigger its usage from various places in the code.
 */
void file_character(PlayerType *player_ptr, std::string_view filename)
{
    const auto path = path_build(ANGBAND_DIR_USER, filename);
    auto fd = fd_open(path, O_RDONLY);
    if (fd >= 0) {
        const auto &path_str = path.string();
        std::stringstream ss;
        ss << _("現存するファイル ", "Replace existing file ") << path_str << _(" に上書きしますか? ", "? ");
        (void)fd_close(fd);
        if (input_check_strict(player_ptr, ss.str(), UserCheck::NO_HISTORY)) {
            fd = -1;
        } else {
            return;
        }
    }

    FILE *fff = nullptr;
    if (fd < 0) {
        fff = angband_fopen(path, FileOpenMode::WRITE);
    }

    constexpr auto error_msg = _("キャラクタ情報のファイルへの書き出しに失敗しました！", "Character dump failed!");
    if (!fff) {
        msg_print(error_msg);
        msg_erase();
        return;
    }

    screen_save();
    make_character_dump(player_ptr, fff);
    screen_load();

    if (ferror(fff)) {
        angband_fclose(fff);
        msg_print(error_msg);
        msg_erase();
        return;
    }

    angband_fclose(fff);
    msg_print(_("キャラクタ情報のファイルへの書き出しに成功しました。", "Character dump successful."));
    msg_erase();
}

/*!
 * @brief ファイルからランダムに行を一つ取得する
 * @param file_name ファイル名
 * @param entry 特定条件時のN:タグヘッダID
 * @return ファイルから取得した行 (但しファイルがなかったり異常値ならばnullopt)
 */
tl::optional<std::string> get_random_line(concptr file_name, int entry)
{
    const auto path = path_build(ANGBAND_DIR_FILE, file_name);
    auto *fp = angband_fopen(path, FileOpenMode::READ);
    if (!fp) {
        return tl::nullopt;
    }

    int test;
    auto line_num = 0;
    const auto monrace_id = i2enum<MonraceId>(entry);
    const auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);
    while (true) {
        const auto line_str = angband_fgets(fp);
        if (!line_str) {
            angband_fclose(fp);
            return tl::nullopt;
        }
        const auto *buf = line_str->data();

        line_num++;
        if ((buf[0] != 'N') || (buf[1] != ':')) {
            continue;
        }

        if (buf[2] == '*') {
            break;
        }

        if (buf[2] == 'M') {
            if (monrace.is_male()) {
                break;
            }
        } else if (buf[2] == 'F') {
            if (monrace.is_female()) {
                break;
            }
        } else if (sscanf(&(buf[2]), "%d", &test) != EOF) {
            if (test == entry) {
                break;
            }
        } else {
            msg_format("Error in line %d of %s!", line_num, file_name);
            angband_fclose(fp);
            return tl::nullopt;
        }
    }

    auto counter = 0;
    tl::optional<std::string> line{};
    while (true) {
        tl::optional<std::string> buf;
        while (true) {
            buf = angband_fgets(fp);
            if (!buf) {
                break;
            }

            if (buf->starts_with("N:")) {
                continue;
            }

            if (!buf->starts_with('#')) {
                break;
            }
        }

        if (!buf || buf->empty()) {
            break;
        }

        if (one_in_(counter + 1)) {
            line = std::move(buf);
        }

        counter++;
    }

    angband_fclose(fp);
    return line;
}

#ifdef JP
/*!
 * @brief ファイルからランダムに行を一つ取得する(日本語文字列のみ)
 * @param file_name ファイル名
 * @param entry 特定条件時のN:タグヘッダID
 * @param count 試行回数
 * @return ファイルから取得した行 (但しファイルがなかったり異常値ならばnullopt)
 * @details
 */
tl::optional<std::string> get_random_line_ja_only(concptr file_name, int entry, int count)
{
    tl::optional<std::string> line;
    for (auto i = 0; i < count; i++) {
        line = get_random_line(file_name, entry);
        if (!line) {
            return tl::nullopt;
        }

        auto is_kanji = false;
        for (const auto c : *line) {
            is_kanji |= iskanji(c);
        }

        if (is_kanji) {
            return line;
        }
    }

    return line;
}
#endif

/*!
 * @brief ファイル位置をシーク /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fd ファイルディスクリプタ
 * @param where ファイルバイト位置
 * @param flag FALSEならば現ファイルを超えた位置へシーク時エラー、TRUEなら足りない間を0で埋め尽くす
 * @return エラーコード
 * @details
 */
static errr counts_seek(PlayerType *player_ptr, int fd, uint32_t where, bool flag)
{
    char temp1[128]{}, temp2[128]{};
    auto short_pclass = enum2i(player_ptr->pclass);
#ifdef SAVEFILE_USE_UID
    const auto user_id = UnixUserIds::get_instance().get_user_id();
    const auto header = format("%d.%s.%d%d%d", user_id, savefile_base.string().data(), short_pclass, player_ptr->ppersonality, player_ptr->age);
#else
    const auto header = format("%s.%d%d%d", savefile_base.string().data(), short_pclass, player_ptr->ppersonality, player_ptr->age);
#endif
    angband_strcpy(temp1, header, sizeof(temp1));
    for (int i = 0; temp1[i]; i++) {
        temp1[i] ^= (i + 1) * 63;
    }

    int seekpoint = 0;
    uint32_t zero_header[3] = { 0L, 0L, 0L };
    while (true) {
        if (fd_seek(fd, seekpoint + 3 * sizeof(uint32_t))) {
            return 1;
        }
        if (fd_read(fd, (char *)(temp2), sizeof(temp2))) {
            if (!flag) {
                return 1;
            }
            /* add new name */
            fd_seek(fd, seekpoint);
            fd_write(fd, (char *)zero_header, 3 * sizeof(uint32_t));
            fd_write(fd, (char *)(temp1), sizeof(temp1));
            break;
        }

        if (strcmp(temp1, temp2) == 0) {
            break;
        }

        seekpoint += 128 + 3 * sizeof(uint32_t);
    }

    return fd_seek(fd, seekpoint + where * sizeof(uint32_t));
}

/*!
 * @brief ファイル位置を読み込む
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param where ファイルバイト位置
 * @return エラーコード
 * @details
 */
uint32_t counts_read(PlayerType *player_ptr, int where)
{
    const auto path = path_build(ANGBAND_DIR_DATA, _("z_info_j.raw", "z_info.raw"));
    auto fd = fd_open(path, O_RDONLY);
    uint32_t count = 0;
    if (counts_seek(player_ptr, fd, where, false) || fd_read(fd, (char *)(&count), sizeof(uint32_t))) {
        count = 0;
    }

    (void)fd_close(fd);
    return count;
}

/*!
 * @brief ファイル位置に書き込む /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param where ファイルバイト位置
 * @param count 書き込む値
 * @return エラーコード
 * @details
 */
errr counts_write(PlayerType *player_ptr, int where, uint32_t count)
{
    const auto path = path_build(ANGBAND_DIR_DATA, _("z_info_j.raw", "z_info.raw"));
    safe_setuid_grab();
    auto fd = fd_open(path, O_RDWR);
    safe_setuid_drop();
    if (fd < 0) {
        safe_setuid_grab();
        fd = fd_make(path);
        safe_setuid_drop();
    }

    safe_setuid_grab();
    auto err = fd_lock(fd, F_WRLCK);
    safe_setuid_drop();
    if (err) {
        return 1;
    }

    counts_seek(player_ptr, fd, where, true);
    fd_write(fd, (char *)(&count), sizeof(uint32_t));
    safe_setuid_grab();
    err = fd_lock(fd, F_UNLCK);
    safe_setuid_drop();

    if (err) {
        return 1;
    }

    (void)fd_close(fd);
    return 0;
}

/*!
 * @brief 墓のアスキーアートテンプレを読み込んで画面に表示する
 */
void read_dead_file()
{
    const auto path = path_build(ANGBAND_DIR_FILE, _("dead_j.txt", "dead.txt"));
    auto *fp = angband_fopen(path, FileOpenMode::READ);
    if (!fp) {
        return;
    }

    int i = 0;
    while (true) {
        const auto buf = angband_fgets(fp);
        if (!buf) {
            break;
        }
        put_str(*buf, i++, 0);
    }

    angband_fclose(fp);
}
