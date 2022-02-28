#pragma once

enum player_redraw_type {
    PR_MISC = 0x00000001L, /*!< 再描画フラグ: 種族と職業 / Display Race/Class */
    PR_TITLE = 0x00000002L, /*!< 再描画フラグ: 称号 / Display Title */
    PR_LEV = 0x00000004L, /*!< 再描画フラグ: レベル / Display Level */
    PR_EXP = 0x00000008L, /*!< 再描画フラグ: 経験値 / Display Experience */
    PR_STATS = 0x00000010L, /*!< 再描画フラグ: ステータス /  Display Stats */
    PR_ARMOR = 0x00000020L, /*!< 再描画フラグ: AC / Display Armor */
    PR_HP = 0x00000040L, /*!< 再描画フラグ: HP / Display Hitpoints */
    PR_MANA = 0x00000080L, /*!< 再描画フラグ: MP / Display Mana */
    PR_GOLD = 0x00000100L, /*!< 再描画フラグ: 所持金 / Display Gold */
    PR_DEPTH = 0x00000200L, /*!< 再描画フラグ: ダンジョンの階 / Display Depth */
    PR_EQUIPPY = 0x00000400L, /*!< 再描画フラグ: 装備シンボル / Display equippy chars */
    PR_HEALTH = 0x00000800L, /*!< 再描画フラグ: モンスターのステータス / Display Health Bar */
    PR_CUT = 0x00001000L, /*!< 再描画フラグ: 負傷度 / Display Extra (Cut) */
    PR_STUN = 0x00002000L, /*!< 再描画フラグ: 朦朧度 / Display Extra (Stun) */
    PR_HUNGER = 0x00004000L, /*!< 再描画フラグ: 空腹度 / Display Extra (Hunger) */
    PR_STATUS = 0x00008000L, /*!< 再描画フラグ: プレイヤーの付与状態 /  Display Status Bar */
    PR_XXX0 = 0x00010000L, /*!< (unused) */
    PR_UHEALTH = 0x00020000L, /*!< 再描画フラグ: ペットのステータス / Display Uma Health Bar */
    PR_XXX1 = 0x00040000L, /*!< (unused) */
    PR_XXX2 = 0x00080000L, /*!< (unused) */
    PR_STATE = 0x00100000L, /*!< 再描画フラグ: プレイヤーの行動状態 / Display Extra (State) */
    PR_SPEED = 0x00200000L, /*!< 再描画フラグ: 加速 / Display Extra (Speed) */
    PR_STUDY = 0x00400000L, /*!< 再描画フラグ: 学習 / Display Extra (Study) */
    PR_IMITATION = 0x00800000L, /*!< 再描画フラグ: ものまね / Display Extra (Imitation) */
    PR_EXTRA = 0x01000000L, /*!< 再描画フラグ: 拡張ステータス全体 / Display Extra Info */
    PR_BASIC = 0x02000000L, /*!< 再描画フラグ: 基本ステータス全体 / Display Basic Info */
    PR_MAP = 0x04000000L, /*!< 再描画フラグ: ゲームマップ / Display Map */
    PR_WIPE = 0x08000000L, /*!< 再描画フラグ: 画面消去 / Hack -- Total Redraw */
};
