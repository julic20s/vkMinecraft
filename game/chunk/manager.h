#pragma once
#ifndef VKMC_CHUNK_MANAGER_H_
#define VKMC_CHUNK_MANAGER_H_

#include <array>
#include <cstdint>
#include <unordered_map>

#include <event/publisher.h>

#include "chunk.h"
#include "generator.h"

class ChunkManager {
private:
  struct ChunkIdHash {
    std::uint64_t operator()(ChunkId val) const noexcept {
      std::uint64_t x64 = val.x;
      return ((x64 << 32) | val.y) ^ (x64 << 16);
    }
  };

  ChunkGenerator generator_;
  std::unordered_map<ChunkId, Chunk, ChunkIdHash> chunks_;
  std::array<ChunkId, 9> loaded_;
  bool valid_;
  EventPublisher *chunk_load_;
  EventPublisher *chunk_unload_;
  EventPublisher *chunk_update_;

public:
  ChunkManager(std::uint64_t seed);

  void LoadAutomatic(const BlockRegistry &registry, const glm::ivec3 &pos);

  BlockId GetBlock(const glm::ivec3 &pos) const noexcept;

  void SetBlock(const glm::ivec3 &pos, BlockId) noexcept;

  Chunk &Load(const BlockRegistry &registry, ChunkId);

  void Unload(ChunkId);
};

#endif // VKMC_CHUNK_MANAGER_H_
