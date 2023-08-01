#pragma once
#ifndef VKMC_BASE_INTERNAL_VULKAN_H_
#define VKMC_BASE_INTERNAL_VULKAN_H_

#include <string>

#include <vulkan.h>

#include <GLFW/glfw3.h>

namespace vulkan::internal {

void Initialize(GLFWwindow *, const std::string &name);

void Uninitialize();

} // namespace vulkan

#endif // VKMC_BASE_INTERNAL_VULKAN_H_
