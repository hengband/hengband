#include "core/score-file.h"
#include "io/files-util.h"
#include "io/uid-checker.h"
#include "locale/character-encoding.h"
#include "player-info/class-types.h"
#include "player-info/race-types.h"
#include "player/player-personality-types.h"
#include "util/angband-files.h"
#include "util/string-processor.h"
#include <algorithm>
#include <filesystem>
#include <fmt/format.h>
#include <functional>
#include <nlohmann/json.hpp>
#include <system_error>
#include <utility>

namespace {
constexpr auto SCORE_FILE_VERSION = 1;
constexpr std::string_view SCORE_FILE_NAME = "scores.json";
constexpr std::string_view OLD_SCORE_FILE_NAME = "scores.raw";

//! 別のユーザーのゲームも読み書きできるよう、ゲームが作ったスコアファイルに付ける権限 (0664)
constexpr auto SHARED_FILE_PERMISSION = std::filesystem::perms::owner_read | std::filesystem::perms::owner_write | std::filesystem::perms::group_read |
                                        std::filesystem::perms::group_write | std::filesystem::perms::others_read;

/*!
 * @brief lib/apex のファイルのパスを求める
 * @param name ファイル名
 * @return パス。fd_open() と同じく path_parse() で解決する ("~" で始まるディレクトリを展開する)
 */
std::filesystem::path score_file_path(std::string_view name)
{
    return path_parse(path_build(ANGBAND_DIR_APEX, name));
}

/*!
 * @brief ファイルがあるかを調べる
 * @param path ファイルのパス
 * @return あれば true、無ければ false。グループにしか読めないディレクトリもあるので権限を上げて調べ、調べられないときは tl::nullopt
 */
tl::optional<bool> file_exists(const std::filesystem::path &path)
{
    std::error_code ec;
    safe_setuid_grab();
    const auto exists = std::filesystem::exists(path, ec);
    safe_setuid_drop();
    if (ec) {
        return tl::nullopt;
    }

    return exists;
}

/*!
 * @brief 表示で範囲外を読まない記録か
 * @param score 記録
 * @return 種族・職業・性格が範囲内なら true
 */
bool is_valid_score(const HighScore &score)
{
    const auto in_range = [](int value, int max) { return (0 <= value) && (value < max); };
    return in_range(score.race, MAX_RACES) && in_range(score.pclass, PLAYER_CLASS_TYPE_MAX) && in_range(score.personality, MAX_PERSONALITIES);
}

/*!
 * @brief 中身が空白だけか
 * @param text スコアファイルの中身
 * @return 空白だけ (空を含む) なら true。記録 0 件として読み、前の版のスコアファイルから記録を移す
 */
bool is_blank_text(std::string_view text)
{
    return text.find_first_not_of(" \t\r\n") == std::string_view::npos;
}

/*!
 * @brief ファイル記述子の指すファイルの中身を全部読む
 * @param fd ファイル記述子
 * @return 中身。読めないときは tl::nullopt
 */
tl::optional<std::string> read_whole_file(int fd)
{
    const auto size = fd_size(fd);
    if (!size || fd_seek(fd, 0)) {
        return tl::nullopt;
    }

    std::string text(*size, '\0');
    if (!text.empty() && fd_read(fd, text.data(), text.length())) {
        return tl::nullopt;
    }

    return text;
}

/*!
 * @brief ほかのゲームが同時に読み書きしないよう、スコアファイルをロックして読み書きする
 * @details
 * ロックがほかのファイル記述子を閉じたときに外れることのないよう (NFS などでは fcntl のロックで代用される)、
 * 読み書きもロックしたファイル記述子で行う。排他ロックには書き込みの権限が要る環境もあるので、読み書きの両方の権限で開く。
 */
class LockedScoreFile {
public:
    LockedScoreFile()
    {
        safe_setuid_grab();
        this->fd = fd_open(score_file_path(SCORE_FILE_NAME), O_RDWR);
        if ((this->fd >= 0) && fd_lock(this->fd, F_WRLCK)) {
            (void)fd_close(this->fd);
            this->fd = -1;
        }

        safe_setuid_drop();
    }

