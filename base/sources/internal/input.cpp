#include "input.h"

static GLFWwindow *window_ptr;

void input::internal::Bind(GLFWwindow *window) noexcept {
  window_ptr = window;
}

bool input::GetKey(Key key) noexcept {
  return glfwGetKey(window_ptr, int(key));
}

input::MouseKeyState input::GetMouseKey(MouseKey key) noexcept {
  return input::MouseKeyState(glfwGetMouseButton(window_ptr, int(key)));
}
