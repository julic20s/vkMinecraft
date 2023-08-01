#pragma once
#ifndef VKMC_CAMERA_H_
#define VKMC_CAMERA_H_

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class Camera {
public:
  float fovy;
  float near, far;
  float aspect;
  glm::vec3 gaze;

  [[nodiscard]] glm::mat4 CreateViewMatrix(const glm::vec3 &position) const noexcept;

  [[nodiscard]] glm::mat4 CreateProjectionMatrix() const noexcept;
};

#endif // VKMC_CAMERA_H_
