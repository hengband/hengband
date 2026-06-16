#pragma once

class BaseitemConfig;
class BaseitemService {
public:
    BaseitemService() = delete;

    static void initialize_baseitem_configs();
    static void reset_all_visuals();
    static const BaseitemConfig &pick_one_at_random();
};
