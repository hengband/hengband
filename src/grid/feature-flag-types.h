#pragma once

/*
 * Feature flags - should be used instead of feature indexes unless generating.
 * Originally from UnAngband, and modified into TR-like style in Hengband
 */

enum feature_flag_type : int {
	FF_LOS = 0, /*!< 視界が通る地形である */
    FF_PROJECT = 1, /*!< 飛び道具が通過できる地形である */
    FF_MOVE = 2, /*!< 移動可能な地形である */
    FF_PLACE = 3, /*!< モンスター配置をしても良い地形である(cave_empty_bold/cave_empty_gridで利用) */
    FF_DROP = 4, /*!< アイテムを落としてよい地形である */
    FF_SECRET = 5, /*!< 隠し扉やトラップが潜んでいる地形である */
    FF_NOTICE = 6, /*!< 何か興味を引くものがある地形である(シフトキー＋方向で走行中の時に止まる基準) */
    FF_REMEMBER = 7, /*!< 常に記憶対象となる地形である(記憶喪失時に忘れたりしなくなる) */
    FF_OPEN = 8, /*!< 開けるコマンドの対象となる地形である */
    FF_CLOSE = 9, /*!< 閉じるコマンドの対象となる地形である */
    FF_BASH = 10, /*!< 体当たりコマンドの対象となる地形である */
    FF_SPIKE = 11, /*!< くさびを打つコマンドの対象となる地形である */
    FF_DISARM = 12, /*!< 解除コマンドの対象となる地形である */
    FF_STORE = 13, /*!< 店舗の入口となる地形である */
    FF_TUNNEL = 14, /*!< 魔王変化などで掘り進められる地形である */
    FF_MAY_HAVE_GOLD = 15, /*!< 何か財宝を隠した可能性のある地形である？(f_infoに使用している地形なし) */
    FF_HAS_GOLD = 16, /*!< 財宝を含んだ地形である */
    FF_HAS_ITEM = 17, /*!< アイテムを含んだ地形である */
    FF_DOOR = 18, /*!< ドアのある地形である */
    FF_TRAP = 19, /*!< トラップのある地形である */
    FF_STAIRS = 20, /*!< 階段のある地形である */
    FF_RUNE_PROTECTION = 21, /*!< 守りのルーンが張られた地形である */
    FF_LESS = 22, /*!< 階上に通じる地形である */
    FF_MORE = 23, /*!< 階下に通じる地形である */
    FF_AVOID_RUN = 24, /*!< 自動移動機能時に障害として迂回すべき地形である */
    FF_FLOOR = 25, /*!< 床のある地形である */
    FF_WALL = 26, /*!< 壁のある地形である */
    FF_PERMANENT = 27, /*!< 絶対に破壊できない永久地形である */
    FF_HIT_TRAP = 31, /*!< トラップのある地形である(TRAPと常に重複している？) */
    FF_GLOW = 37, /*!< 常に光っている地形である */
    FF_ENSECRET = 38, /*!< 不明(f_info.txt上で利用している地形がない) */
    FF_WATER = 39, /*!< 水のある地形である */
    FF_LAVA = 40, /*!< 溶岩のある地形である */
    FF_SHALLOW = 41, /*!< 浅い地形である */
    FF_DEEP = 42, /*!< 深い地形である */
    FF_POISON_PUDDLE = 43, /*!< 毒溜まりがある */
    FF_HURT_ROCK = 44, /*!< 岩石溶解の対象となる地形である */
    FF_COLD_PUDDLE = 48, /*!< 冷気溜まりがある */
    FF_ACID_PUDDLE = 49, /*!< 酸溜まりがある */
    FF_ELEC_PUDDLE = 51, /*!< 接地部が帯電している */
    FF_CAN_FLY = 53, /*!< 飛行可能な地形である */
    FF_CAN_SWIM = 54, /*!< 泳ぐことが可能な地形である */
    FF_CAN_PASS = 55, /*!< 通過可能な地形である */
    FF_CAN_DIG = 57, /*!< 掘削コマンドの対象となる地形である */
    FF_TREE = 83, /*!< 木の生えた地形である */
    FF_SPECIAL = 96, /*!< クエストやダンジョンに関わる特別な地形である */
    FF_HURT_DISI = 97, /*!< 分解属性の対象となる地形である */
    FF_QUEST_ENTER = 98, /*!< クエストの入り口である */
    FF_QUEST_EXIT = 99, /*!< クエストの出口である */
    FF_QUEST = 100, /*!< クエストに関する地形である */
    FF_SHAFT = 101, /*!< 坑道である。(2階層移動する階段である) */
    FF_MOUNTAIN = 102, /*!< ダンジョンの山地形である */
    FF_BLDG = 103, /*!< 施設の入り口である */
    FF_RUNE_EXPLOSION = 104, /*!< 爆発のルーンのある地形である */
    FF_PATTERN = 105, /*!< パターンのある地形である */
    FF_TOWN = 106, /*!< 広域マップ用の街がある地形である */
    FF_ENTRANCE = 107, /*!< 広域マップ用のダンジョンがある地形である */
    FF_MIRROR = 108, /*!< 鏡使いの鏡が張られた地形である */
    FF_UNPERM = 109, /*!< 破壊不能な地形である(K:フラグ向け？) */
    FF_TELEPORTABLE = 110, /*!< テレポート先の対象となる地形である */
    FF_CONVERT = 111, /*!< 地形生成処理中の疑似フラグ */
    FF_GLASS = 112, /*!< ガラス製の地形である */
    FF_FLAG_MAX = 113
};

#define FF_FLAG_SIZE (1 + ((FF_FLAG_MAX - 1) / 32))
