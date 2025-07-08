/*!
 * @brief モンスター死亡時の特殊処理switch (一般的な処理もdefaultで実施)
 * @date 2020/08/21
 * @author Hourier
 */

#include "monster-floor/special-death-switcher.h"
#include "artifact/fixed-art-generator.h"
#include "artifact/fixed-art-types.h"
#include "artifact/random-art-generator.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/floor-object.h"
#include "floor/floor-util.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-death-util.h"
#include "monster-floor/monster-death.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "object/object-kind-hook.h"
#include "spell/summon-types.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem/baseitem-allocation.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 死亡召喚に使用するモード選択
 * @param md_ptr モンスター撃破構造体への参照ポインタ
 * @return 撃破モンスターがPETであればPM_FORCE_PETを、CLONEであればPM_CLONEを立てる
 */
static BIT_FLAGS dead_mode(MonsterDeath *md_ptr)
{
    bool pet = md_ptr->m_ptr->is_pet();
    BIT_FLAGS mode = pet ? PM_FORCE_PET : PM_NONE;
    if (md_ptr->cloned) {
        mode = mode | PM_CLONE;
    }
    if (md_ptr->is_chameleon) {
        mode = mode | PM_CHAMELEON_FINAL_SUMMON;
    }

    return mode;
}

/*!
 * @brief 死亡時召喚処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param md_ptr モンスター撃破構造体への参照ポインタ
 * @param summon_data 召喚情報
 * @return プレイヤーが死亡時召喚を視認していればtrue, 視認しなければfalse, 召喚失敗時はtl::nullopt
 */
static tl::optional<bool> final_summon(PlayerType *player_ptr, MonsterDeath *md_ptr, const MonsterSummon &summon_data)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    if (floor.inside_arena || AngbandSystem::get_instance().is_phase_out() || !evaluate_percent(summon_data.probability)) {
        return tl::nullopt;
    }
    if (md_ptr->is_chameleon) {
        const auto &monraces = MonraceList::get_instance();
        if (!MonraceList::is_valid(summon_data.id) || (summon_data.id >= static_cast<MonraceId>(monraces.size()))) {
            return tl::nullopt;
        }
        const auto &monrace = monraces.get_monrace(summon_data.id);
        if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            return tl::nullopt;
        }
    }
    bool notice = false;
    const auto summon_num = rand_range(summon_data.min_num, summon_data.max_num);
    const auto p_pos = player_ptr->get_position();
    for (int i = 0; i < summon_num; i++) {
        auto m_pos = md_ptr->get_position();
        auto attempts = 0;
        for (; attempts < 100; attempts++) {
            m_pos = scatter(player_ptr, md_ptr->get_position(), summon_data.radius, PROJECT_NONE);
            if (floor.contains(m_pos) && floor.can_generate_monster_at(m_pos) && (p_pos != m_pos)) {
                break;
            }
        }
        if (attempts == 100) {
            break;
        }

        BIT_FLAGS mode = dead_mode(md_ptr);
        const auto summon_src = md_ptr->m_ptr->is_pet() ? -1 : md_ptr->m_idx;
        if (summon_named_creature(player_ptr, summon_src, m_pos.y, m_pos.x, summon_data.id, mode) && player_can_see_bold(player_ptr, m_pos.y, m_pos.x)) {
            notice = true;
        }
    }
    return notice;
}

static void on_dead_bloodletter(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    if (!md_ptr->drop_chosen_item || (randint1(100) >= 15)) {
        return;
    }

    ItemEntity item({ ItemKindType::SWORD, SV_BLADE_OF_CHAOS });
    ItemMagicApplier(player_ptr, &item, player_ptr->current_floor_ptr->object_level, AM_NO_FIXED_ART | md_ptr->mo_mode).execute();
    (void)drop_near(player_ptr, item, md_ptr->get_position());
}

static void on_dead_raal(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    if (!md_ptr->drop_chosen_item || (floor.dun_level <= 9)) {
        return;
    }

    const auto restrict = ((floor.dun_level > 49) && one_in_(5)) ? kind_is_good_book : kind_is_book;

    if (auto item = make_object(player_ptr, md_ptr->mo_mode, restrict)) {
        (void)drop_near(player_ptr, *item, md_ptr->get_position());
    }
}

static void on_dead_sacred_treasures(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    if ((player_ptr->ppersonality != PERSONALITY_LAZY) || !md_ptr->drop_chosen_item) {
        return;
    }

    constexpr static auto namake_equipments = {
        FixedArtifactId::NAMAKE_HAMMER,
        FixedArtifactId::NAMAKE_BOW,
        FixedArtifactId::NAMAKE_ARMOR
    };

    std::vector<FixedArtifactId> candidates;
    std::copy_if(namake_equipments.begin(), namake_equipments.end(), std::back_inserter(candidates),
        [](FixedArtifactId a_idx) {
            const auto &artifact = ArtifactList::get_instance().get_artifact(a_idx);
            return !artifact.is_generated;
        });

    if (candidates.empty()) {
        return;
    }

    const auto a_idx = rand_choice(candidates);
    create_named_art(player_ptr, a_idx, md_ptr->md_y, md_ptr->md_x);
}

