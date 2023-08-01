#include "face_instance.h"

[[nodiscard]] std::span<const vk::VertexInputAttributeDescription>
FaceInstance::GetInputAttributes() noexcept {
  static auto attributes = std::array{
      // face:
      vk::VertexInputAttributeDescription{
          .format = vk::Format::eR32Uint,
          .offset = offsetof(FaceInstance, face),
      },
      // texture:
      vk::VertexInputAttributeDescription{
          .format = vk::Format::eR32Uint,
          .offset = offsetof(FaceInstance, texture),
      },
      // position:
      vk::VertexInputAttributeDescription{
          .format = vk::Format::eR32G32B32Sint,
          .offset = offsetof(FaceInstance, position),
      },
  };
  return attributes;
}
