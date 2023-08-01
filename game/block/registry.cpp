#include <ranges>
#include <stdexcept>


#include <assets/image.h>
#include <assets/json.h>
#include <assets/load.h>

#include "registry.h"

static FaceDirection ConvertToFaceDirection(std::string_view name) {
  if (name == "north") {
    return FaceDirection::kNorth;
  } else if (name == "south") {
    return FaceDirection::kSouth;
  } else if (name == "west") {
    return FaceDirection::kWest;
  } else if (name == "east") {
    return FaceDirection::kEast;
  } else if (name == "top") {
    return FaceDirection::kTop;
  } else if (name == "bottom") {
    return FaceDirection::kBottom;
  }
  throw std::runtime_error("Unknown face!");
}

BlockRegistry::BlockRegistry() {
  RegisterBlock("grass_block");
  RegisterBlock("dirt");
}

std::uint32_t BlockRegistry::RegisterBlock(const std::string &name) {
  auto [it, add] = block_ids_.try_emplace(name, std::uint32_t(block_ids_.size()));
  if (!add) {
    throw std::runtime_error(name + " was registered!");
  }

  auto file = "blocks/" + name + ".json";
  auto &json = assets::LoadJson(file);

  auto &block = blocks_.emplace_back();

  for (auto &member : json["faces"].items()) {
    auto direction = ConvertToFaceDirection(member.key());
    block[std::uint32_t(direction)] = GetTextureId(member.value());
  }

  assets::Unload(file);
  return blocks_.size();
}

std::uint32_t BlockRegistry::GetBlockId(const std::string &name) const {
  auto it = block_ids_.find(name);
  if (it == block_ids_.end()) {
    throw std::runtime_error(name + " not found!");
  }
  return it->second;
}

std::size_t BlockRegistry::GetMegaBlocksTextureWidth() const noexcept {
  return kBlockTextureSize;
}

std::size_t BlockRegistry::GetMegaBlocksTextureHeight() const noexcept {
  return kBlockTextureSize * texture_ids_.size();
}

void BlockRegistry::MakeMegaBlocksTexture(void *dst) const {
  std::byte *bytes = reinterpret_cast<std::byte *>(dst);
  constexpr auto image_size = kBlockTextureSize * kBlockTextureSize * 4;
  for (auto &name : texture_ids_ | std::views::keys) {
    auto image = assets::LoadImage(name);
    if (image.width != kBlockTextureSize || image.height != kBlockTextureSize) {
      throw std::runtime_error("Block texture must be 16 x 16!");
    }
    std::memcpy(bytes, image.data, image_size);
    bytes += image_size;
    assets::Unload(name);
  }
}

TextureId BlockRegistry::GetFaceTextureId(std::uint32_t block_id, FaceDirection dir) const {
  return blocks_[block_id][std::uint32_t(dir)];
}

std::uint32_t BlockRegistry::GetTextureId(const std::string &name) const noexcept {
  return texture_ids_.try_emplace(name, std::uint32_t(texture_ids_.size())).first->second;
}