    ~LockedScoreFile()
    {
        // 閉じればロックも外れる
        if (this->fd >= 0) {
            (void)fd_close(this->fd);
        }
    }

    LockedScoreFile(const LockedScoreFile &) = delete;
    LockedScoreFile(LockedScoreFile &&) = delete;
    LockedScoreFile &operator=(const LockedScoreFile &) = delete;
    LockedScoreFile &operator=(LockedScoreFile &&) = delete;

    bool is_locked() const
    {
        return this->fd >= 0;
    }

    /*!
     * @brief 中身を読む
     * @return 中身。読めないときは tl::nullopt
     */
    tl::optional<std::string> read()
    {
        return read_whole_file(this->fd);
    }

    /*!
     * @brief 中身を書き換える
     * @param text 新しい中身
     * @return 書き込めたら true
     * @details
     * 前の版と同じく、所有者 (setgid でインストールしたときは root:games) を変えないよう、その場で上書きする。
     * 前の中身より短いときは、切り詰めの失敗で古い中身が残って読めなくならないよう、また環境ごとの切り詰めの関数を
     * 使わずに済むよう、空白で埋めて同じ長さにする (JSON としては同じ中身)。末尾が改行なら、その前を埋める。
     */
    bool write(std::string text)
    {
        const auto size = fd_size(this->fd);
        if (!size) {
            return false;
        }

        if (text.length() < *size) {
            const auto pos = text.ends_with('\n') ? text.length() - 1 : text.length();
            text.insert(pos, *size - text.length(), ' ');
        }

        return !fd_seek(this->fd, 0) && !fd_write(this->fd, text.data(), text.length());
    }

private:
    int fd = -1;
};

/*!
 * @brief 記録 (または scores.json の全体) の整数の欄を読む
 * @param record 記録、または scores.json の全体
 * @param key 欄の名前
 * @param value 読んだ値を入れる。欄が無ければ 0 にする
 * @return 読めたら true。整数でない、受け取る型の範囲を超えるときは false (手で編集したファイルの値が範囲内に化けないよう、丸めずに使わない)
 */
template <typename T>
bool read_integer(const nlohmann::json &record, const char *key, T &value)
{
    const auto it = record.find(key);
    if (it == record.end()) {
        value = 0;
        return true;
    }

    // is_number_integer() は符号無しの整数でも true になる
    if (!it->is_number_integer()) {
        return false;
    }

    const auto is_unsigned = it->is_number_unsigned();
    const auto fits = is_unsigned ? std::in_range<T>(it->template get<uint64_t>()) : std::in_range<T>(it->template get<int64_t>());
    if (!fits) {
        return false;
    }

    value = is_unsigned ? static_cast<T>(it->template get<uint64_t>()) : static_cast<T>(it->template get<int64_t>());
    return true;
}

/*!
 * @brief scores.raw の日付を scores.json の形にする
 * @param date scores.raw の日付
 * @return "@YYYYMMDD" は "YYYY-MM-DD" にし、それ以外 (古い版の "MM/DD/YY" など) はそのまま返す
 */
std::string migrate_date(std::string_view date)
{
    // 前の版の表示と同じく、先頭の空白を除いて読む
    const auto trimmed = str_ltrim(date);
    if ((trimmed.length() != 9) || !trimmed.starts_with('@') || (trimmed.find_first_not_of("0123456789", 1) != std::string::npos)) {
        return trimmed;
    }

    return fmt::format("{}-{}-{}", trimmed.substr(1, 4), trimmed.substr(5, 2), trimmed.substr(7, 2));
}

/*!
 * @brief scores.raw の文字列の欄を UTF-8 にする
 * @param str 文字列の欄
 * @param prefer_legacy_encoding 旧版の内部の文字コード (LEGACY_CHARACTER_ENCODING) として正しく読めるなら、文字コードを推定しないか
 * @return UTF-8 の文字列。変換できないときは "?"
 * @details
 * 日本語版では、Windows 版と Unix 系の間で受け渡したファイルのために、guess_legacy_encoding() で文字コードを推定する。
 * ただし推定は半角カナの並びなどを別の文字コードと取り違えることがあるので、名前は前の版の表示と同じく
 * 旧版の内部の文字コード (LEGACY_CHARACTER_ENCODING。Windows 版は Shift_JIS、Unix 系は EUC-JP) として正しく読めるならそのまま変換し、
 * 読めないときだけ推定する (prefer_legacy_encoding)。
 * 死因は前の版の表示と同じく先に推定する。
 *
 * 前の版は名前と死因をバイト数で切って書いたので、末尾の文字が途中で切れていることがある。
 * scores.raw は移した後は読まず、欄全体を "?" にすると元に戻せないので、末尾の半端なバイト (2 バイトまで) を落として変換する。
 * Windows 版の変換は半端なバイトを置き換えて成功してしまうので、変換の成否ではなく、
 * 文字コードを推定できたか、旧版の内部の文字コードとして末尾の文字だけが欠けているかで、落とす長さを決める。
 */
std::string migrate_text(std::string_view str, [[maybe_unused]] bool prefer_legacy_encoding)
{
    const auto max_cut = std::min(2, static_cast<int>(str.length()));
    const auto prefix = [str](int cut) { return str.substr(0, str.length() - cut); };
#ifdef JP
    // 落とすバイトが、旧版の内部の文字コードの文字の途中で切れた先頭の部分か
    // (EUC-JP は 2 バイトの文字の先頭バイトと 3 バイトの文字の先頭の 2 バイト、Shift_JIS は 2 バイトの文字の先頭バイト)
    const auto is_cut_lead = [](std::string_view dropped) {
        if (dropped.empty()) {
            return true;
        }

        // 3 バイトの文字の先頭の 2 バイトなら、2 バイト目も EUC-JP の文字のバイトであること
        const auto second = (dropped.length() < 2) ? 0xa1 : static_cast<unsigned char>(dropped[1]);
        const auto is_euc_second = (second >= 0xa1) && (second <= 0xfe);
        return (dropped.length() < legacy_lead_byte_length(dropped[0], LEGACY_CHARACTER_ENCODING)) && is_euc_second;
    };
    const auto convert_complete = [&]() -> tl::optional<std::string> {
        if (auto utf8 = legacy_to_sys(str, LEGACY_CHARACTER_ENCODING)) {
            return utf8;
        }

        // 全体がもう一方の文字コードと推定できるなら、末尾を落とさずに推定に任せる
        // (旧版の内部の文字コードが EUC-JP なら、「将」の Shift_JIS の 8F AB を EUC-JP の欠けた文字とみなして空にしない。
        // Shift_JIS なら、EUC-JP の文字列の末尾を Shift_JIS の欠けた文字とみなして落とさない)
        constexpr auto other_encoding = (LEGACY_CHARACTER_ENCODING == CharacterEncoding::EUC_JP) ? CharacterEncoding::SHIFT_JIS : CharacterEncoding::EUC_JP;
        if (guess_legacy_encoding(str) == other_encoding) {
            return tl::nullopt;
        }

        for (auto cut = 1; cut <= max_cut; cut++) {
            if (!is_cut_lead(str.substr(str.length() - cut))) {
                continue;
            }

            if (auto utf8 = legacy_to_sys(prefix(cut), LEGACY_CHARACTER_ENCODING)) {
                return utf8;
            }
        }

        return tl::nullopt;
    };

    const auto convert_guessed = [&]() -> tl::optional<std::string> {
        for (auto cut = 0; cut <= max_cut; cut++) {
            const auto text = prefix(cut);
            const auto encoding = guess_legacy_encoding(text);
            if (encoding == CharacterEncoding::UNKNOWN) {
                continue;
            }

            // 推定できても変換に失敗することがある (iconv などで変換するため) ので、そのときは短くして推定し直す
            if (auto utf8 = legacy_to_sys(text, encoding)) {
                return utf8;
            }
        }

        return tl::nullopt;
    };

    const auto converted = prefer_legacy_encoding ? convert_complete().or_else(convert_guessed) : convert_guessed().or_else(convert_complete);
    if (converted) {
        return *converted;
    }
#endif

    for (auto cut = 0; cut <= max_cut; cut++) {
        if (auto utf8 = sys_to_utf8(prefix(cut))) {
            return std::move(*utf8);
        }
    }

    return "?";
}

/*!
 * @brief ロックせずにスコアファイルの中身を読む
 * @return 中身。開けない、読めないときは tl::nullopt
 * @details 書き込みの権限が無く、ロックできないユーザーも表示できるよう、前の版と同じく読むだけで開く。
 */
tl::optional<std::string> read_score_file_text_unlocked()
{
    safe_setuid_grab();
    const auto fd = fd_open(score_file_path(SCORE_FILE_NAME), O_RDONLY);
    safe_setuid_drop();
    if (fd < 0) {
        return tl::nullopt;
    }

    auto text = read_whole_file(fd);
    (void)fd_close(fd);
    return text;
}
}

