#pragma once
#ifndef VKMC_RENDER_MOD_H_
#define VKMC_RENDER_MOD_H_

#include <bitset>
#include <cstdint>
#include <map>
#include <span>
#include <queue>
#include <utility>
#include <vector>

#include <common/classes.h>
#include <event/scope.h>
#include <vulkan.h>

#include "chunk/manager.h"
#include "render/camera.h"
#include "render/buffer.h"
#include "mesh/face_instance.h"
#include "block/registry.h"

class Renderer : NonCopyMove, EventScope {
private:
  static constexpr std::uint32_t kMaxFramesInFlight = 2;
  static constexpr auto kChunkBufferSize =
      Chunk::kLength * Chunk::kLength * Chunk::kLength * 6 * sizeof(FaceInstance);

public:
  Renderer(const ChunkManager &chunks, const BlockRegistry &);

  ~Renderer();

  void BindCamera(const Camera &camera) {
    camera_ = &camera;
  }

  void Render(const glm::vec3 &position);

private:
  void RecreateSwapchain(std::uint32_t width, std::uint32_t height);

  void CreateRenderPass(vk::Format);
  void CreatePipelineLayout();
  void CreatePipeline();

  vk::Format FindSupportedFormat(
      std::span<const vk::Format> candidates,
      vk::ImageTiling,
      vk::FormatFeatureFlags
  );

  std::uint32_t FindMemoryType(std::uint32_t types, vk::MemoryPropertyFlags);
  Buffer CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags, vk::MemoryPropertyFlags);
  Image CreateImage(
      std::uint32_t width,
      std::uint32_t height,
      vk::Format, vk::ImageTiling,
      vk::ImageUsageFlags, vk::MemoryPropertyFlags
  );

  void CreateUniformBuffers();
  void CreateDescriptorPool();
  void CreateDescriptorSet();
  void CreateTextureImage();
  void CreateTextureImageView();
  void CreateTextureSampler();
  void CreateDepthImages();
  void CreateDepthImageViews();

  void CreateCommandPool();
  void CreateCommandBuffers();
  void RecordCommandBuffer(std::uint32_t image_id);
  void CreateSyncObjects();

  void UpdateUniformBuffer(const glm::vec3 &position, void *);

  void DestroySwapchain();

  MappingBuffer CreateChunkMeshBufferForCPU();
  Buffer CreateChunkMeshBufferForGPU();

  struct ChunkInfo {
    const Chunk *chunk;
    /// The index of vertex buffer for the chunk
    std::uint32_t index;
    /// The number of block faces in the chunk
    std::uint32_t n_face;
  };

  void GenerateChunkResources(ChunkId, const Chunk *);
  void GenerateChunkMesh(ChunkId, const Chunk *, ChunkInfo &);
  void ReleaseChunkResources(ChunkId);

  std::uint32_t current_frame_;

  const Camera *camera_;

  const ChunkManager &chunk_manager_;

  vk::Format depth_format_;

  vk::Queue graphics_queue_;
  vk::Queue present_queue_;
  vk::SurfaceFormatKHR surface_format_;
  vk::PresentModeKHR present_mode_;
  vk::Extent2D extent_;
  vk::SurfaceCapabilitiesKHR capabilities_;

  vk::RenderPass pass_;
  vk::PipelineLayout pipeline_layout_;
  vk::DescriptorSetLayout descriptor_set_layout_;
  vk::Pipeline pipeline_;

  vk::DescriptorPool descriptor_pool_;

  vk::SwapchainKHR swapchain_;
  std::vector<vk::ImageView> imageviews_;
  std::vector<vk::Framebuffer> framebuffers_;
  std::vector<Image> depth_buffers_;

  vk::CommandPool cmd_pool_;

  std::map<std::uint64_t, ChunkInfo> chunks_;
  std::vector<MappingBuffer> chunk_buffer_;
  std::queue<std::uint32_t> free_chunk_buffer_index_;

  struct : NonCopy {
    vk::CommandBuffer cmd;
    vk::DescriptorSet descriptor_set;
    MappingBuffer uniform_buffer;

    vk::Semaphore image_available_semaphore;
    vk::Semaphore render_finished_semaphore;
    vk::Fence flight_fence;

    std::vector<std::pair<Buffer, bool>> chunk_buffer;
  } frames_[kMaxFramesInFlight];

  Image block_texture_;
  vk::Sampler block_texture_sampler_;

  const BlockRegistry &block_registry_;

  struct PushConstants {
    std::uint32_t texture_count;
  };
};

#endif // VKMC_RENDER_MOD_H_