static void on_dead_serpent(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    if (!md_ptr->drop_chosen_item) {
        return;
    }

    ItemEntity item_grond({ ItemKindType::HAFTED, SV_GROND });
    item_grond.fa_id = FixedArtifactId::GROND;
    ItemMagicApplier(player_ptr, &item_grond, -1, AM_GOOD | AM_GREAT).execute();
    (void)drop_near(player_ptr, item_grond, md_ptr->get_position());

    ItemEntity item_chaos({ ItemKindType::CROWN, SV_CHAOS });
    item_chaos.fa_id = FixedArtifactId::CHAOS;
    ItemMagicApplier(player_ptr, &item_chaos, -1, AM_GOOD | AM_GREAT).execute();
    (void)drop_near(player_ptr, item_chaos, md_ptr->get_position());
}

static void on_dead_death_sword(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    if (!md_ptr->drop_chosen_item) {
        return;
    }

    ItemEntity item({ ItemKindType::SWORD, randint1(2) });
    (void)drop_near(player_ptr, item, md_ptr->get_position());
}

static void on_dead_can_angel(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    auto is_drop_can = md_ptr->drop_chosen_item;
    auto is_silver = md_ptr->m_ptr->r_idx == MonraceId::A_SILVER;
    is_silver &= md_ptr->r_ptr->r_akills % 5 == 0;
    is_drop_can &= (md_ptr->m_ptr->r_idx == MonraceId::A_GOLD) || is_silver;
    if (!is_drop_can) {
        return;
    }

    ItemEntity item({ ItemKindType::CHEST, SV_CHEST_KANDUME });
    ItemMagicApplier(player_ptr, &item, player_ptr->current_floor_ptr->object_level, AM_NO_FIXED_ART).execute();
    (void)drop_near(player_ptr, item, md_ptr->get_position());
}

/*!
 * @brief 装備品の生成を試みる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param drop_mode ドロップ品の質
 * @param restrict ベースアイテム制約関数。特になければnullptrで良い
 * @return 生成した装備品。生成に失敗した場合はtl::nulloptを返す。
 * @todo 汎用的に使えそうだがどこかにいいファイルはないか？
 */
static tl::optional<ItemEntity> make_equipment(PlayerType *player_ptr, const BIT_FLAGS drop_mode, BaseitemRestrict restrict)
{
    if (!restrict) {
        // アイテムの制約を指定しない場合は、すべての装備品から選ぶ
        restrict = [](short bi_id) {
            const auto &item = BaseitemList::get_instance().get_baseitem(bi_id);
            return item.bi_key.is_wearable() && (item.bi_key.tval() != ItemKindType::CARD);
        };
    }

    return make_object(player_ptr, drop_mode, restrict);
}

/*!
 * @brief 死亡時ドロップとしてランダムアーティファクトのみを生成する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param md_ptr モンスター撃破構造体への参照ポインタ
 * @param restrict ベースアイテム制約関数。特になければnullptrで良い
 * @return なし
 * @details
 * 最初のアイテム生成でいきなり☆が生成された場合を除き、中途半端な☆ (例：呪われている)は生成しない.
 * このルーチンで★は生成されないので、★生成フラグのキャンセルも不要
 */
static void on_dead_random_artifact(PlayerType *player_ptr, MonsterDeath *md_ptr, BaseitemRestrict restrict)
{
    const auto drop_mode = md_ptr->mo_mode | AM_NO_FIXED_ART;
    while (true) {
        auto item = make_equipment(player_ptr, drop_mode, restrict);
        if (!item) {
            return;
        }

        if (item->is_random_artifact()) {
            (void)drop_near(player_ptr, *item, md_ptr->get_position());
            return;
        }

        if (item->is_ego()) {
            continue;
        }

        (void)become_random_artifact(player_ptr, &*item, false);
        auto is_good_random_art = !item->is_cursed();
        is_good_random_art &= item->to_h > 0;
        is_good_random_art &= item->to_d > 0;
        is_good_random_art &= item->to_a > 0;
        is_good_random_art &= item->pval > 0;
        if (is_good_random_art) {
            (void)drop_near(player_ptr, *item, md_ptr->get_position());
            return;
        }
    }
}

/*!
 * @brief マニマニのあくま撃破時メッセージ
 * @todo 死亡時の特殊メッセージを表示するだけの処理を複数作るなら、switch/case文に分けられるように汎用化すること
 */
static void on_dead_manimani(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    if (!is_seen(player_ptr, *md_ptr->m_ptr)) {
        return;
    }

    msg_print(_("どこからか声が聞こえる…「ハロー！　そして…グッドバイ！」", "Heard a voice from somewhere... 'Hello! And... good bye!'"));
}

