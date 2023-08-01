#pragma once
#ifndef VKMC_BASE_INTERNAL_WINDOW_H_
#define VKMC_BASE_INTERNAL_WINDOW_H_

#include <GLFW/glfw3.h>

#include <window.h>

namespace window::internal {

GLFWwindow *CreateWindow(int width, int height, const char *title);

}; // namespace window::internal

#endif // VKMC_BASE_INTERNAL_WINDOW_H_
