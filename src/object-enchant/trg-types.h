#pragma once

// clang-format off
/*!
 * @brief アイテム生成条件フラグ
 */
enum class ItemGenerationTraitType {
	INSTA_ART         =  0, //!< 固定アーティファクトになる / Item must be an artifact
    QUESTITEM         =  1, //!< クエスト専用アイテム / quest level item -KMW-
    XTRA_POWER        =  2, //!< 追加能力を得る / Extra power
    ONE_SUSTAIN       =  3, //!< 追加維持を得る / One sustain
    XTRA_RES_OR_POWER =  4, //!< 追加能力or耐性を得る / Extra resistance or power
    XTRA_H_RES        =  5, //!< 追加上位耐性を得る / Extra high resistance
    XTRA_E_RES        =  6, //!< 追加元素耐性を得る / Extra element resistance
    XTRA_L_RES        =  7, //!< 王者の加護版追加耐性を得る / Extra lordly resistance
    XTRA_D_RES        =  8, //!< ドラゴン的追加耐性を得る / Extra dragon resistance
    XTRA_RES          =  9, //!< 追加耐性を得る / Extra resistance
    CURSED            = 10, //!< 呪われている / Item is Cursed
    HEAVY_CURSE       = 11, //!< 重い呪い / Item is Heavily Cursed
    PERMA_CURSE       = 12, //!< 永遠の呪い / Item is Perma Cursed
    RANDOM_CURSE0     = 13, //!< 軽い呪い効果を付ける / Item is Random Cursed
    RANDOM_CURSE1     = 14, //!< 呪い効果を付ける(太古と反感以外) / Item is Random Cursed
    RANDOM_CURSE2     = 15, //!< 重い呪い効果を付ける / Item is Random Cursed
    XTRA_DICE         = 16, //!< ダイス数を増やす / Extra dice
    POWERFUL          = 17, //!< 呪われているが修正値を正にする / Item has good value even if Cursed
    LIGHT_WEIGHT      = 18, //!< 重量を軽くする(2/3倍) / Make lighten (エゴ用、他は重量を普通に設定でよい)
    HEAVY_WEIGHT      = 19, //!< 重量を重くする(4/3倍) / Make heavy (エゴ用、他は重量を普通に設定でよい)
    XTRA_AC           = 20, //!< ベースACを上げる / Add base AC (エゴ用、他はACを普通に設定でよい)
    HIGH_TELEPATHY    = 21, //!< 高水準のテレパシーを付ける(テレパシーの冠) / Add high quarity esps.
    LOW_TELEPATHY     = 22, //!< 中水準のテレパシーを付ける(鋭敏) / Add middle quarity esps.
    XTRA_L_ESP        = 23, //!< 種族テレパシーを得る / Extra racial esp.
    MOD_ACCURACY      = 24, //!< 命中偏重の殺戮修正にする / Adjust to-hit more than to-dam.
    MOD_VELOCITY      = 25, //!< ダメージ偏重の殺戮修正にする / Adjust to-dam more than to-hit.
    XTRA_DICE_SIDE    = 26, //!< ダイス面を増やす / Extra dice side
    ADD_DICE          = 27, //!< ダイス数を1増やす / Add dice number
    DOUBLED_DICE      = 28, //!< ダイス数を2倍にする / Doubled dice number
    MAX,
};
// clang-format on
