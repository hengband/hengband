#pragma once

enum class ArtifactKnowledgeMode {
    KNOWN, //!< 既知のアーティファクト
    IDENTIFIED, //!< 当該セーブデータで入手済のアーティファクト
};

class PlayerType;
void do_cmd_knowledge_artifacts(PlayerType *player_ptr, ArtifactKnowledgeMode mode);
void do_cmd_knowledge_objects(PlayerType *player_ptr, bool *need_redraw, bool visual_only, short direct_k_idx);
