#pragma once
#ifndef VKMC_GENERATOR_H_
#define VKMC_GENERATOR_H_

#include <cstdint>

#include "../block/registry.h"
#include "../math/perlin.h"
#include "chunk.h"

class ChunkGenerator {
private:
  /// The number of smaples for each chunk
  static constexpr std::int32_t kSamples = 4;

public:

  ChunkGenerator(std::uint64_t seed) noexcept;

  void Generate(
      const BlockRegistry &registry,
      Chunk &chunk, std::int32_t x, std::int32_t z
  );

private:
  Perlin<512> perlin_;
};

#endif // VKMC_GENERATOR_H_
