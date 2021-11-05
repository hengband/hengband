﻿#include "player-info/alignment.h"
#include "artifact/fixed-art-types.h"
#include "avatar/avatar.h"
#include "game-option/text-display-options.h"
#include "inventory/inventory-slot-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "player-info/equipment-info.h"
#include "player-info/race-info.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

PlayerAlignment::PlayerAlignment(PlayerType *player_ptr)
{
    this->player_ptr = player_ptr;
}

/*!
 * @brief プレイヤーの抽象的善悪アライメントの表記を返す。 / Return alignment title
 * @param with_value 徳の情報と一緒に表示する時だけtrue
 * @return アライメントの表記を返す。
 */
concptr PlayerAlignment::get_alignment_description(bool with_value)
{
    auto s = alignment_label();
    if (with_value || show_actual_value)
        return format(_("%s(%ld)", "%s (%ld)"), s, static_cast<long>(this->player_ptr->alignment));

    return s;
}

/*
 * @brief アライメント情報の更新
 * @param なし
 */
void PlayerAlignment::update_alignment()
{
    this->reset_alignment();
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (MONSTER_IDX m_idx = floor_ptr->m_max - 1; m_idx >= 1; m_idx--) {
        auto *m_ptr = &floor_ptr->m_list[m_idx];
        if (!monster_is_valid(m_ptr))
            continue;
        auto *r_ptr = &r_info[m_ptr->r_idx];

        if (!is_pet(m_ptr))
            continue;

        if (any_bits(r_ptr->flags3, RF3_GOOD)) {
            this->bias_good_alignment(r_ptr->level);
        }

        if (any_bits(r_ptr->flags3, RF3_EVIL)) {
            this->bias_evil_alignment(r_ptr->level);
        }
    }

    if (player_ptr->mimic_form) {
        switch (player_ptr->mimic_form) {
        case MIMIC_DEMON:
            this->bias_evil_alignment(200);
            break;
        case MIMIC_DEMON_LORD:
            this->bias_evil_alignment(200);
            break;
        }
    } else {
        switch (player_ptr->prace) {
        case PlayerRaceType::ARCHON:
            this->bias_good_alignment(200);
            break;
        case PlayerRaceType::BALROG:
            this->bias_evil_alignment(200);
            break;

        default:
            break;
        }
    }

    for (int i = 0; i < 2; i++) {
        if (!has_melee_weapon(player_ptr, INVEN_MAIN_HAND + i) || (player_ptr->inventory_list[INVEN_MAIN_HAND + i].name1 != ART_IRON_BALL))
            continue;

        this->bias_evil_alignment(1000);
    }

    int j = 0;
    int neutral[2];
    for (int i = 0; i < 8; i++) {
        switch (player_ptr->vir_types[i]) {
        case V_JUSTICE:
            this->bias_good_alignment(player_ptr->virtues[i] * 2);
            break;
        case V_CHANCE:
            break;
        case V_NATURE:
        case V_HARMONY:
            neutral[j++] = i;
            break;
        case V_UNLIFE:
            this->bias_evil_alignment(player_ptr->virtues[i]);
            break;
        default:
            this->bias_good_alignment(player_ptr->virtues[i]);
            break;
        }
    }

    for (int i = 0; i < j; i++) {
        if (player_ptr->alignment > 0) {
            this->bias_evil_alignment(player_ptr->virtues[neutral[i]] / 2);
            if (player_ptr->alignment < 0)
                this->reset_alignment();
        } else if (player_ptr->alignment < 0) {
            this->bias_good_alignment(player_ptr->virtues[neutral[i]] / 2);
            if (player_ptr->alignment > 0)
                this->reset_alignment();
        }
    }
}

void PlayerAlignment::bias_good_alignment(int value)
{
    this->player_ptr->alignment += value;
}

void PlayerAlignment::bias_evil_alignment(int value)
{
    this->player_ptr->alignment -= value;
}

void PlayerAlignment::reset_alignment()
{
    this->player_ptr->alignment = 0;
}

/*!
 * @brief プレイヤーの抽象的善悪アライメントの表記名のみを返す。 / Return only alignment title
 * @param player_ptr プレイヤーへの参照ポインタ。
 * @return アライメントの表記名
 */
concptr PlayerAlignment::alignment_label()
{
    if (this->player_ptr->alignment > 150)
        return _("大善", "Lawful");
    else if (this->player_ptr->alignment > 50)
        return _("中善", "Good");
    else if (this->player_ptr->alignment > 10)
        return _("小善", "Neutral Good");
    else if (this->player_ptr->alignment > -11)
        return _("中立", "Neutral");
    else if (this->player_ptr->alignment > -51)
        return _("小悪", "Neutral Evil");
    else if (this->player_ptr->alignment > -151)
        return _("中悪", "Evil");
    else
        return _("大悪", "Chaotic");
}
