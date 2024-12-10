#pragma once

enum class TerrainAction {
    DESTROY = 1,
    NO_DROP = 2,
    CRASH_GLASS = 3,
    MAX,
};

enum class TerrainCharacteristics;
class TerrainActionFlagChecker {
public:
    TerrainActionFlagChecker() = delete;
    static bool has(TerrainCharacteristics tc, TerrainAction taf);
};
