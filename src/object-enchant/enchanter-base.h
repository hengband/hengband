#pragma once

#include <tl/optional.hpp>

/*
 * @file enchanter-base.h
 * @brief エゴ・ランダムアーティファクト・呪われたアイテムをベースアイテムから生成する処理の基底クラス
 * @author Hourier
 * @date 2021/04/30
 * @details
 * エゴの要求は基底で保持し、種類別の強化処理で適用する。要求には次の3通りがある。
 * - 要求なし: 自然生成。ランダムアーティファクトの抽選を含め、元のとおりに強化する。
 * - 指定のエゴ (request_ego()): ランダムアーティファクトにせず、そのエゴを付ける。
 * - ランダムなよいエゴ (request_random_ego()): ランダムアーティファクトにせず、種類に合うよいエゴを抽選する。
 * どちらの要求も、呼び出し側で can_apply_ego() により付けられることを確かめてから行う。
 */

enum class EgoType;
class ItemEntity;
class PlayerType;
class EnchanterBase {
public:
    void request_ego(EgoType ego)
    {
        this->specific_ego_request = ego;
        this->random_ego_requested = false;
    }
    void request_random_ego()
    {
        this->specific_ego_request = tl::nullopt;
        this->random_ego_requested = true;
    }
    virtual void apply_magic() = 0;
    virtual ~EnchanterBase() = default;

protected:
    EnchanterBase() = default;

    /*! 指定のエゴかランダムなよいエゴの要求があるか（あればランダムアーティファクトにしない） */
    bool has_ego_request() const
    {
        return this->specific_ego_request.has_value() || this->random_ego_requested;
    }

    /*! 指定のエゴの要求（要求なし・ランダムなよいエゴの要求のときは空） */
    const tl::optional<EgoType> &get_specific_ego_request() const
    {
        return this->specific_ego_request;
    }

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

private:
    tl::optional<EgoType> specific_ego_request;
    bool random_ego_requested = false;
};
