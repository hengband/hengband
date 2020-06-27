/*!
 * @brief ベースアイテム情報の構造体 / Information about object "kinds", including player knowledge.
 * @date 2019/05/01
 * @author deskull
 * @details
 * ゲーム進行用のセーブファイル上では aware と tried のみ保存対象とすること。と英文ではあるが実際はもっとある様子である。 /
 * Only "aware" and "tried" are saved in the savefile
 */

#include "object-kind.h"

/*
 * The object kind arrays
 */
object_kind *k_info;
char *k_name;
char *k_text;

/*
 * Maximum number of items in k_info.txt
 */
KIND_OBJECT_IDX max_k_idx;