static void drop_specific_item_on_dead(PlayerType *player_ptr, MonsterDeath *md_ptr, BaseitemRestrict restrict)
{
    if (auto item = make_object(player_ptr, md_ptr->mo_mode, restrict)) {
        (void)drop_near(player_ptr, *item, md_ptr->get_position());
    }
}

static void on_dead_mimics(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    if (!md_ptr->drop_chosen_item) {
        return;
    }

    switch (md_ptr->r_ptr->symbol_definition.character) {
    case '(':
        if (player_ptr->current_floor_ptr->dun_level <= 0) {
            return;
        }

        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_cloak);
        return;
    case '/':
        if (player_ptr->current_floor_ptr->dun_level <= 4) {
            return;
        }

        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_polearm);
        return;
    case '[':
        if (player_ptr->current_floor_ptr->dun_level <= 19) {
            return;
        }

        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_armor);
        return;
    case '\\':
        if (player_ptr->current_floor_ptr->dun_level <= 4) {
            return;
        }

        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_hafted);
        return;
    case '|':
        if (md_ptr->m_ptr->r_idx == MonraceId::STORMBRINGER) {
            return;
        }

        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_sword);
        return;
    case ']':
        if (player_ptr->current_floor_ptr->dun_level <= 19) {
            return;
        }

        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_boots);
        return;
    default:
        return;
    }
}

static void on_dead_swordfish(PlayerType *player_ptr, MonsterDeath *md_ptr, AttributeFlags attribute_flags)
{
    if (attribute_flags.has_not(AttributeType::COLD) || !md_ptr->drop_chosen_item || (randint1(100) >= 10)) {
        return;
    }

    drop_single_artifact(player_ptr, md_ptr, FixedArtifactId::FROZEN_SWORDFISH);
}

void switch_special_death(PlayerType *player_ptr, MonsterDeath *md_ptr, AttributeFlags attribute_flags)
{
    auto &monrace = MonraceList::get_instance().get_monrace(md_ptr->ap_r_ptr->idx);
    const auto &summon_list = monrace.get_final_summons();
    if (!summon_list.empty()) {
        auto do_message = false;
        for (auto &summon_data : summon_list) {
            if (auto notice = final_summon(player_ptr, md_ptr, summon_data)) {
                do_message |= *notice;
            }
        }
        if (do_message) {
            const auto monster_message = monrace.get_message(monrace.name, MonsterMessageType::SPEAK_SPAWN);
            if (monster_message) {
                msg_print(*monster_message);
                sound(SoundKind::SUMMON);
            }
        }
        return;
    }

    switch (md_ptr->ap_r_ptr->idx) {
    case MonraceId::BLOODLETTER:
        on_dead_bloodletter(player_ptr, md_ptr);
        return;
    case MonraceId::RAAL:
        on_dead_raal(player_ptr, md_ptr);
        return;
    case MonraceId::UNMAKER:
        (void)project(player_ptr, md_ptr->m_idx, 6, md_ptr->md_y, md_ptr->md_x, 100, AttributeType::CHAOS, PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
        break;
    case MonraceId::UNICORN_ORD:
    case MonraceId::MORGOTH:
    case MonraceId::ONE_RING:
        on_dead_sacred_treasures(player_ptr, md_ptr);
        return;
    case MonraceId::SERPENT:
        on_dead_serpent(player_ptr, md_ptr);
        return;
    case MonraceId::B_DEATH_SWORD:
        on_dead_death_sword(player_ptr, md_ptr);
        return;
    case MonraceId::A_GOLD:
    case MonraceId::A_SILVER:
        on_dead_can_angel(player_ptr, md_ptr);
        return;
    case MonraceId::ROLENTO:
        (void)project(player_ptr, md_ptr->m_idx, 3, md_ptr->md_y, md_ptr->md_x, Dice::roll(20, 10), AttributeType::FIRE, PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
        return;
    case MonraceId::CAIT_SITH:
        if (player_ptr->current_floor_ptr->dun_level <= 0 || md_ptr->is_chameleon) {
            return;
        }
        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_boots);
        return;
    case MonraceId::YENDOR_WIZARD_1:
        if (md_ptr->is_chameleon) {
            return;
        }
        on_dead_random_artifact(player_ptr, md_ptr, kind_is_amulet);
        return;
    case MonraceId::YENDOR_WIZARD_2:
        if (player_ptr->current_floor_ptr->dun_level <= 0 || md_ptr->is_chameleon) {
            return;
        }
        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_amulet);
        return;
    case MonraceId::MANIMANI:
        on_dead_manimani(player_ptr, md_ptr);
        return;
    case MonraceId::LOSTRINGIL:
        if (md_ptr->is_chameleon) {
            return;
        }
        on_dead_random_artifact(player_ptr, md_ptr, kind_is_sword);
        return;
    case MonraceId::SWORDFISH:
        on_dead_swordfish(player_ptr, md_ptr, attribute_flags);
        break;
    default:
        on_dead_mimics(player_ptr, md_ptr);
        return;
    }
}
