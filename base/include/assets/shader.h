#pragma once
#ifndef VKMC_BASE_ASSETS_SHADER_H_
#define VKMC_BASE_ASSETS_SHADER_H_

#include <string_view>

#include <vulkan/vulkan.hpp>

namespace assets {

// Load a vulkan shder module from default.assets
[[nodiscard]] vk::ShaderModule LoadShader(std::string_view name);

} // namespace assets

#endif // VKMC_BASE_ASSETS_SHADER_H_
