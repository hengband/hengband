#pragma once

#include "system/system-variables.h"

#include "object-enchant/tr-flags.h"

#include <memory>
#include <optional>

struct object_type;
struct player_type;
class ItemTester;

enum class SmithEffect;
enum class SmithCategory;
enum class SmithEssence;
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
    static std::optional<random_art_activation_type> get_effect_activation(SmithEffect effect);
    static std::optional<SmithEffect> object_effect(const object_type *o_ptr);

    int get_essence_num_of_posessions(SmithEssence essence) const;
    DrainEssenceResult drain_essence(object_type *o_ptr);
    bool add_essence(SmithEffect effect, object_type *o_ptr, int consumption);
    void erase_essence(object_type *o_ptr) const;
    int get_addable_count(SmithEffect smith_effect, int item_number) const;

private:
    player_type *player_ptr;
};
