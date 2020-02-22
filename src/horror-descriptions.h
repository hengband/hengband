#pragma once

/* ELDRITCH_HORRORによるsanity blast処理に関するメッセージの最大数 / Number of entries in the sanity-blast descriptions */
#define MAX_SAN_HORROR 20 /*!< 恐ろしい対象の形容数(正常時) */
#define MAX_SAN_FUNNY 22  /*!< 恐ろしい対象の形容数(幻覚時) */
#define MAX_SAN_COMMENT 5 /*!< 恐ろしい対象を見たときの絶叫メッセージ数(幻覚時) */

concptr horror_desc[MAX_SAN_HORROR];
concptr funny_desc[MAX_SAN_FUNNY];
concptr funny_comments[MAX_SAN_COMMENT];
