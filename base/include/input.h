#pragma once
#ifndef VKMC_BASE_INPUT_H_
#define VKMC_BASE_INPUT_H_

#include <GLFW/glfw3.h>

namespace input {

enum class Key {
  kW = GLFW_KEY_W,
  kA = GLFW_KEY_A,
  kS = GLFW_KEY_S,
  kD = GLFW_KEY_D,
  kSpace = GLFW_KEY_SPACE,
  kLShift = GLFW_KEY_LEFT_SHIFT,
};

enum class MouseKey {
  kLeft = GLFW_MOUSE_BUTTON_LEFT,
  kRight = GLFW_MOUSE_BUTTON_RIGHT,
};

enum class MouseKeyState {
  kRelease = GLFW_RELEASE,
  kPresse = GLFW_PRESS,
  kRepeat = GLFW_REPEAT,
};

/// Detect that whether the key was pressed
[[nodiscard]] bool GetKey(Key) noexcept;

/// Detect the state of mouse key
[[nodiscard]] MouseKeyState GetMouseKey(MouseKey) noexcept;

} // namespace input

#endif // VKMC_BASE_INPUT_H_
