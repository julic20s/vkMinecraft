#pragma once
#ifndef VKMC_BASE_ASSETS_IMAGE_H_
#define VKMC_BASE_ASSETS_IMAGE_H_

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace assets {

struct Image {
  std::uint32_t width;
  std::uint32_t height;
  const std::byte *data;
};

/// Load image from default.assets
[[nodiscard]] Image LoadImage(std::string_view name);

}; // namespace assets

#endif // VKMC_BASE_ASSETS_IMAGE_H_
