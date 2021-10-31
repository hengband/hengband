#pragma once

/*
 * @file enchanter-base.h
 * @brief エゴ・ランダムアーティファクト・呪われたアイテムをベースアイテムから生成する処理の基底クラス
 * @author Hourier
 * @date 2021/04/30
 * @details 純粋仮想関数につき、必要なメンバ変数は派生クラスで設定すること
 */

#include "system/angband.h"

struct object_type;
struct player_type;
class EnchanterBase {
public:
    virtual void apply_magic() = 0;

protected:
    EnchanterBase() = default;
    virtual ~EnchanterBase() = default;
    virtual void enchant() = 0;
    virtual void give_ego_index() = 0;
    virtual void give_high_ego_index() = 0;
    virtual void give_cursed() = 0;
};
