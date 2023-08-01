#pragma once
#ifndef VKMC_GAMEPLAY_BLOCK_REGISTRY_H_
#define VKMC_GAMEPLAY_BLOCK_REGISTRY_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "types.h"

class BlockRegistry {
public:

  static constexpr std::size_t kBlockTextureSize = 16;

  /// The reserved block id for air
  static constexpr BlockId kAir = ~BlockId(0);

  BlockRegistry();

  /// Register a new block, will index its name and texture from assets
  BlockId RegisterBlock(const std::string &name);

  /// Get the id of a registered block
  [[nodiscard]] BlockId GetBlockId(const std::string &name) const;

  /// Get the texture of given block face
  [[nodiscard]] TextureId GetFaceTextureId(BlockId block_id, FaceDirection) const;

  [[nodiscard]] std::size_t GetMegaBlocksTextureWidth() const noexcept;

  [[nodiscard]] std::size_t GetMegaBlocksTextureHeight() const noexcept;

  [[nodiscard]] std::size_t GetMegaBlocksTextureDepth() const noexcept {
    return 4;
  }

  [[nodiscard]] std::size_t GetTextureCount() const noexcept {
    return texture_ids_.size();
  }

  void MakeMegaBlocksTexture(void *dst) const;

private:
  TextureId GetTextureId(const std::string &name) const noexcept;

  std::map<std::string, std::uint32_t> block_ids_;
  std::vector<std::array<std::uint32_t, 6>> blocks_;
  mutable std::map<std::string, std::uint32_t> texture_ids_;
};

#endif // VKMC_GAMEPLAY_BLOCK_REGISTRY_H_
