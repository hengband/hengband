#pragma once

#include <tl/optional.hpp>

enum class RumorRarity;
class RumorDefinition;
class RumorService {
public:
    RumorService() = delete;
    RumorService(RumorService &&) = delete;
    RumorService(const RumorService &) = delete;
    RumorService &operator=(const RumorService &) = delete;
    RumorService &operator=(RumorService &&) = delete;

    static void initialize();
    static void retouch();
    static const RumorDefinition &pick_rumor(tl::optional<RumorRarity> rarity);
};
