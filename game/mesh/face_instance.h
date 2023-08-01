#pragma once
#ifndef VKMC_MESH_FACE_INSTANCE_H_
#define VKMC_MESH_FACE_INSTANCE_H_

#include <span>

#include <glm/vec3.hpp>
#include <vulkan/vulkan.hpp>

#include "../block/types.h"

struct FaceInstance {
  FaceDirection face;
  TextureId texture;
  glm::ivec3 position;

  [[nodiscard]] static std::span<const vk::VertexInputAttributeDescription>
  GetInputAttributes() noexcept;
};

#endif // VKMC_MESH_FACE_INSTANCE_H_
