#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "camera.h"

glm::mat4 Camera::CreateViewMatrix(const glm::vec3 &position) const noexcept {
  return glm::lookAt(position, position + gaze, {0, 1, 0});
}

glm::mat4 Camera::CreateProjectionMatrix() const noexcept {
  auto proj = glm::perspective(fovy, aspect, near, far);
  // Flip Y axis
  proj[1][1] = -proj[1][1];
  return proj;
}
