#pragma once

/*
 * @file enchanter-base.h
 * @brief エゴ・ランダムアーティファクト・呪われたアイテムをベースアイテムから生成する処理の基底クラス
 * @author Hourier
 * @date 2021/04/30
 * @details 純粋仮想関数につき、必要なメンバ変数は派生クラスで設定すること
 */

class ItemEntity;
class PlayerType;
class EnchanterBase {
public:
    virtual void apply_magic() = 0;
    virtual ~EnchanterBase() = default;

protected:
    EnchanterBase() = default;

    /*!
     * @brief svalごとの強化処理.
     * @details
     * 現在はアミュレット・指輪のみ.
     * ドラゴン防具シリーズのようなランダム強化アイテムが他にも実装されたらメソッドを分割する.
     */
    virtual void sval_enchant() = 0;
    virtual void give_ego_index() = 0;
    virtual void give_high_ego_index() = 0;
    virtual void give_cursed() = 0;
};
