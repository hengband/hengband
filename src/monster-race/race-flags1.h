#pragma once

enum race_flags1 {
    RF1_UNIQUE = 0x00000001, /*!< モンスター特性: ユニーク / Unique Monster */
    RF1_QUESTOR = 0x00000002, /*!< モンスター特性: クエストモンスター / Quest Monster */
    RF1_MALE = 0x00000004, /*!< モンスター特性: 男性 / Male gender */
    RF1_FEMALE = 0x00000008, /*!< モンスター特性: 女性 / Female gender */
    RF1_CHAR_CLEAR = 0x00000010, /*!< モンスター特性: シンボルが完全に透明 / Absorbs symbol */
    RF1_SHAPECHANGER = 0x00000020, /*!< モンスター特性: シンボルアルファベットがランダムになる / TY: shapechanger */
    RF1_ATTR_CLEAR = 0x00000040, /*!< モンスター特性: シンボルカラーが透明色になる(地形と同じ色になる) / Absorbs color */
    RF1_ATTR_MULTI = 0x00000080, /*!< モンスター特性: シンボルカラーがランダムに変化する(基本7色) / Changes color */
    RF1_FORCE_DEPTH = 0x00000100, /*!< モンスター特性: 指定階未満では生成されない / Start at "correct" depth */
    RF1_FORCE_MAXHP = 0x00000200, /*!< モンスター特性: 通常生成時必ずHPがダイス最大値になる / Start with max hitpoints */
    RF1_PREVENT_SUDDEN_MAGIC = 0x00000400, /*!< モンスター特性: 生成直後に魔法を使用しない / Start with no-magic */
    RF1_FORCE_EXTRA = 0x00000800, /*!< モンスター特性: (未使用) / Start out something */
    RF1_ATTR_SEMIRAND = 0x00001000, /*!< モンスター特性: シンボルカラーがランダムに変化する(15色) / Color is determined semi-randomly */
    RF1_FRIENDS = 0x00002000, /*!< モンスター特性: 同種の友軍を用意している / Arrive with some friends */
    RF1_ESCORT = 0x00004000, /*!< モンスター特性: 護衛を用意している/ Arrive with an escort */
    RF1_ESCORTS = 0x00008000, /*!< モンスター特性: さらに大量の護衛を用意している / Arrive with some escorts */
    RF1_NEVER_BLOW = 0x00010000, /*!< モンスター特性: 打撃を一切行わない / Never make physical blow */
    RF1_NEVER_MOVE = 0x00020000, /*!< モンスター特性: 移動を一切行わない / Never make physical move */
    RF1_RAND_25 = 0x00040000, /*!< モンスター特性: ランダムに移動する確率+25%/ Moves randomly (25%) */
    RF1_RAND_50 = 0x00080000, /*!< モンスター特性: ランダムに移動する確率+50%/ Moves randomly (50%) */
    RF1_ONLY_GOLD = 0x00100000, /*!< モンスター特性: 財宝しか落とさない / Drop only gold */
    RF1_ONLY_ITEM = 0x00200000, /*!< モンスター特性: アイテムしか落とさない / Drop only items */
    RF1_DROP_60 = 0x00400000, /*!< モンスター特性: 落とすアイテム数60%で+1/ Drop an item/gold (60%) */
    RF1_DROP_90 = 0x00800000, /*!< モンスター特性: 落とすアイテム数90%で+1 / Drop an item/gold (90%) */
    RF1_DROP_1D2 = 0x01000000, /*!< モンスター特性: 落とすアイテム数+1d2 / Drop 1d2 items/gold */
    RF1_DROP_2D2 = 0x02000000, /*!< モンスター特性: 落とすアイテム数+2d2 / Drop 2d2 items/gold */
    RF1_DROP_3D2 = 0x04000000, /*!< モンスター特性: 落とすアイテム数+3d2 / Drop 3d2 items/gold */
    RF1_DROP_4D2 = 0x08000000, /*!< モンスター特性: 落とすアイテム数+4d2 / Drop 4d2 items/gold */
    RF1_DROP_GOOD = 0x10000000, /*!< モンスター特性: 必ず上質品をドロップする / Drop good items */
    RF1_DROP_GREAT = 0x20000000, /*!< モンスター特性: 必ず高級品をドロップする / Drop great items */
    RF1_XXX2 = 0x40000000, /*!< モンスター特性: 未使用 / XXX */
    RF1_XXX3 = 0x80000000, /*!< モンスター特性: 未使用 / XXX */
};