/*!
 * @brief 記録を scores.json の中身にする
 * @param scores 記録
 * @return scores.json の中身 (UTF-8)
 */
std::string scores_to_json(const std::vector<HighScore> &scores)
{
    auto records = nlohmann::ordered_json::array();
    for (const auto &score : scores) {
        records.push_back({
            { "game_version", score.version },
            { "points", score.points },
            { "gold", score.gold },
            { "turns", score.turns },
            { "date", score.date },
            { "name", score.name },
            { "uid", score.uid },
            { "sex", score.sex },
            { "race", score.race },
            { "class", score.pclass },
            { "personality", score.personality },
            { "level", score.level },
            { "dungeon_level", score.dungeon_level },
            { "max_level", score.max_level },
            { "max_dungeon_level", score.max_dungeon_level },
            { "died_from", score.died_from },
        });
    }

    const nlohmann::ordered_json json = { { "version", SCORE_FILE_VERSION }, { "scores", records } };
    return json.dump(2, ' ', false, nlohmann::ordered_json::error_handler_t::replace).append("\n");
}

/*!
 * @brief scores.json の中身から記録を読む
 * @param text scores.json の中身 (UTF-8)
 * @return 記録。空白だけの中身は記録 0 件とする。JSON として読めない、形式の版が新しい、欄の型が違う、
 * 種族・職業・性格が範囲外のときは tl::nullopt (表示で範囲外を読まないよう、また書き直して壊さないよう、ファイルごと使わない)
 */
