#pragma once

class RumorService {
public:
    RumorService() = delete;
    RumorService(RumorService &&) = delete;
    RumorService(const RumorService &) = delete;
    RumorService &operator=(const RumorService &) = delete;
    RumorService &operator=(RumorService &&) = delete;

    static void initialize();
    static void retouch();
};
