#pragma once
#ifndef VKMC_PLAYER_H_
#define VKMC_PLAYER_H_

#include <optional>

#include <glm/vec3.hpp>

#include <event/scope.h>

#include "physical/entity.h"
#include "chunk/manager.h"
#include "render/camera.h"

class Player : EventScope {
public:
  static constexpr float kHeight = 2;
  static constexpr float kWidth = 1;
  static constexpr float kMove = 0.1;

private:
  Camera camera_;
  Entity entity_;

  ChunkManager &chunks_;

  bool first_mouse_;
  double last_mouse_x_;
  double last_mouse_y_;
  float yaw_;
  float pitch_;

public:
  Player(ChunkManager &) noexcept;

  [[nodiscard]] Entity &GetEntity() noexcept {
    return entity_;
  }

  [[nodiscard]] const Camera &GetCamera() const noexcept {
    return camera_;
  }

  void Update();

private:
  void DestroyBlock();

  void UpdateRotateByMouse(double x, double y);

  std::optional<glm::vec3> RayMarch(
      const glm::vec3 &start,
      const glm::vec3 &gaze
  );
};

#endif // VKMC_PLAYER_H_
