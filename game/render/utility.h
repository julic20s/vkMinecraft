#pragma once
#ifndef VKMC_BASE_VULKAN_UTILITY_H_
#define VKMC_BASE_VULKAN_UTILITY_H_

#include <concepts>

#include <vulkan/vulkan.hpp>

namespace vku {

using namespace vk;

namespace internal {
[[nodiscard]] vk::CommandBuffer BeginOneTimeSubmit(Device, CommandPool);
void EndOneTimeSubmit(Device, CommandPool, CommandBuffer, Queue);
}

/// Create and begin an one-time command buffer, it will be submitted to queue at once.
/// The current thread will be blocked until the commands all done.
template <std::invocable<CommandBuffer> Func>
void OneTimeSubmit(Device device, CommandPool pool, Func func, Queue queue) {
  auto cmd = internal::BeginOneTimeSubmit(device, pool);
  func(cmd);
  internal::EndOneTimeSubmit(device, pool, cmd, queue);
}

void TransitionImageLayout(
    vk::CommandBuffer cmd, vk::Image image, vk::Format format,
    vk::ImageLayout from, vk::ImageLayout to
);

} // namespace vku

#endif // VKMC_BASE_VULKAN_UTILITY_H_
