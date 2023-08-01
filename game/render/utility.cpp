#include <limits>
#include <cstdint>

#include "utility.h"

using namespace vku;

CommandBuffer internal::BeginOneTimeSubmit(Device device, CommandPool pool) {
  CommandBufferAllocateInfo alloc{
      .commandPool = pool,
      .level = CommandBufferLevel::ePrimary,
      .commandBufferCount = 1,
  };

  auto cmd = device.allocateCommandBuffers(alloc)[0];
  CommandBufferBeginInfo begin{};
  if (cmd.begin(&begin) != Result::eSuccess) {
    throw std::runtime_error("Failed to begin command buffer!");
  }
  return cmd;
}

void internal::EndOneTimeSubmit(Device device, CommandPool pool, CommandBuffer cmd, Queue queue) {
  cmd.end();
  auto fence = device.createFence({});
  SubmitInfo submit{
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd,
  };
  queue.submit(submit, fence);
  constexpr auto timeout = std::numeric_limits<std::uint64_t>::max();
  auto result = device.waitForFences(1, &fence, true, timeout);
  if (result != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to wait for fence!");
  }
  device.destroyFence(fence);
  device.freeCommandBuffers(pool, cmd);
}

static bool HasStencilComponent(Format format) noexcept {
  return format == Format::eD32SfloatS8Uint || format == Format::eD24UnormS8Uint;
}

void vku::TransitionImageLayout(
    CommandBuffer cmd, Image image, Format format,
    ImageLayout from, ImageLayout to
) {
  ImageMemoryBarrier barrier{
      .oldLayout = from,
      .newLayout = to,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange{
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
      }};

  if (to == ImageLayout::eDepthStencilAttachmentOptimal) {
    barrier.subresourceRange.aspectMask = ImageAspectFlagBits::eDepth;
    if (HasStencilComponent(format)) {
      barrier.subresourceRange.aspectMask |= ImageAspectFlagBits::eStencil;
    }
  } else {
    barrier.subresourceRange.aspectMask = ImageAspectFlagBits::eColor;
  }

  PipelineStageFlags src_stage;
  PipelineStageFlags dst_stage;

  if (from == ImageLayout::eUndefined && to == ImageLayout::eTransferDstOptimal) {
    barrier.srcAccessMask = AccessFlagBits::eNone;
    barrier.dstAccessMask = AccessFlagBits::eTransferWrite;
    src_stage = PipelineStageFlagBits::eTopOfPipe;
    dst_stage = PipelineStageFlagBits::eTransfer;
  } else if (from == ImageLayout::eTransferDstOptimal && to == ImageLayout::eShaderReadOnlyOptimal) {
    barrier.srcAccessMask = AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = AccessFlagBits::eShaderRead;

    src_stage = PipelineStageFlagBits::eTransfer;
    dst_stage = PipelineStageFlagBits::eFragmentShader;
  } else if (from == ImageLayout::eUndefined && to == ImageLayout::eDepthStencilAttachmentOptimal) {
    barrier.srcAccessMask = AccessFlagBits::eNone;
    barrier.dstAccessMask =
        AccessFlagBits::eDepthStencilAttachmentRead |
        AccessFlagBits::eDepthStencilAttachmentWrite;

    src_stage = PipelineStageFlagBits::eTopOfPipe;
    dst_stage = PipelineStageFlagBits::eEarlyFragmentTests;
  } else {
    throw std::invalid_argument("Unsupported layout transition!");
  }

  cmd.pipelineBarrier(
      src_stage, dst_stage,
      static_cast<DependencyFlags>(0),
      0, nullptr, 0, nullptr, 1, &barrier
  );
}