tl::optional<std::vector<HighScore>> scores_from_json(std::string_view text)
{
    if (is_blank_text(text)) {
        return std::vector<HighScore>();
    }

    try {
        const auto json = nlohmann::json::parse(text);
        // 形式の版は、記録の欄と同じく整数で範囲内のものだけを読む (1.5 などを 1 に丸めて読まない)
        auto version = 0;
        if (!json.is_object() || !read_integer(json, "version", version) || (version > SCORE_FILE_VERSION)) {
            return tl::nullopt;
        }

        const auto &records = json.at("scores");
        if (!records.is_array()) {
            return tl::nullopt;
        }

        std::vector<HighScore> scores;
        for (const auto &record : records) {
            if (!record.is_object()) {
                return tl::nullopt;
            }

            HighScore score;
            score.version = record.value("game_version", "");
            score.date = record.value("date", "");
            score.name = record.value("name", "");
            score.sex = record.value("sex", "");
            score.died_from = record.value("died_from", "");
            const auto is_read = read_integer(record, "points", score.points) && read_integer(record, "gold", score.gold) &&
                                 read_integer(record, "turns", score.turns) && read_integer(record, "uid", score.uid) &&
                                 read_integer(record, "race", score.race) && read_integer(record, "class", score.pclass) &&
                                 read_integer(record, "personality", score.personality) && read_integer(record, "level", score.level) &&
                                 read_integer(record, "dungeon_level", score.dungeon_level) && read_integer(record, "max_level", score.max_level) &&
                                 read_integer(record, "max_dungeon_level", score.max_dungeon_level);
            if (!is_read || !is_valid_score(score)) {
                return tl::nullopt;
            }

            scores.push_back(std::move(score));
        }

        // 手で編集したファイルでも順位を求められるよう、スコアの高い順に並べる (同じスコアは元の順)
        std::ranges::stable_sort(scores, std::greater<>{}, &HighScore::points);
        return scores;
    } catch (const nlohmann::json::exception &) {
        return tl::nullopt;
    }
}

