#pragma once
#ifndef VKMC_GAMEPLAY_CHUNK_H_
#define VKMC_GAMEPLAY_CHUNK_H_

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <unordered_map>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "../block/types.h"

using ChunkId = glm::ivec2;

class Chunk {
private:
  static constexpr std::size_t kChunkSizePow = 5; // 2^5 = 32
  static_assert(
      kChunkSizePow <= std::numeric_limits<std::uint8_t>::digits,
      "The size of chunk is too large!"
  );

  static constexpr std::size_t kChunkPrimitives = std::size_t(1) << (kChunkSizePow * 3);

public:
  static constexpr std::size_t kLength = 1 << kChunkSizePow;

  [[nodiscard]] static constexpr glm::vec<3, std::uint8_t>
  GetPositionByIndex(std::uint32_t i) noexcept {
    constexpr auto k = kChunkSizePow;
    constexpr auto m = (1 << k) - 1;
    // index is [y][x][z], swizzle to [x][y][z]
    return {(i >> k) & m, (i >> (k * 2)) & m, i & m};
  }

  [[nodiscard]] static constexpr ChunkId
  GetChunkIdFromWorldPosition(const glm::ivec3 &world) noexcept {
    return {world.x >> kChunkSizePow, world.z >> kChunkSizePow};
  }

  [[nodiscard]] static constexpr glm::vec<3, std::uint8_t>
  GetPositionInChunk(const glm::ivec3 &world) noexcept {
    constexpr auto m = (1 << kChunkSizePow) - 1;
    return {world.x & m, world.y & m, world.z & m};
  }

  [[nodiscard]] const std::uint32_t &
  operator()(std::uint8_t x, std::uint8_t y, std::uint8_t z) const {
    return data_[y][x][z];
  }

  [[nodiscard]] std::uint32_t &
  operator()(std::uint8_t x, std::uint8_t y, std::uint8_t z) {
    return data_[y][x][z];
  }

  [[nodiscard]] std::uint32_t &
  operator()(const glm::vec<3, std::uint8_t> &pos) {
    return operator()(pos.x, pos.y, pos.z);
  }

  [[nodiscard]] const std::uint32_t &
  operator()(const glm::vec<3, std::uint8_t> &pos) const {
    return operator()(pos.x, pos.y, pos.z);
  }

private:
  BlockId data_[kLength][kLength][kLength];
};

#endif // VKMC_GAMEPLAY_CHUNK_H_
