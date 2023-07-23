#pragma once

#include "system/system-variables.h"

#include "object-enchant/tr-flags.h"

#include <memory>
#include <optional>
#include <unordered_map>

class ItemEntity;
class PlayerType;
class ISmithInfo;
struct essence_drain_type;
class ItemTester;
struct smith_data_type;

enum class SmithEffectType : int16_t;
enum class SmithCategoryType;
enum class SmithEssenceType : int16_t;
enum class RandomArtActType : short;

/*!
 * @brief 鍛冶クラス
 */
class Smith {
public:
    //! エッセンスとその抽出量を表すタプルのリスト
    using DrainEssenceResult = std::vector<std::tuple<SmithEssenceType, int>>;

    Smith(PlayerType *player_ptr);

    static const std::vector<SmithEssenceType> &get_essence_list();
    static concptr get_essence_name(SmithEssenceType essence);
    static std::vector<SmithEffectType> get_effect_list(SmithCategoryType category);
    static concptr get_effect_name(SmithEffectType effect);
    static std::string get_need_essences_desc(SmithEffectType effect);
    static std::vector<SmithEssenceType> get_need_essences(SmithEffectType effect);
    static int get_essence_consumption(SmithEffectType effect, const ItemEntity *o_ptr = nullptr);
    static std::unique_ptr<ItemTester> get_item_tester(SmithEffectType effect);
    static TrFlags get_effect_tr_flags(SmithEffectType effect);
    static std::optional<RandomArtActType> object_activation(const ItemEntity *o_ptr);
    static std::optional<SmithEffectType> object_effect(const ItemEntity *o_ptr);

    int get_essence_num_of_posessions(SmithEssenceType essence) const;
    DrainEssenceResult drain_essence(ItemEntity *o_ptr);
    bool add_essence(SmithEffectType effect, ItemEntity *o_ptr, int consumption);
    void erase_essence(ItemEntity *o_ptr) const;
    int get_addable_count(SmithEffectType smith_effect, const ItemEntity *o_ptr = nullptr) const;

    static constexpr int ESSENCE_AMOUNT_MAX = 20000;

private:
    static std::optional<const ISmithInfo *> find_smith_info(SmithEffectType effect);

    static const std::vector<SmithEssenceType> essence_list_order;
    static const std::unordered_map<SmithEssenceType, concptr> essence_to_name;
    static const std::vector<essence_drain_type> essence_drain_info_table;
    static const std::vector<std::shared_ptr<ISmithInfo>> smith_info_table;

    PlayerType *player_ptr;
    std::shared_ptr<smith_data_type> smith_data;
};