/*!
 * @brief 新しい記録が入る順位を求める
 * @param scores 記録 (スコアの高い順)
 * @param points 新しい記録のスコア
 * @return 順位 (0 が 1 位)。同じスコアの記録の後ろに入る。記録が上限まであり、どれよりも低いときは最下位と入れ替わる
 */
int find_score_rank(const std::vector<HighScore> &scores, int64_t points)
{
    const auto count = std::min(static_cast<int>(scores.size()), MAX_HISCORES);
    for (auto i = 0; i < count; i++) {
        if (points > scores[i].points) {
            return i;
        }
    }

    return (count < MAX_HISCORES) ? count : MAX_HISCORES - 1;
}

/*!
 * @brief 記録を順位の位置に入れ、上限を超えた記録を除く
 * @param scores 記録 (スコアの高い順)
 * @param score 新しい記録
 * @return 入れた順位 (0 が 1 位)
 */
int insert_score(std::vector<HighScore> &scores, const HighScore &score)
{
    const auto rank = find_score_rank(scores, score.points);
    scores.insert(scores.begin() + rank, score);
    if (std::ssize(scores) > MAX_HISCORES) {
        scores.resize(MAX_HISCORES);
    }

    return rank;
}

/*!
 * @brief scores.raw の 1 件を、移し替える記録にする
 * @param raw scores.raw の 1 件
 * @return 記録。名前と死因は UTF-8 にし、日付は scores.json の形にする
 */
HighScore migrate_raw_score(const high_score &raw)
{
    auto score = raw.to_score();
    score.date = migrate_date(score.date);
    score.name = migrate_text(score.name, true);
    score.died_from = migrate_text(score.died_from, false);
    return score;
}

/*!
 * @brief スコアファイルを読む
 * @return 記録 (スコアの高い順)。開けない、読めないときは tl::nullopt
 */
tl::optional<std::vector<HighScore>> read_score_file()
{
    // ほかのゲームが書き換えている途中を読まないよう、ロックできればロックして読む
    LockedScoreFile file;
    const auto text = file.is_locked() ? file.read() : read_score_file_text_unlocked();
    if (!text) {
        return tl::nullopt;
    }

    return scores_from_json(*text);
}

/*!
 * @brief スコアファイルに記録を足せるか
 * @return ロックでき、記録を読めれば true。読めても書けないユーザーは false
 */
bool can_add_score_to_file()
{
    LockedScoreFile file;
    if (!file.is_locked()) {
        return false;
    }

    const auto text = file.read();
    return text && scores_from_json(*text);
}

/*!
 * @brief スコアファイルに記録を足す
 * @param score 足す記録
 * @param rank 足した順位 (0 が 1 位) を返す
 * @return 足した後の記録 (スコアの高い順)。ロックできない、読めない、書き込めないときは tl::nullopt
 */
tl::optional<std::vector<HighScore>> add_score_to_file(const HighScore &score, int &rank)
{
    LockedScoreFile file;
    if (!file.is_locked()) {
        return tl::nullopt;
    }

    const auto text = file.read();
    if (!text) {
        return tl::nullopt;
    }

    auto scores = scores_from_json(*text);
    if (!scores) {
        return tl::nullopt;
    }

    rank = insert_score(*scores, score);
    if (!file.write(scores_to_json(*scores))) {
        return tl::nullopt;
    }

    return scores;
}

