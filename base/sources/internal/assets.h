#pragma once
#ifndef VKMC_BASE_INTERNAL_ASSETS_H_
#define VKMC_BASE_INTERNAL_ASSETS_H_

#include <filesystem>
#include <functional>

#include <assets/load.h>

namespace assets::internal {

void LoadAssetsFile(std::filesystem::path &&file);

void UnloadAssetsFile();

/// Load assets and automatic call given function when Unload()
std::span<const std::byte> Load(
    std::string_view name,
    std::function<void(std::string_view)> unload
);

} // namespace assets::internal

#endif // VKMC_BASE_INTERNAL_ASSETS_H_
