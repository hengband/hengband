#include "system/artifact/artifact-record.h"
#include "artifact/fixed-art-types.h"
#include "system/angband-exceptions.h"
#include "util/enum-converter.h"
#include <fmt/format.h>

const tl::optional<short> &ArtifactRecord::get_floor_id() const
{
    return this->floor_id;
}

bool ArtifactRecord::get_generated() const
{
    return this->is_generated;
}

bool ArtifactRecord::get_identified() const
{
    return this->is_identified;
}

bool ArtifactRecord::get_known() const
{
    return this->is_known;
}

bool ArtifactRecord::can_generate() const
{
    return !this->is_generated && !this->is_quest_reward;
}

void ArtifactRecord::set_floor_id(const tl::optional<short> &id)
{
    this->floor_id = id;
}

void ArtifactRecord::set_generated(bool new_state)
{
    this->is_generated = new_state;
}

void ArtifactRecord::set_identified(bool new_state)
{
    this->is_identified = new_state;
}

void ArtifactRecord::set_known(bool new_state)
{
    this->is_known = new_state;
}

void ArtifactRecord::set_quest_reward(bool new_state)
{
    this->is_quest_reward = new_state;
}

ArtifactRecords ArtifactRecords::instance{};

ArtifactRecords &ArtifactRecords::get_instance()
{
    return instance;
}

void ArtifactRecords::initialize(size_t size)
{
    if (!this->records.empty()) {
        for (auto &[_, record] : this->records) {
            record.set_floor_id(tl::nullopt);
            record.set_generated(false);
            record.set_identified(false);
            record.set_known(false);
            record.set_quest_reward(false);
        }

        return;
    }

    for (size_t i = 1; i <= size; i++) {
        this->records.emplace(i2enum<FixedArtifactId>(i), ArtifactRecord());
    }
}

const tl::optional<short> &ArtifactRecords::get_floor_id(FixedArtifactId fa_id) const
{
    this->validate_fixed_artifact_id(fa_id);
    if (fa_id == FixedArtifactId::NONE) {
        static tl::optional<short> dummy;
        return dummy;
    }

    return this->records.at(fa_id).get_floor_id();
}

bool ArtifactRecords::get_generated(FixedArtifactId fa_id) const
{
    this->validate_fixed_artifact_id(fa_id);
    if (fa_id == FixedArtifactId::NONE) {
        return false;
    }

    return this->records.at(fa_id).get_generated();
}

bool ArtifactRecords::get_identified(FixedArtifactId fa_id) const
{
    this->validate_fixed_artifact_id(fa_id);
    if (fa_id == FixedArtifactId::NONE) {
        return false;
    }

    return this->records.at(fa_id).get_identified();
}

bool ArtifactRecords::get_known(FixedArtifactId fa_id) const
{
    this->validate_fixed_artifact_id(fa_id);
    if (fa_id == FixedArtifactId::NONE) {
        return false;
    }

    return this->records.at(fa_id).get_known();
}

bool ArtifactRecords::can_generate(FixedArtifactId fa_id) const
{
    this->validate_fixed_artifact_id(fa_id);
    if (fa_id == FixedArtifactId::NONE) {
        return false;
    }

    return this->records.at(fa_id).can_generate();
}

void ArtifactRecords::set_floor_id(FixedArtifactId fa_id, const tl::optional<short> &id)
{
    this->validate_fixed_artifact_id(fa_id);
    if (fa_id == FixedArtifactId::NONE) {
        return;
    }

    this->records[fa_id].set_floor_id(id);
}

void ArtifactRecords::set_generated(FixedArtifactId fa_id, bool new_state)
{
    this->validate_fixed_artifact_id(fa_id);
    if (fa_id == FixedArtifactId::NONE) {
        return;
    }

    return this->records[fa_id].set_generated(new_state);
}

void ArtifactRecords::set_identified(FixedArtifactId fa_id, bool new_state)
{
    this->validate_fixed_artifact_id(fa_id);
    if (fa_id == FixedArtifactId::NONE) {
        return;
    }

    return this->records[fa_id].set_identified(new_state);
}

void ArtifactRecords::set_known(FixedArtifactId fa_id, bool new_state)
{
    this->validate_fixed_artifact_id(fa_id);
    if (fa_id == FixedArtifactId::NONE) {
        return;
    }

    return this->records[fa_id].set_known(new_state);
}

void ArtifactRecords::set_quest_reward(FixedArtifactId fa_id, bool new_state)
{
    this->validate_fixed_artifact_id(fa_id);
    if (fa_id == FixedArtifactId::NONE) {
        return;
    }

    return this->records[fa_id].set_quest_reward(new_state);
}

void ArtifactRecords::reset_all_without_knowledge()
{
    for (auto &[_, record] : this->records) {
        record.set_floor_id(tl::nullopt);
        record.set_generated(false);
        record.set_identified(false);
        record.set_quest_reward(false);
    }
}

void ArtifactRecords::validate_fixed_artifact_id(FixedArtifactId fa_id) const
{
    if ((fa_id < FixedArtifactId::NONE) || (enum2i(fa_id) > static_cast<short>(this->records.size()))) {
        THROW_EXCEPTION(std::out_of_range, fmt::format("Invalid Fixed Artifact ID: {}", enum2i(fa_id)));
    }
}
