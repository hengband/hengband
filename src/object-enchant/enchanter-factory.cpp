/*!
 * @brief アイテムの強化/弱化処理クラスを選択するファクトリクラス
 * @date 2022/03/22
 * @author Hourier
 */

#include "object-enchant/enchanter-factory.h"
#include "object-enchant/others/apply-magic-amulet.h"
#include "object-enchant/others/apply-magic-lite.h"
#include "object-enchant/others/apply-magic-others.h"
#include "object-enchant/others/apply-magic-ring.h"
#include "object-enchant/protector/apply-magic-armor.h"
#include "object-enchant/protector/apply-magic-boots.h"
#include "object-enchant/protector/apply-magic-cloak.h"
#include "object-enchant/protector/apply-magic-crown.h"
#include "object-enchant/protector/apply-magic-dragon-armor.h"
#include "object-enchant/protector/apply-magic-gloves.h"
#include "object-enchant/protector/apply-magic-hard-armor.h"
#include "object-enchant/protector/apply-magic-helm.h"
#include "object-enchant/protector/apply-magic-shield.h"
#include "object-enchant/protector/apply-magic-soft-armor.h"
#include "object-enchant/weapon/apply-magic-arrow.h"
#include "object-enchant/weapon/apply-magic-bow.h"
#include "object-enchant/weapon/apply-magic-digging.h"
#include "object-enchant/weapon/apply-magic-hafted.h"
#include "object-enchant/weapon/apply-magic-polearm.h"
#include "object-enchant/weapon/apply-magic-sword.h"
#include "object/tval-types.h"
#include "system/item-entity.h"

std::unique_ptr<EnchanterBase> EnchanterFactory::create_enchanter(PlayerType *player_ptr, ItemEntity *o_ptr, int lev, int power)
{
    switch (o_ptr->bi_key.tval()) {
    case ItemKindType::SHOT:
    case ItemKindType::ARROW:
    case ItemKindType::BOLT:
        return std::make_unique<ArrowEnchanter>(player_ptr, o_ptr, lev, power);
    case ItemKindType::BOW:
        return std::make_unique<BowEnchanter>(player_ptr, o_ptr, lev, power);
    case ItemKindType::DIGGING:
        return std::make_unique<DiggingEnchanter>(player_ptr, o_ptr, lev, power);
    case ItemKindType::HAFTED:
        return std::make_unique<HaftedEnchanter>(player_ptr, o_ptr, lev, power);
    case ItemKindType::POLEARM:
        return std::make_unique<PolearmEnchanter>(player_ptr, o_ptr, lev, power);
    case ItemKindType::SWORD:
        return std::make_unique<SwordEnchanter>(player_ptr, o_ptr, lev, power);
    case ItemKindType::BOOTS:
        return std::make_unique<BootsEnchanter>(player_ptr, o_ptr, lev, power);
    case ItemKindType::GLOVES:
        return std::make_unique<GlovesEnchanter>(player_ptr, o_ptr, lev, power);
    case ItemKindType::HELM:
        return std::make_unique<HelmEnchanter>(player_ptr, o_ptr, lev, power);
    case ItemKindType::CROWN:
        return std::make_unique<CrownEnchanter>(player_ptr, o_ptr, lev, power);
    case ItemKindType::SHIELD:
        return std::make_unique<ShieldEnchanter>(player_ptr, o_ptr, lev, power);
    case ItemKindType::CLOAK:
        return std::make_unique<CloakEnchanter>(player_ptr, o_ptr, lev, power);
    case ItemKindType::SOFT_ARMOR:
        return std::make_unique<SoftArmorEnchanter>(player_ptr, o_ptr, lev, power);
    case ItemKindType::HARD_ARMOR:
        return std::make_unique<HardArmorEnchanter>(player_ptr, o_ptr, lev, power);
    case ItemKindType::DRAG_ARMOR:
        return std::make_unique<DragonArmorEnchanter>(player_ptr, o_ptr, lev, power);
    case ItemKindType::LITE:
        return std::make_unique<LiteEnchanter>(player_ptr, o_ptr, power);
    case ItemKindType::AMULET:
        return std::make_unique<AmuletEnchanter>(player_ptr, o_ptr, lev, power);
    case ItemKindType::RING:
        return std::make_unique<RingEnchanter>(player_ptr, o_ptr, lev, power);
    default:
        return std::make_unique<OtherItemsEnchanter>(player_ptr, o_ptr);
    }
}
