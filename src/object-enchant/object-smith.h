#pragma once

#include "system/system-variables.h"

#include "object-enchant/tr-flags.h"

#include <memory>
#include <optional>
#include <unordered_map>

struct object_type;
struct player_type;
class ISmithInfo;
struct essence_drain_type;
class ItemTester;
struct smith_data_type;

enum class SmithEffect : int16_t;
enum class SmithCategory;
enum class SmithEssence : int16_t;
enum random_art_activation_type : uint8_t;

/*!
 * @brief 鍛冶クラス
 */
class Smith {
public:
    //! エッセンスとその抽出量を表すタプルのリスト
    using DrainEssenceResult = std::vector<std::tuple<SmithEssence, int>>;

    Smith(player_type *player_ptr);

    static const std::vector<SmithEssence> &get_essence_list();
    static concptr get_essence_name(SmithEssence essence);
    static std::vector<SmithEffect> get_effect_list(SmithCategory category);
    static concptr get_effect_name(SmithEffect effect);
    static std::string get_need_essences_desc(SmithEffect effect);
    static std::vector<SmithEssence> get_need_essences(SmithEffect effect);
    static int get_essence_consumption(SmithEffect effect, const object_type *o_ptr = nullptr);
    static std::unique_ptr<ItemTester> get_item_tester(SmithEffect effect);
    static TrFlags get_effect_tr_flags(SmithEffect effect);
    static std::optional<random_art_activation_type> object_activation(const object_type *o_ptr);
    static std::optional<SmithEffect> object_effect(const object_type *o_ptr);

    int get_essence_num_of_posessions(SmithEssence essence) const;
    DrainEssenceResult drain_essence(object_type *o_ptr);
    bool add_essence(SmithEffect effect, object_type *o_ptr, int consumption);
    void erase_essence(object_type *o_ptr) const;
    int get_addable_count(SmithEffect smith_effect, const object_type *o_ptr = nullptr) const;

    static constexpr int ESSENCE_AMOUNT_MAX = 20000;

private:
    static std::optional<const ISmithInfo *> find_smith_info(SmithEffect effect);

    static const std::vector<SmithEssence> essence_list_order;
    static const std::unordered_map<SmithEssence, concptr> essence_to_name;
    static const std::vector<essence_drain_type> essence_drain_info_table;
    static const std::vector<std::shared_ptr<ISmithInfo>> smith_info_table;

    player_type *player_ptr;
    std::shared_ptr<smith_data_type> smith_data;
};
