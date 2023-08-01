#include <cstdint>
#include <stdexcept>

#include <event.h>

#include "window.h"

static std::uint16_t width;
static std::uint16_t height;

static struct {
  EventPublisher *window_size;
  EventPublisher *mouse;
  EventPublisher *mouse_left_click;
} publishers;

GLFWwindow *window::internal::CreateWindow(int width, int height, const char *title) {
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  auto window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (window == nullptr) {
    throw std::runtime_error("Failed to create window!");
  }
  events::global.SubscribeEvent(
      events::kWindowSize, +[](std::uint16_t width, std::uint16_t height) {
        ::width = width;
        ::height = height;
      }
  );

  publishers.window_size = &events::global.RegisterEvent(events::kWindowSize);
  publishers.mouse = &events::global.RegisterEvent(events::kMouse);
  publishers.mouse_left_click = &events::global.RegisterEvent(events::kMouseLeftClick);
  glfwSetWindowSizeCallback(window, [](GLFWwindow *window, int width, int height) {
    publishers.window_size->EmitArgs(std::uint16_t(width), std::uint16_t(height));
  });
  glfwSetCursorPosCallback(window, [](GLFWwindow *window, double xpos, double ypos) {
    publishers.mouse->EmitArgs(xpos, ypos);
  });
  glfwSetMouseButtonCallback(window, [](GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
      publishers.mouse_left_click->EmitArgs();
    }
  });
  glfwGetWindowSize(window, &width, &height);
  ::width = width;
  ::height = height;

  return window;
}

std::uint16_t window::GetWidth() noexcept {
  return width;
}

std::uint16_t window::GetHeight() noexcept {
  return height;
}
