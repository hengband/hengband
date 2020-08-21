#include "info-reader/info-reader-util.h"
#include "main/angband-headers.h"
#include "object-enchant/activation-info-table.h"
#include "view/display-messages.h"

/* Help give useful error messages */
int error_idx; /*!< データ読み込み/初期化時に汎用的にエラーコードを保存するグローバル変数 */
int error_line; /*!< データ読み込み/初期化時に汎用的にエラー行数を保存するグローバル変数 */

/*!
 * @brief データの可変文字列情報をテキストとして保管する /
 * Add a text to the text-storage and store offset to it.
 * @param offset 文字列保管ポインタからのオフセット
 * @param head テキスト保管ヘッダ情報の構造体参照ポインタ
 * @param buf 保管文字列
 * @param normal_text テキストの正規化を行う
 * @return
 * 無事保管ができたらTRUEを返す。
 * Returns FALSE when there isn't enough space available to store
 * the text.
 */
bool add_text(u32b *offset, angband_header *head, concptr buf, bool normal_text)
{
    if (head->text_size + strlen(buf) + 8 > FAKE_TEXT_SIZE)
        return FALSE;

    if (*offset == 0) {
        *offset = ++head->text_size;
    } else if (normal_text) {
        /*
         * If neither the end of the last line nor
         * the beginning of current line is not a space,
         * fill up a space as a correct separator of two words.
         */
        if (head->text_size > 0 &&
#ifdef JP
            (*(head->text_ptr + head->text_size - 1) != ' ') && ((head->text_size == 1) || !iskanji(*(head->text_ptr + head->text_size - 2))) && (buf[0] != ' ')
            && !iskanji(buf[0])
#else
            (*(head->text_ptr + head->text_size - 1) != ' ') && (buf[0] != ' ')
#endif
        ) {
            *(head->text_ptr + head->text_size) = ' ';
            head->text_size++;
        }
    }

    strcpy(head->text_ptr + head->text_size, buf);
    head->text_size += strlen(buf);
    return TRUE;
}

/*!
 * @brief データの可変文字列情報を名前として保管する /
 * Add a name to the name-storage and return an offset to it.
 * @param offset 文字列保管ポインタからのオフセット
 * @param head テキスト保管ヘッダ情報の構造体参照ポインタ
 * @param buf 保管文字列
 * @return
 * 無事保管ができたらTRUEを返す。
 * Returns FALSE when there isn't enough space available to store
 * the text.
 */
bool add_name(u32b *offset, angband_header *head, concptr buf)
{
    if (head->name_size + strlen(buf) + 8 > FAKE_NAME_SIZE)
        return FALSE;

    if (*offset == 0) {
        *offset = ++head->name_size;
    }

    strcpy(head->name_ptr + head->name_size, buf);
    head->name_size += strlen(buf);
    return TRUE;
}

/*!
 * @brief データの可変文字列情報をタグとして保管する /
 * Add a tag to the tag-storage and return an offset to it.
 * @param offset 文字列保管ポインタからのオフセット
 * @param head テキスト保管ヘッダ情報の構造体参照ポインタ
 * @param buf 保管文字列
 * @return
 * 無事保管ができたらTRUEを返す。
 * Returns FALSE when there isn't enough space available to store
 * the text.
 */
bool add_tag(STR_OFFSET *offset, angband_header *head, concptr buf)
{
    u32b i;
    for (i = 1; i < head->tag_size; i += strlen(&head->tag_ptr[i]) + 1) {
        if (streq(&head->tag_ptr[i], buf))
            break;
    }

    if (i >= head->tag_size) {
        if (head->tag_size + strlen(buf) + 8 > FAKE_TAG_SIZE)
            return FALSE;

        strcpy(head->tag_ptr + head->tag_size, buf);
        i = head->tag_size;
        head->tag_size += strlen(buf) + 1;
    }

    *offset = (s16b)i;
    return TRUE;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(汎用) /
 * Grab one flag from a textual string
 * @param flags ビットフラグを追加する先の参照ポインタ
 * @param names トークン定義配列
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
errr grab_one_flag(u32b *flags, concptr names[], concptr what)
{
    for (int i = 0; i < 32; i++) {
        if (streq(what, names[i])) {
            *flags |= (1L << i);
            return 0;
        }
    }

    return -1;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(発動能力用) /
 * Grab one activation index flag
 * @param what 参照元の文字列ポインタ
 * @return 発動能力ID
 */
byte grab_one_activation_flag(concptr what)
{
    for (int i = 0;; i++) {
        if (activation_info[i].flag == NULL)
            break;

        if (streq(what, activation_info[i].flag)) {
            return activation_info[i].index;
        }
    }

    int j = atoi(what);
    if (j > 0) {
        return ((byte)j);
    }

    msg_format(_("未知の発動・フラグ '%s'。", "Unknown activation flag '%s'."), what);
    return 0;
}