/*!
 * @brief スコアファイルが無ければ作り、前の版のスコアファイルから記録を移す
 * @return スコアファイルがあるか、作れたら true
 * @details
 * scores.json が無い、または空 (make install で作った直後など) で、前の版のスコアファイル (scores.raw) があれば、
 * その記録を移す。scores.raw は旧版のために残し、scores.json に記録を書いた後は読まない。
 */
bool prepare_score_file()
{
    // 調べられないときは、あるスコアファイルを移し直して上書きしかねないので、何もしない
    const auto path = score_file_path(SCORE_FILE_NAME);
    const auto exists = file_exists(path);
    if (!exists) {
        return false;
    }

    // 記録があれば何もしない。読めても書けないユーザーも、前の版と同じく (記録はできないが) 起動できるよう、ロックせずに読む
    if (*exists) {
        const auto text = read_score_file_text_unlocked();
        if (!text) {
            return false;
        }

        if (!is_blank_text(*text)) {
            return true;
        }
    } else {
        safe_setuid_grab();
        const auto fd = fd_make(path, true);
        if (fd >= 0) {
            // 作ったユーザーの umask によらず、ほかのユーザーのゲームも読み書きできるようにする
            std::error_code ec;
            std::filesystem::permissions(path, SHARED_FILE_PERMISSION, ec);
            (void)fd_close(fd);
        }

        safe_setuid_drop();
        // ほかのゲームが同時に作ったときは作れないが、あれば構わない
        if ((fd < 0) && !file_exists(path).value_or(false)) {
            return false;
        }
    }

    // ここではファイルがある。ロックできない (書き込みの権限が無いなど) ときは移せないが、前の版と同じく
    // (記録はできないが) 起動できるよう、空のファイルを記録 0 件として使う
    LockedScoreFile file;
    if (!file.is_locked()) {
        return true;
    }

    // 空でなければ移さない (ロックを待つ間に、ほかのゲームが移していることもある)
    const auto text = file.read();
    if (!text) {
        return false;
    }

    if (!is_blank_text(*text)) {
        return true;
    }

    const auto raw_path = score_file_path(OLD_SCORE_FILE_NAME);
    // 前の版は権限を上げて scores.raw を開いていたので、グループにしか読めないこともある
    safe_setuid_grab();
    const auto fd = fd_open(raw_path, O_RDONLY);
    safe_setuid_drop();
    if (fd < 0) {
        // scores.raw があるのに (あるかを調べられずに) 読めないときは、記録を移せないまま記録が足されないよう、使わない
        const auto raw_exists = file_exists(raw_path);
        if (!raw_exists || *raw_exists) {
            return false;
        }

        // 起動するたびに移し替えを確かめ直さないよう、記録 0 件を書いておく
        return file.write(scores_to_json({}));
    }

    // 途中で読めなくなったとき、読めた分だけを移して残りを移し損ねないよう、先に件数を求める。
    // 末尾の 1 件に満たないバイトは、前の版と同じく読まない
    const auto raw_size = fd_size(fd);
    if (!raw_size || fd_seek(fd, 0)) {
        (void)fd_close(fd);
        return false;
    }

    // 範囲外の記録はスコアファイルごと使えなくなるので移さない
    const auto num_raw = *raw_size / sizeof(high_score);
    std::vector<HighScore> scores;
    for (ulong i = 0; (i < num_raw) && (std::ssize(scores) < MAX_HISCORES); i++) {
        high_score raw;
        if (fd_read(fd, reinterpret_cast<char *>(&raw), sizeof(raw))) {
            (void)fd_close(fd);
            return false;
        }

        if (auto score = migrate_raw_score(raw); is_valid_score(score)) {
            scores.push_back(std::move(score));
        }
    }

    (void)fd_close(fd);
    // 移す記録が無くても、起動するたびに移し直さないよう書いておく
    return file.write(scores_to_json(scores));
}
