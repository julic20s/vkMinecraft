#include <numbers>
#include <optional>

#include <event.h>
#include <input.h>

#include "window.h"

#include "player.h"

Player::Player(ChunkManager &chunks) noexcept : chunks_(chunks), first_mouse_(true), yaw_(0), pitch_(0) {
  camera_.aspect = float(window::GetWidth()) / window::GetHeight();
  camera_.near = 0.1;
  camera_.far = 128;
  camera_.fovy = 70;
  camera_.gaze = {0, 0, -1};

  SubscribeInScope(
      events::global, events::kWindowSize, this,
      +[](Player *player, std::uint16_t width, std::uint16_t height) {
        player->camera_.aspect = float(width) / height;
      }
  );

  SubscribeInScope(
      events::global, events::kMouse, this,
      +[](Player *player, double x, double y) {
        player->UpdateRotateByMouse(x, y);
      }
  );

  SubscribeInScope(
      events::global, events::kMouseLeftClick, this,
      +[](Player *player) {
        player->DestroyBlock();
      }
  );

  entity_.aabb = glm::vec3(kWidth, kHeight, kWidth);
  entity_.velocity = glm::vec3(0);
  entity_.acceleration = glm::vec3(0);
}

void Player::Update() {
  auto &gaze = camera_.gaze;

  auto forward = glm::normalize(glm::vec3(gaze.x, 0, gaze.z));
  auto up = glm::vec3(0, 1, 0);
  auto side = glm::cross(up, forward);

  if (input::GetKey(input::Key::kW)) {
    entity_.acceleration += forward;
  }
  if (input::GetKey(input::Key::kS)) {
    entity_.acceleration -= forward;
  }
  if (input::GetKey(input::Key::kA)) {
    entity_.acceleration += side;
  }
  if (input::GetKey(input::Key::kD)) {
    entity_.acceleration -= side;
  }
  if (input::GetKey(input::Key::kSpace)) {
    entity_.acceleration += up;
  }
  if (input::GetKey(input::Key::kLShift)) {
    entity_.acceleration -= up;
  }
  if (glm::length(entity_.acceleration) != 0) {
    entity_.acceleration = glm::normalize(entity_.acceleration) * kMove;
  }
}

void Player::UpdateRotateByMouse(double x, double y) {
  if (first_mouse_) {
    last_mouse_x_ = x;
    last_mouse_y_ = y;
    first_mouse_ = false;
  }

  auto dx = x - last_mouse_x_;
  auto dy = last_mouse_y_ - y;
  last_mouse_x_ = x;
  last_mouse_y_ = y;

  yaw_ += dx;
  pitch_ += dy;

  if (pitch_ > 89.0)
    pitch_ = 89.0;
  if (pitch_ < -89.0)
    pitch_ = -89.0;

  auto yaw_rad = (180 - yaw_) / 180 * std::numbers::pi;
  auto pitch_rad = pitch_ / 180 * std::numbers::pi;
  glm::vec3 gaze{
      sin(yaw_rad) * cos(pitch_rad),
      sin(pitch_rad),
      cos(yaw_rad) * cos(pitch_rad),
  };
  camera_.gaze = gaze;
}

std::optional<glm::vec3> Player::RayMarch(
    const glm::vec3 &start,
    const glm::vec3 &gaze
) {
  auto dist = glm::length(gaze);
  auto step = glm::normalize(gaze);
  for (float k = 0; k < dist; k += std::min(1.f, dist - k)) {
    auto center = glm::floor(start + step * glm::vec3(k));
    if (chunks_.GetBlock(center) != blocks::kAir) {
      return center;
    }
  }
  return std::nullopt;
}

void Player::DestroyBlock() {
  auto target = RayMarch(entity_.position, glm::normalize(camera_.gaze) * glm::vec3(10));
  if (target.has_value()) {
    chunks_.SetBlock(target.value(), blocks::kAir);
  }
}
