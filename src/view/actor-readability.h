#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>

namespace original_tiles {
using ActorTile = std::array<uint32_t, 32 * 32>;
constexpr uint32_t ACTOR_TRANSPARENT = 0x00ff00ff;

inline ActorTile readable_actor_tile(const ActorTile &source, bool player)
{
    auto left = 32;
    auto top = 32;
    auto right = -1;
    auto bottom = -1;
    for (auto y = 0; y < 32; ++y) {
        for (auto x = 0; x < 32; ++x) {
            if ((source[y * 32 + x] & 0xffffff) != ACTOR_TRANSPARENT) {
                left = std::min(left, x);
                top = std::min(top, y);
                right = std::max(right, x);
                bottom = std::max(bottom, y);
            }
        }
    }
    ActorTile body;
    body.fill(ACTOR_TRANSPARENT);
    if (right < left) {
        return body;
    }
    const auto width = right - left + 1;
    const auto height = bottom - top + 1;
    const auto extent = player ? 30 : 28;
    const auto longest = std::max(width, height);
    const auto out_width = std::max(1, width * extent / longest);
    const auto out_height = std::max(1, height * extent / longest);
    const auto offset_x = (32 - out_width) / 2;
    const auto offset_y = (32 - out_height) / 2;
    for (auto y = 0; y < out_height; ++y) {
        for (auto x = 0; x < out_width; ++x) {
            const auto sx = left + (2 * x + 1) * width / (2 * out_width);
            const auto sy = top + (2 * y + 1) * height / (2 * out_height);
            const auto pixel = source[sy * 32 + sx] & 0xffffff;
            if (pixel == ACTOR_TRANSPARENT) {
                continue;
            }
            const auto maximum = std::max({ pixel & 255, (pixel >> 8) & 255, (pixel >> 16) & 255 });
            auto output = uint32_t{ 0x00141820 };
            if (maximum >= 48) {
                const auto level = maximum < 80 ? 104U : maximum < 128 ? 144U
                                                     : maximum < 176   ? 184U
                                                     : maximum < 224   ? 224U
                                                                       : 248U;
                output = 0;
                for (auto shift = 0; shift < 24; shift += 8) {
                    const auto value = ((pixel >> shift) & 255) * level / maximum;
                    output |= std::min(248U, ((value + 8) / 16) * 16) << shift;
                }
            }
            body[(offset_y + y) * 32 + offset_x + x] = output;
        }
    }
    auto result = body;
    const uint32_t outline = player ? 0x00f0d890 : 0x009cacb8;
    for (auto y = 0; y < 32; ++y) {
        for (auto x = 0; x < 32; ++x) {
            if (body[y * 32 + x] != ACTOR_TRANSPARENT) {
                continue;
            }
            for (auto dy = -1; dy <= 1; ++dy) {
                for (auto dx = -1; dx <= 1; ++dx) {
                    const auto nx = x + dx;
                    const auto ny = y + dy;
                    if (nx >= 0 && nx < 32 && ny >= 0 && ny < 32 && body[ny * 32 + nx] != ACTOR_TRANSPARENT) {
                        result[y * 32 + x] = outline;
                    }
                }
            }
        }
    }
    return result;
}

inline bool make_actor_region_readable(std::span<uint32_t> pixels, int width, int height, int base_row, int count, bool players, bool legacy_avatar = false)
{
    if (width != 512 || height < 0 || pixels.size() != static_cast<size_t>(width) * static_cast<size_t>(height) || base_row < 0 || count < 0) {
        return false;
    }
    const auto end = (static_cast<uint64_t>(base_row) + (static_cast<uint64_t>(count) + 15) / 16) * 32;
    if (end > static_cast<uint64_t>(height)) {
        return false;
    }
    for (auto index = 0; index < count; ++index) {
        const auto sx = index % 16 * 32;
        const auto sy = (base_row + index / 16) * 32;
        ActorTile source;
        for (auto y = 0; y < 32; ++y) {
            for (auto x = 0; x < 32; ++x) {
                source[y * 32 + x] = pixels[(sy + y) * width + sx + x];
            }
        }
        // 旧96画像バンクだけは、各表示色の組にプレイヤー画像を含む。
        const auto output = readable_actor_tile(source, players || (legacy_avatar && index % 96 == 76));
        for (auto y = 0; y < 32; ++y) {
            for (auto x = 0; x < 32; ++x) {
                pixels[(sy + y) * width + sx + x] = output[y * 32 + x];
            }
        }
    }
    return true;
}
}
