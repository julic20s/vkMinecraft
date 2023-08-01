#pragma once
#ifndef VKMC_RENDER_BUFFER_H_
#define VKMC_RENDER_BUFFER_H_

#include <vulkan/vulkan.hpp>

struct Buffer {
  vk::Buffer buffer;
  vk::DeviceMemory memory;
};

struct MappingBuffer : Buffer {
  void *mapping;
};

struct Image {
  vk::DeviceMemory memory;
  vk::Image image;
  vk::ImageView view;
};

#endif // VKMC_RENDER_BUFFER_H_
