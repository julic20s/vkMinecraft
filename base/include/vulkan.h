#pragma once
#ifndef VKMC_BASE_VULKAN_H_
#define VKMC_BASE_VULKAN_H_

#include <cstdint>

#include <vulkan/vulkan.hpp>

namespace vulkan {

extern vk::PhysicalDevice gpu;
extern vk::Instance instance;
extern vk::SurfaceKHR surface;
extern vk::Device device;

[[nodiscard]] std::uint32_t GetGraphicsQueue();
[[nodiscard]] std::uint32_t GetPresentQueue();

} // namespace vulkan

#endif // VKMC_BASE_VULKAN_H_
