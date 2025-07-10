#include "player-info/alignment.h"
#include "artifact/fixed-art-types.h"
#include "avatar/avatar.h"
#include "game-option/text-display-options.h"
#include "inventory/inventory-slot-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "player-info/equipment-info.h"
#include "player-info/race-info.h"
#include "system/angband-exceptions.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include <array>
#include <fmt/format.h>

PlayerAlignment::PlayerAlignment(PlayerType *player_ptr)
{
    this->player_ptr = player_ptr;
}

/*!
 * @brief プレイヤーの抽象的善悪アライメントの表記を返す。 / Return alignment title
 * @param with_value 徳の情報と一緒に表示する時だけtrue
 * @return アライメントの表記を返す。
 */
std::string PlayerAlignment::get_alignment_description(bool with_value)
{
    const auto s = this->alignment_label();
    if (with_value || show_actual_value) {
        return fmt::format(_("{}({})", "{} ({})"), s, this->player_ptr->alignment);
    }

    return s;
}

/*
 * @brief アライメント情報の更新
 * @param なし
 */
void PlayerAlignment::update_alignment()
{
    this->reset_alignment();
    const auto &floor = *this->player_ptr->current_floor_ptr;
    for (MONSTER_IDX m_idx = floor.m_max - 1; m_idx >= 1; m_idx--) {
        const auto &monster = floor.m_list[m_idx];
        if (!monster.is_valid()) {
            continue;
        }
        const auto &monrace = monster.get_monrace();

        if (!monster.is_pet()) {
            continue;
        }

        if (monrace.kind_flags.has(MonsterKindType::GOOD)) {
            this->bias_good_alignment(monrace.level);
        }

        if (monrace.kind_flags.has(MonsterKindType::EVIL)) {
            this->bias_evil_alignment(monrace.level);
        }
    }

    switch (this->player_ptr->mimic_form) {
    case MimicKindType::NONE:
        switch (this->player_ptr->prace) {
        case PlayerRaceType::ARCHON:
            this->bias_good_alignment(200);
            break;
        case PlayerRaceType::BALROG:
            this->bias_evil_alignment(200);
            break;
        default:
            break;
        }

        break;
    case MimicKindType::DEMON:
        this->bias_evil_alignment(200);
        break;
    case MimicKindType::DEMON_LORD:
        this->bias_evil_alignment(200);
        break;
    case MimicKindType::VAMPIRE:
        break;
    default:
        THROW_EXCEPTION(std::logic_error, "Invalid MimicKindType was specified!");
    }

    if (this->player_ptr->is_wielding(FixedArtifactId::IRON_BALL)) {
        this->bias_evil_alignment(1000);
    }

    int j = 0;
    std::array<int, 2> neutral{};
    for (int i = 0; i < 8; i++) {
        switch (this->player_ptr->vir_types[i]) {
        case Virtue::JUSTICE:
            this->bias_good_alignment(this->player_ptr->virtues[i] * 2);
            break;
        case Virtue::CHANCE:
            break;
        case Virtue::NATURE:
        case Virtue::HARMONY:
            neutral[j++] = i;
            break;
        case Virtue::UNLIFE:
            this->bias_evil_alignment(this->player_ptr->virtues[i]);
            break;
        default:
            this->bias_good_alignment(this->player_ptr->virtues[i]);
            break;
        }
    }

    for (int i = 0; i < j; i++) {
        if (this->player_ptr->alignment > 0) {
            this->bias_evil_alignment(this->player_ptr->virtues[neutral[i]] / 2);
            if (this->player_ptr->alignment < 0) {
                this->reset_alignment();
            }
        } else if (this->player_ptr->alignment < 0) {
            this->bias_good_alignment(this->player_ptr->virtues[neutral[i]] / 2);
            if (this->player_ptr->alignment > 0) {
                this->reset_alignment();
            }
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
std::string PlayerAlignment::alignment_label() const
{
    if (this->player_ptr->alignment > 150) {
        return _("大善", "Lawful");
    } else if (this->player_ptr->alignment > 50) {
        return _("中善", "Good");
    } else if (this->player_ptr->alignment > 10) {
        return _("小善", "Neutral Good");
    } else if (this->player_ptr->alignment > -11) {
        return _("中立", "Neutral");
    } else if (this->player_ptr->alignment > -51) {
        return _("小悪", "Neutral Evil");
    } else if (this->player_ptr->alignment > -151) {
        return _("中悪", "Evil");
    } else {
        return _("大悪", "Chaotic");
    }
}
