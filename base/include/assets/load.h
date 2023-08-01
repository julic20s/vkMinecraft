#pragma once
#ifndef VKMC_BASE_ASSETS_BYTES_H_
#define VKMC_BASE_ASSETS_BYTES_H_

#include <cstddef>
#include <span>
#include <string_view>

namespace assets {

/// Load assets from default.assets
[[nodiscard]] std::span<const std::byte> Load(std::string_view name);

/// Unload assets
void Unload(std::string_view name);

} // namespace assets

#endif // VKMC_BASE_ASSETS_BYTES_H_
