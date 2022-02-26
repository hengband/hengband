#pragma once

#include "system/angband.h"

/* ELDRITCH_HORRORによるsanity blast処理に関するメッセージの最大数 / Number of entries in the sanity-blast descriptions */
#define MAX_SAN_HORROR_SUM 20 /*!< 恐ろしい対象の形容数(合計) */
#define MAX_SAN_HORROR_COMMON 5 /*!< 恐ろしい対象の形容数(正常時、邪悪・中立共通) */
#define MAX_SAN_HORROR_EVIL 15 /*!< 恐ろしい対象の形容数(正常時、邪悪) */
#define MAX_SAN_HORROR_NEUTRAL 15 /*!< 恐ろしい対象の形容数(正常時、中立) */
#define MAX_SAN_FUNNY 22 /*!< 恐ろしい対象の形容数(幻覚時) */
#define MAX_SAN_COMMENT 5 /*!< 恐ろしい対象を見たときの絶叫メッセージ数(幻覚時) */

extern concptr horror_desc_common[MAX_SAN_HORROR_COMMON];
extern concptr horror_desc_evil[MAX_SAN_HORROR_EVIL];
extern concptr horror_desc_neutral[MAX_SAN_HORROR_NEUTRAL];
extern concptr funny_desc[MAX_SAN_FUNNY];
extern concptr funny_comments[MAX_SAN_COMMENT];
