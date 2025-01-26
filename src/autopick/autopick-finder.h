#pragma once

#include <string>
#include <string_view>

class ItemEntity;
class PlayerType;
struct text_body_type;
int find_autopick_list(PlayerType *player_ptr, const ItemEntity *o_ptr);
void search_for_object(PlayerType *player_ptr, text_body_type *tb, const ItemEntity *o_ptr, bool forward);
void search_for_string(text_body_type *tb, std::string_view search_str, bool forward);

enum class AutopickSearchResult {
    BACK = -1,
    CANCEL = 0,
    FORWARD = 1,
};

class AutopickSearch {
public:
    AutopickSearch(ItemEntity *item_ptr, std::string_view search_str)
        : item_ptr(item_ptr)
        , search_str(search_str)
    {
    }

    ItemEntity *item_ptr;
    std::string search_str;
    AutopickSearchResult result = AutopickSearchResult::CANCEL;
};

AutopickSearch get_string_for_search(PlayerType *player_ptr, const AutopickSearch &as_initial);
bool get_object_for_search(PlayerType *player_ptr, AutopickSearch &as);
bool get_destroyed_object_for_search(PlayerType *player_ptr, AutopickSearch &as);
