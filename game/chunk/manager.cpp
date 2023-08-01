#include "manager.h"

#include <application.h>
#include <bitset>
#include <ranges>
#include <event.h>

#include "manager.h"
#include "../events.h"

ChunkManager::ChunkManager(std::uint64_t seed) : generator_(seed), valid_(false) {
  chunk_load_ = &events::global.RegisterEvent(events::kChunkLoaded);
  chunk_unload_ = &events::global.RegisterEvent(events::kChunkUnloaded);
  chunk_update_ = &events::global.RegisterEvent(events::kChunkUpdate);
}

void ChunkManager::LoadAutomatic(const BlockRegistry &registry, const glm::ivec3 &pos) {
  auto chunk = Chunk::GetChunkIdFromWorldPosition(pos);
  std::array<glm::ivec2, 9> new_load;
  std::bitset<9> keep;
  {
    auto it = new_load.begin();
    constexpr auto offset = std::array{-1, 0, 1};
    for (auto x : offset) {
      for (auto y : offset) {
        *it = {chunk.x + x, chunk.y + y};
        if (valid_) {
          int i = 0;
          for (auto &l : loaded_) {
            if (l == *it) {
              keep[i] = true;
            }
            ++i;
          }
        }
        ++it;
      }
    }
  }

  for (int i = 0; i != loaded_.size(); ++i) {
    if (!keep[i]) {
      Unload(loaded_[i]);
    }
  }

  for (auto &c : new_load) {
    auto &chunk = Load(registry, c);
  }
  loaded_ = new_load;
  valid_ = true;
}

Chunk &ChunkManager::Load(const BlockRegistry &registry, ChunkId id) {
  auto [it, add] = chunks_.try_emplace(id);
  if (add) {
    generator_.Generate(registry, it->second, id.x, id.y);
    chunk_load_->EmitArgs(id, &it->second);
  }
  return it->second;
}

void ChunkManager::Unload(ChunkId id) {
  if (chunks_.erase(id)) {
    chunk_unload_->EmitArgs(id);
  }
}

BlockId ChunkManager::GetBlock(const glm::ivec3 &position) const noexcept {
  if (position.y < 0 || Chunk::kLength <= position.y) {
    return blocks::kAir;
  }
  auto id = Chunk::GetChunkIdFromWorldPosition(position);
  auto it = chunks_.find(id);
  if (it == chunks_.end()) {
    return blocks::kAir;
  }

  return it->second(Chunk::GetPositionInChunk(position));
}

void ChunkManager::SetBlock(const glm::ivec3 &position, BlockId block) noexcept {
  if (position.y < 0 || Chunk::kLength <= position.y) {
    return;
  }
  auto id = Chunk::GetChunkIdFromWorldPosition(position);
  auto it = chunks_.find(id);
  if (it == chunks_.end()) {
    return;
  }

  chunk_update_->EmitArgs(id, &it->second);

  it->second(Chunk::GetPositionInChunk(position)) = block;
}
