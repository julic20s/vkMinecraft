#include <random>

#include "generator.h"

ChunkGenerator::ChunkGenerator(std::uint64_t seed) noexcept : perlin_(std::mt19937_64(seed)) {}

void ChunkGenerator::Generate(
    const BlockRegistry &registry,
    Chunk &chunk, std::int32_t x, std::int32_t z
) {
  auto dirt = registry.GetBlockId("dirt");
  auto grass = registry.GetBlockId("grass_block");

  x *= kSamples;
  z *= kSamples;

  // Sample distance between two blocks
  constexpr auto dx = float(kSamples) / Chunk::kLength;
  constexpr auto dz = float(kSamples) / Chunk::kLength;

  constexpr auto tiling_size = Chunk::kLength / kSamples;

  for (std::int32_t xx = 0; xx != kSamples; ++xx) {
    for (std::int32_t zz = 0; zz != kSamples; ++zz) {

      // Generate for each tiling
      for (int xxx = 0; xxx != tiling_size; ++xxx) {
        auto sample_x = xx + xxx * dx;
        for (int zzz = 0; zzz != tiling_size; ++zzz) {
          auto sample_z = zz + zzz * dz;

          // 0 ~ 1
          auto value = perlin_({x + sample_x, z + sample_z}) + .5f;
          constexpr auto max = Chunk::kLength / 8 - 1;
          std::int32_t block_y = Chunk::kLength / 2 + max * value;

          auto block_x = xx * tiling_size + xxx;
          auto block_z = zz * tiling_size + zzz;

          for (int i = Chunk::kLength - 1; i != block_y; --i) {
            chunk(block_x, i, block_z) = BlockRegistry::kAir;
          }
          chunk(block_x, block_y, block_z) = grass;
          if (block_y == 0) {
            continue;
          }

          while (--block_y >= 0) {
            chunk(block_x, block_y, block_z) = dirt;
          }
        }
      }
    }
  }
}
