#pragma once

class DisplaySymbol;
class BaseitemConfig;
class BaseitemService {
public:
    BaseitemService() = delete;

    static void initialize_baseitem_configs();
    static void reset_all_visuals();
    static const BaseitemConfig &pick_one_at_random();
    static const DisplaySymbol &get_dummy_symbol();
};
