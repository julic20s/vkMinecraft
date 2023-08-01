#include <cstdint>
#include <limits>
#include <ranges>
#include <string_view>

#include <glm/mat4x4.hpp>

#include <assets/load.h>
#include <assets/shader.h>
#include <event.h>
#include <window.h>

#include "events.h"
#include "render/utility.h"
#include "renderer.h"

static vk::PresentModeKHR ChooseVkPresentMode(
    std::span<const vk::PresentModeKHR> available
) noexcept {
  auto it = std::ranges::find(available, vk::PresentModeKHR::eMailbox);
  return it != available.end() ? *it : vk::PresentModeKHR::eFifo;
}

static vk::SurfaceFormatKHR ChooseVkSurfaceFormat(
    std::span<const vk::SurfaceFormatKHR> available
) noexcept {
  const vk::SurfaceFormatKHR prefer{
      .format = vk::Format::eA8B8G8R8SrgbPack32,
      .colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
  };
  auto it = std::ranges::find(available, prefer);
  return it != available.end() ? prefer : available.front();
}

static vk::Extent2D ChooseVkExtent(
    std::uint32_t width, std::uint32_t height,
    const vk::SurfaceCapabilitiesKHR &capabilities
) noexcept {
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  }

  vk::Extent2D actual(width, height);
  auto &min = capabilities.minImageExtent;
  auto &max = capabilities.maxImageExtent;
  return {
      .width = std::clamp(width, min.width, max.width),
      .height = std::clamp(height, min.height, max.height),
  };
}

Renderer::Renderer(
    const ChunkManager &chunk_manager,
    const BlockRegistry &block_registry
) : chunk_manager_(chunk_manager),
    block_registry_(block_registry),
    current_frame_(0) {
  graphics_queue_ = vulkan::device.getQueue(vulkan::GetGraphicsQueue(), 0);
  present_queue_ = vulkan::device.getQueue(vulkan::GetPresentQueue(), 0);

  auto present_modes = vulkan::gpu.getSurfacePresentModesKHR(vulkan::surface);
  auto surface_formats = vulkan::gpu.getSurfaceFormatsKHR(vulkan::surface);

  present_mode_ = ChooseVkPresentMode(present_modes);
  surface_format_ = ChooseVkSurfaceFormat(surface_formats);

  auto candidates = std::array{
      vk::Format::eD32Sfloat,
      vk::Format::eD32SfloatS8Uint,
      vk::Format::eD24UnormS8Uint,
  };
  depth_format_ = FindSupportedFormat(
      candidates, vk::ImageTiling::eOptimal,
      vk::FormatFeatureFlagBits::eDepthStencilAttachment
  );

  CreateRenderPass(surface_format_.format);

  CreatePipelineLayout();
  CreatePipeline();

  CreateCommandPool();
  CreateCommandBuffers();
  CreateUniformBuffers();
  CreateDescriptorPool();
  CreateTextureImage();
  CreateTextureImageView();
  CreateTextureSampler();
  CreateDescriptorSet();

  CreateSyncObjects();

  SubscribeInScope(
      events::global, events::kWindowSize, this,
      +[](Renderer *r, std::uint16_t width, std::uint16_t height) {
        r->RecreateSwapchain(width, height);
      }
  );

  SubscribeInScope(
      events::global, events::kChunkLoaded, this,
      +[](Renderer *renderer, ChunkId chunk, const Chunk *c) {
        renderer->GenerateChunkResources(chunk, c);
      }
  );

  SubscribeInScope(
      events::global, events::kChunkUnloaded, this,
      +[](Renderer *renderer, ChunkId chunk) {
        renderer->ReleaseChunkResources(chunk);
      }
  );

  SubscribeInScope(
      events::global, events::kChunkUpdate, this,
      +[](Renderer *renderer, ChunkId chunk, const Chunk *c) {
        renderer->ReleaseChunkResources(chunk);
        renderer->GenerateChunkResources(chunk, c);
      }
  );

  RecreateSwapchain(window::GetWidth(), window::GetHeight());
}

void Renderer::RecreateSwapchain(std::uint32_t width, std::uint32_t height) {
  vulkan::device.waitIdle();

  DestroySwapchain();

  capabilities_ = vulkan::gpu.getSurfaceCapabilitiesKHR(vulkan::surface);
  extent_ = ChooseVkExtent(width, height, capabilities_);

  auto n = capabilities_.minImageCount + 1;
  auto max = capabilities_.maxImageCount;
  if (max > 0) {
    n = std::min(n, max);
  }

  vk::SwapchainCreateInfoKHR ci{
      .surface = vulkan::surface,
      .minImageCount = n,
      .imageFormat = surface_format_.format,
      .imageColorSpace = surface_format_.colorSpace,
      .imageExtent = extent_,
      .imageArrayLayers = 1,
      .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
      .preTransform = capabilities_.currentTransform,
      .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
      .presentMode = present_mode_,
      .clipped = VK_TRUE,
  };

  std::uint32_t indices[]{
      vulkan::GetGraphicsQueue(),
      vulkan::GetPresentQueue(),
  };

  if (indices[0] == indices[1]) {
    ci.imageSharingMode = vk::SharingMode::eExclusive;
  } else {
    ci.imageSharingMode = vk::SharingMode::eConcurrent;
    ci.queueFamilyIndexCount = 2;
    ci.pQueueFamilyIndices = indices;
  }

  swapchain_ = vulkan::device.createSwapchainKHR(ci);

  auto images = vulkan::device.getSwapchainImagesKHR(swapchain_);

  imageviews_.reserve(images.size());
  framebuffers_.reserve(images.size());

  for (auto image : images) {
    vk::ImageViewCreateInfo view_ci{
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = surface_format_.format,
        .components{
            .r = vk::ComponentSwizzle::eIdentity,
            .g = vk::ComponentSwizzle::eIdentity,
            .b = vk::ComponentSwizzle::eIdentity,
            .a = vk::ComponentSwizzle::eIdentity,
        },
        .subresourceRange{
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    auto view = vulkan::device.createImageView(view_ci);
    imageviews_.emplace_back(view);
  }

  CreateDepthImages();
  CreateDepthImageViews();

  for (auto i = 0; i != images.size(); ++i) {
    auto attachments = std::array{imageviews_[i], depth_buffers_[i].view};
    vk::FramebufferCreateInfo ci{
        .renderPass = pass_,
        .attachmentCount = attachments.size(),
        .pAttachments = attachments.data(),
        .width = extent_.width,
        .height = extent_.height,
        .layers = 1,
    };

    framebuffers_.emplace_back(vulkan::device.createFramebuffer(ci));
  }
}

vk::Format Renderer::FindSupportedFormat(
    std::span<const vk::Format> candidates,
    vk::ImageTiling tiling,
    vk::FormatFeatureFlags features
) {
  for (auto format : candidates) {
    auto properties = vulkan::gpu.getFormatProperties(format);
    if (tiling == vk::ImageTiling::eLinear && (properties.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == vk::ImageTiling::eOptimal && (properties.optimalTilingFeatures & features) == features) {
      return format;
    }
  }
  throw std::runtime_error("Failed to find supported format!");
}

void Renderer::CreateDepthImages() {
  depth_buffers_.resize(imageviews_.size());

  vku::OneTimeSubmit(
      vulkan::device, cmd_pool_, [&](vk::CommandBuffer cmd) {
        for (auto &depth : depth_buffers_) {
          depth = CreateImage(
              extent_.width, extent_.height, depth_format_, vk::ImageTiling::eOptimal,
              vk::ImageUsageFlagBits::eDepthStencilAttachment,
              vk::MemoryPropertyFlagBits::eDeviceLocal
          );

          vku::TransitionImageLayout(
              cmd, depth.image, depth_format_, vk::ImageLayout::eUndefined,
              vk::ImageLayout::eDepthStencilAttachmentOptimal
          );
        }
      },
      graphics_queue_
  );
}

void Renderer::CreateDepthImageViews() {
  for (auto &img : depth_buffers_) {
    vk::ImageViewCreateInfo ci{
        .image = img.image,
        .viewType = vk::ImageViewType::e2D,
        .format = depth_format_,
        .subresourceRange{
            .aspectMask = vk::ImageAspectFlagBits::eDepth,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    img.view = vulkan::device.createImageView(ci);
  }
}

void Renderer::CreateRenderPass(vk::Format surface_format) {
  vk::AttachmentDescription color{
      .format = surface_format,
      .samples = vk::SampleCountFlagBits::e1,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
      .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
      .initialLayout = vk::ImageLayout::eUndefined,
      .finalLayout = vk::ImageLayout::ePresentSrcKHR,
  };

  vk::AttachmentDescription depth{
      .format = depth_format_,
      .samples = vk::SampleCountFlagBits::e1,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
      .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
      .initialLayout = vk::ImageLayout::eUndefined,
      .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
  };

  vk::AttachmentReference color_ref{
      .attachment = 0,
      .layout = vk::ImageLayout::eColorAttachmentOptimal,
  };
  vk::AttachmentReference depth_ref{
      .attachment = 1,
      .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
  };

  vk::SubpassDescription subpass{
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_ref,
      .pDepthStencilAttachment = &depth_ref,
  };

  vk::SubpassDependency dependency{
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask =
          vk::PipelineStageFlagBits::eColorAttachmentOutput |
          vk::PipelineStageFlagBits::eEarlyFragmentTests,
      .dstStageMask =
          vk::PipelineStageFlagBits::eColorAttachmentOutput |
          vk::PipelineStageFlagBits::eEarlyFragmentTests,
      .dstAccessMask =
          vk::AccessFlagBits::eColorAttachmentWrite |
          vk::AccessFlagBits::eDepthStencilAttachmentWrite,
  };

  auto attachments = std::array{color, depth};
  vk::RenderPassCreateInfo ci{
      .attachmentCount = attachments.size(),
      .pAttachments = attachments.data(),
      .subpassCount = 1,
      .pSubpasses = &subpass,
      .dependencyCount = 1,
      .pDependencies = &dependency,
  };

  pass_ = vulkan::device.createRenderPass(ci);
}

void Renderer::CreatePipelineLayout() {
  vk::DescriptorSetLayoutBinding ubo{
      .binding = 0,
      .descriptorType = vk::DescriptorType::eUniformBuffer,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eVertex,
  };
  vk::DescriptorSetLayoutBinding texture{
      .binding = 1,
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eFragment,
  };

  auto bindings = std::array{ubo, texture};

  vk::DescriptorSetLayoutCreateInfo desc_ci{
      .bindingCount = bindings.size(),
      .pBindings = bindings.data(),
  };

  descriptor_set_layout_ = vulkan::device.createDescriptorSetLayout(desc_ci);

  vk::PushConstantRange push_constants{
      .stageFlags = vk::ShaderStageFlagBits::eVertex,
      .size = sizeof(PushConstants),
  };

  vk::PipelineLayoutCreateInfo ci{
      .setLayoutCount = 1,
      .pSetLayouts = &descriptor_set_layout_,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &push_constants,
  };

  pipeline_layout_ = vulkan::device.createPipelineLayout(ci);
}

void Renderer::CreatePipeline() {
  auto vert = assets::LoadShader("shaders/block.vert.spv");
  auto frag = assets::LoadShader("shaders/block.frag.spv");

  vk::PipelineShaderStageCreateInfo vert_stage{
      .stage = vk::ShaderStageFlagBits::eVertex,
      .module = vert,
      .pName = "main",
  };

  vk::PipelineShaderStageCreateInfo frag_stage{
      .stage = vk::ShaderStageFlagBits::eFragment,
      .module = frag,
      .pName = "main",
  };

  auto stages = std::array{vert_stage, frag_stage};

  vk::PipelineInputAssemblyStateCreateInfo input_asm{
      .topology = vk::PrimitiveTopology::eTriangleStrip,
  };

  vk::VertexInputBindingDescription face{
      .binding = 0,
      .stride = sizeof(FaceInstance),
      .inputRate = vk::VertexInputRate::eInstance,
  };

  std::vector<vk::VertexInputAttributeDescription> attributes;
  {
    std::uint32_t location = 0;
    auto face_attributes = FaceInstance::GetInputAttributes();
    for (auto &attr : face_attributes) {
      attributes.emplace_back(attr);
      attributes.back().location = location++;
      attributes.back().binding = face.binding;
    }
  }

  auto bindings = std::array{face};
  vk::PipelineVertexInputStateCreateInfo input_ci{
      .vertexBindingDescriptionCount = bindings.size(),
      .pVertexBindingDescriptions = bindings.data(),
      .vertexAttributeDescriptionCount = std::uint32_t(attributes.size()),
      .pVertexAttributeDescriptions = attributes.data(),
  };

  vk::Viewport viewport{
      .width = float(extent_.width),
      .height = float(extent_.height),
      .minDepth = 0,
      .maxDepth = 1,
  };

  vk::Rect2D scissor{
      .extent{
          .width = extent_.width,
          .height = extent_.height,
      },
  };

  vk::PipelineViewportStateCreateInfo viewport_ci{
      .viewportCount = 1,
      .pViewports = &viewport,
      .scissorCount = 1,
      .pScissors = &scissor,
  };

  vk::PipelineRasterizationStateCreateInfo rasterization_ci{
      .polygonMode = vk::PolygonMode::eFill,
      .cullMode = vk::CullModeFlagBits::eBack,
      .frontFace = vk::FrontFace::eCounterClockwise,
      .lineWidth = 1.f,
  };

  auto dynamic_states = std::array{
      vk::DynamicState::eViewport,
      vk::DynamicState::eScissor,
  };

  vk::PipelineDynamicStateCreateInfo dynamic_ci{
      .dynamicStateCount = dynamic_states.size(),
      .pDynamicStates = dynamic_states.data(),
  };

  vk::PipelineMultisampleStateCreateInfo msaa_ci{
      .rasterizationSamples = vk::SampleCountFlagBits::e1,
  };

  vk::PipelineColorBlendAttachmentState blend_attachment{
      .blendEnable = false,
      .colorWriteMask =
          vk::ColorComponentFlagBits::eR |
          vk::ColorComponentFlagBits::eG |
          vk::ColorComponentFlagBits::eB |
          vk::ColorComponentFlagBits::eA,
  };

  vk::PipelineDepthStencilStateCreateInfo depth_ci{
      .depthTestEnable = true,
      .depthWriteEnable = true,
      .depthCompareOp = vk::CompareOp::eLess,
  };

  vk::PipelineColorBlendStateCreateInfo blend_ci{
      .attachmentCount = 1,
      .pAttachments = &blend_attachment,
  };

  vk::GraphicsPipelineCreateInfo ci{
      .stageCount = 2,
      .pStages = stages.data(),
      .pVertexInputState = &input_ci,
      .pInputAssemblyState = &input_asm,
      .pViewportState = &viewport_ci,
      .pRasterizationState = &rasterization_ci,
      .pMultisampleState = &msaa_ci,
      .pDepthStencilState = &depth_ci,
      .pColorBlendState = &blend_ci,
      .pDynamicState = &dynamic_ci,
      .layout = pipeline_layout_,
      .renderPass = pass_,
      .subpass = 0,
  };

  auto result = vulkan::device.createGraphicsPipeline({}, ci);
  if (result.result != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to create renderer pipeline!");
  }

  pipeline_ = result.value;

  assets::Unload("shaders/block.vert.spv");
  assets::Unload("shaders/block.frag.spv");
}

Buffer Renderer::CreateBuffer(
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags property
) {
  vk::BufferCreateInfo ci{
      .size = size,
      .usage = usage,
      .sharingMode = vk::SharingMode::eExclusive,
  };
  auto buffer = vulkan::device.createBuffer(ci);
  auto requirements = vulkan::device.getBufferMemoryRequirements(buffer);
  vk::MemoryAllocateInfo ai{
      .allocationSize = requirements.size,
      .memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits, property),
  };
  auto memory = vulkan::device.allocateMemory(ai);
  vulkan::device.bindBufferMemory(buffer, memory, 0);
  return {buffer, memory};
}

void Renderer::CreateUniformBuffers() {
  for (auto &frame : frames_) {
    auto buffer = CreateBuffer(
        sizeof(glm::mat4),
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    frame.uniform_buffer.memory = buffer.memory;
    frame.uniform_buffer.buffer = buffer.buffer;
    frame.uniform_buffer.mapping = vulkan::device.mapMemory(buffer.memory, 0, sizeof(glm::mat4));
  }
}

void Renderer::CreateDescriptorPool() {
  vk::DescriptorPoolSize mvp_size{
      .type = vk::DescriptorType::eUniformBuffer,
      .descriptorCount = kMaxFramesInFlight,
  };
  vk::DescriptorPoolSize texture_size{
      .type = vk::DescriptorType::eCombinedImageSampler,
      .descriptorCount = kMaxFramesInFlight,
  };

  auto sizes = std::array{mvp_size, texture_size};

  vk::DescriptorPoolCreateInfo pool_ci{
      .maxSets = kMaxFramesInFlight,
      .poolSizeCount = sizes.size(),
      .pPoolSizes = sizes.data(),
  };
  descriptor_pool_ = vulkan::device.createDescriptorPool(pool_ci);
}

MappingBuffer Renderer::CreateChunkMeshBufferForCPU() {
  MappingBuffer mapping_buffer;
  auto buffer = CreateBuffer(
      kChunkBufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferSrc,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
  );

  mapping_buffer.buffer = buffer.buffer;
  mapping_buffer.memory = buffer.memory;
  mapping_buffer.mapping = vulkan::device.mapMemory(buffer.memory, 0, kChunkBufferSize);

  return mapping_buffer;
}

Buffer Renderer::CreateChunkMeshBufferForGPU() {
  return CreateBuffer(
      kChunkBufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
      vk::MemoryPropertyFlagBits::eDeviceLocal
  );
}

Image Renderer::CreateImage(
    std::uint32_t width,
    std::uint32_t height,
    vk::Format format, vk::ImageTiling tiling,
    vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties
) {
  vk::ImageCreateInfo info{
      .imageType = vk::ImageType::e2D,
      .format = format,
      .extent{
          .width = width,
          .height = height,
          .depth = 1,
      },
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = vk::SampleCountFlagBits::e1,
      .tiling = tiling,
      .usage = usage,
      .sharingMode = vk::SharingMode::eExclusive,
      .initialLayout = vk::ImageLayout::eUndefined,
  };

  auto image = vulkan::device.createImage(info);

  auto requirements = vulkan::device.getImageMemoryRequirements(image);

  vk::MemoryAllocateInfo ai{
      .allocationSize = requirements.size,
      .memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits, properties),
  };
  auto memory = vulkan::device.allocateMemory(ai);

  vulkan::device.bindImageMemory(image, memory, 0);

  return {memory, image};
}

void Renderer::CreateTextureImage() {
  auto image_width = block_registry_.GetMegaBlocksTextureWidth();
  auto image_height = block_registry_.GetMegaBlocksTextureHeight();
  auto image_depth = block_registry_.GetMegaBlocksTextureDepth();
  auto image_size = image_width * image_height * image_depth;
  auto stage_buffer = CreateBuffer(
      image_size, vk::BufferUsageFlagBits::eTransferSrc,
      vk::MemoryPropertyFlagBits::eHostVisible |
          vk::MemoryPropertyFlagBits::eHostCoherent
  );

  {
    auto data = vulkan::device.mapMemory(stage_buffer.memory, 0, image_size);
    block_registry_.MakeMegaBlocksTexture(data);
    vulkan::device.unmapMemory(stage_buffer.memory);
  }

  block_texture_ = CreateImage(
      image_width, image_height,
      vk::Format::eA8B8G8R8SrgbPack32, vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
      vk::MemoryPropertyFlagBits::eDeviceLocal
  );

  vku::OneTimeSubmit(
      vulkan::device, cmd_pool_, [&](vk::CommandBuffer cmd) {
        vku::TransitionImageLayout(
            cmd, block_texture_.image, vk::Format::eA8B8G8R8SrgbPack32,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal
        );

        vk::BufferImageCopy copy{
            .imageSubresource{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .imageExtent{
                .width = std::uint32_t(image_width),
                .height = std::uint32_t(image_height),
                .depth = 1,
            },
        };

        cmd.copyBufferToImage(stage_buffer.buffer, block_texture_.image, vk::ImageLayout::eTransferDstOptimal, copy);

        vku::TransitionImageLayout(
            cmd, block_texture_.image, vk::Format::eA8B8G8R8SrgbPack32,
            vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal
        );
      },
      graphics_queue_
  );

  vulkan::device.destroyBuffer(stage_buffer.buffer);
  vulkan::device.freeMemory(stage_buffer.memory);
}

void Renderer::CreateTextureImageView() {
  vk::ImageViewCreateInfo ci{
      .image = block_texture_.image,
      .viewType = vk::ImageViewType::e2D,
      .format = vk::Format::eA8B8G8R8SrgbPack32,
      .subresourceRange{
          .aspectMask = vk::ImageAspectFlagBits::eColor,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
      },
  };

  block_texture_.view = vulkan::device.createImageView(ci);
}

void Renderer::CreateTextureSampler() {
  vk::SamplerCreateInfo ci{
      .magFilter = vk::Filter::eNearest,
      .minFilter = vk::Filter::eLinear,
      .mipmapMode = vk::SamplerMipmapMode::eLinear,
      .compareEnable = VK_TRUE,
      .compareOp = vk::CompareOp::eAlways,
      .unnormalizedCoordinates = VK_FALSE,
  };

  block_texture_sampler_ = vulkan::device.createSampler(ci);
}

std::uint32_t Renderer::FindMemoryType(std::uint32_t types, vk::MemoryPropertyFlags prop) {
  auto available = vulkan::gpu.getMemoryProperties();
  for (std::uint32_t i = 0; i < available.memoryTypeCount; ++i) {
    if ((types & (1 << i)) && (available.memoryTypes[i].propertyFlags & prop) == prop) {
      return i;
    }
  }
  throw std::runtime_error("Failed to find suitable memory type!");
}

void Renderer::CreateDescriptorSet() {
  std::vector<vk::DescriptorSetLayout> layouts(kMaxFramesInFlight, descriptor_set_layout_);
  vk::DescriptorSetAllocateInfo ai{
      .descriptorPool = descriptor_pool_,
      .descriptorSetCount = std::uint32_t(layouts.size()),
      .pSetLayouts = layouts.data(),
  };
  auto desc = vulkan::device.allocateDescriptorSets(ai);
  auto it = desc.begin();
  for (auto &frame : frames_) {
    frame.descriptor_set = *it;
    ++it;

    vk::DescriptorBufferInfo mvp_buf_info{
        .buffer = frame.uniform_buffer.buffer,
        .range = sizeof(glm::mat4),
    };

    vk::DescriptorImageInfo texture_img_info{
        .sampler = block_texture_sampler_,
        .imageView = block_texture_.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
    };

    vk::WriteDescriptorSet mvp_write{
        .dstSet = frame.descriptor_set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .pBufferInfo = &mvp_buf_info,
    };
    vk::WriteDescriptorSet texture_write{
        .dstSet = frame.descriptor_set,
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &texture_img_info,
    };

    vulkan::device.updateDescriptorSets({mvp_write, texture_write}, {});
  }
}

void Renderer::CreateCommandPool() {
  vk::CommandPoolCreateInfo ci{
      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
      .queueFamilyIndex = vulkan::GetGraphicsQueue(),
  };
  cmd_pool_ = vulkan::device.createCommandPool(ci);
}

void Renderer::CreateSyncObjects() {
  vk::FenceCreateInfo fence_ci{
      .flags = vk::FenceCreateFlagBits::eSignaled,
  };
  for (auto &frame : frames_) {
    frame.image_available_semaphore = vulkan::device.createSemaphore({});
    frame.render_finished_semaphore = vulkan::device.createSemaphore({});
    frame.flight_fence = vulkan::device.createFence(fence_ci);
  }
}

void Renderer::CreateCommandBuffers() {
  vk::CommandBufferAllocateInfo ai{
      .commandPool = cmd_pool_,
      .commandBufferCount = std::uint32_t(std::size(frames_)),
  };

  auto cmds = vulkan::device.allocateCommandBuffers(ai);
  auto it = cmds.begin();
  for (auto &frame : frames_) {
    frame.cmd = *it;
    ++it;
  }
}

void Renderer::GenerateChunkResources(ChunkId chunk_id, const Chunk *chunk) {
  std::uint32_t index;
  if (free_chunk_buffer_index_.size()) {
    index = free_chunk_buffer_index_.front();
    free_chunk_buffer_index_.pop();
    for (auto &frame : frames_) {
      frame.chunk_buffer[index].second = true;
    }
  } else {
    index = chunk_buffer_.size();
    chunk_buffer_.emplace_back(CreateChunkMeshBufferForCPU());
    for (auto &frame : frames_) {
      frame.chunk_buffer.emplace_back(CreateChunkMeshBufferForGPU(), true);
    }
  }
  auto &info = chunks_[*reinterpret_cast<std::uint64_t *>(&chunk_id)] = {
      .index = index,
  };
  GenerateChunkMesh(chunk_id, chunk, info);
}

void Renderer::GenerateChunkMesh(ChunkId chunk_id, const Chunk *pointer, ChunkInfo &info) {
  info.chunk = pointer;
  auto &chunk = *pointer;
  std::uint32_t index = 0;

  auto faces = reinterpret_cast<FaceInstance *>(chunk_buffer_[info.index].mapping);
  auto faces_begin = faces;

  auto offset = glm::ivec3(chunk_id.x * Chunk::kLength, 0, chunk_id.y * Chunk::kLength);

  auto add_face = [&](std::uint32_t block, FaceDirection dir, const glm::ivec3 &block_pos) {
    auto texture_id = block_registry_.GetFaceTextureId(block, dir);
    faces->face = dir;
    faces->texture = texture_id;
    faces->position = block_pos;
    ++faces;
  };

  for (int y = 0; y != Chunk::kLength; ++y) {
    for (int x = 0; x != Chunk::kLength; ++x) {
      for (int z = 0; z != Chunk::kLength; ++z) {
        auto block = chunk(x, y, z);
        if (block == blocks::kAir) {
          continue;
        }
        auto block_pos = glm::ivec3(x, y, z) + offset;

        add_face(block, FaceDirection::kNorth, block_pos);
        add_face(block, FaceDirection::kSouth, block_pos);
        add_face(block, FaceDirection::kWest, block_pos);
        add_face(block, FaceDirection::kEast, block_pos);
        add_face(block, FaceDirection::kTop, block_pos);
        add_face(block, FaceDirection::kBottom, block_pos);

        /* if (chunk_manager_.GetBlock(block_pos + glm::ivec3(0, 0, 1)) == blocks::kAir) {
          
        }

        if (chunk_manager_.GetBlock(block_pos + glm::ivec3(0, 0, -1)) == blocks::kAir) {
          
        }

        if (chunk_manager_.GetBlock(block_pos + glm::ivec3(1, 0, 0)) == blocks::kAir) {
          
        }

        if (chunk_manager_.GetBlock(block_pos + glm::ivec3(-1, 0, 0)) == blocks::kAir) {
          
        }

        if (chunk_manager_.GetBlock(block_pos + glm::ivec3(0, 1, 0)) == blocks::kAir) {
          
        }

        if (chunk_manager_.GetBlock(block_pos + glm::ivec3(0, -1, 0)) == blocks::kAir) {
          
        } */
      }
    }
  }

  info.n_face = faces - faces_begin;
}

void Renderer::ReleaseChunkResources(glm::ivec2 chunk_id) {
  auto it = chunks_.find(*reinterpret_cast<std::uint64_t *>(&chunk_id));
  if (it != chunks_.end()) {
    free_chunk_buffer_index_.emplace(it->second.index);
    chunks_.erase(it);
  }
}

void Renderer::UpdateUniformBuffer(const glm::vec3 &position, void *dst) {
  auto &camera = *camera_;
  new (dst) glm::mat4(camera.CreateProjectionMatrix() * camera.CreateViewMatrix(position));
}

void Renderer::RecordCommandBuffer(std::uint32_t image_id) {
  auto &frame = frames_[current_frame_];

  auto cmd = frame.cmd;
  cmd.reset();

  vk::CommandBufferBeginInfo bi{};
  cmd.begin(bi);

  auto c = std::array{.1875f, .3320f, .5352f, 1.f};
  vk::ClearValue color_clear{
      .color{c},
  };
  vk::ClearValue depth_clear{
      .depthStencil{1.f},
  };

  auto clears = std::array{color_clear, depth_clear};
  vk::RenderPassBeginInfo pass_bi{
      .renderPass = pass_,
      .framebuffer = framebuffers_[image_id],
      .renderArea{
          .extent = extent_,
      },
      .clearValueCount = clears.size(),
      .pClearValues = clears.data(),
  };
  cmd.beginRenderPass(pass_bi, vk::SubpassContents::eInline);
  cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_);
  cmd.setViewport(
      0, vk::Viewport{
             .width = float(extent_.width),
             .height = float(extent_.height),
             .minDepth = 0.f,
             .maxDepth = 1.f,
         }
  );
  cmd.setScissor(0, vk::Rect2D{.extent = extent_});
  PushConstants push_constants{
      .texture_count = std::uint32_t(block_registry_.GetTextureCount()),
  };
  cmd.pushConstants(pipeline_layout_, vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants), &push_constants);
  cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout_, 0, frame.descriptor_set, {});
  for (auto &chunk_info : chunks_ | std::views::values) {
    auto chunk_buffer = frame.chunk_buffer[chunk_info.index].first.buffer;
    cmd.bindVertexBuffers(0, {chunk_buffer}, {0});
    cmd.draw(4, chunk_info.n_face, 0, 0);
  }
  cmd.endRenderPass();
  cmd.end();
}

void Renderer::Render(const glm::vec3 &position) {
  // Is required to draw?
  if (extent_.width * extent_.height == 0) {
    return;
  }

  auto &frame = frames_[current_frame_];

  // Wait the frame will to draw
  {
    auto result = vulkan::device.waitForFences(frame.flight_fence, true, ~0ull);
    if (result != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to wait for fence!");
    }
    vulkan::device.resetFences(frame.flight_fence);
  }

  // Acquire next image from swapchain
  std::uint32_t image_index;
  {
    auto index = vulkan::device.acquireNextImageKHR(swapchain_, ~0ull, frame.image_available_semaphore);
    if (index.result != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to acquire next image!");
    }
    image_index = index.value;
  }

  UpdateUniformBuffer(position, frame.uniform_buffer.mapping);

  vku::OneTimeSubmit(
      vulkan::device, cmd_pool_, [&](vk::CommandBuffer cmd) {
        for (auto &chunk_info : chunks_ | std::views::values) {
          auto &[gpu_buffer, invalidate] = frame.chunk_buffer[chunk_info.index];
          if (invalidate) {
            invalidate = false;
            vk::BufferCopy copy{.size = chunk_info.n_face * sizeof(FaceInstance)};
            cmd.copyBuffer(
                chunk_buffer_[chunk_info.index].buffer,
                gpu_buffer.buffer,
                copy
            );
          }
        }
      },
      graphics_queue_
  );

  RecordCommandBuffer(image_index);

  vk::PipelineStageFlags stages[]{vk::PipelineStageFlagBits::eColorAttachmentOutput};
  vk::SubmitInfo submit{
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &frame.image_available_semaphore,
      .pWaitDstStageMask = stages,
      .commandBufferCount = 1,
      .pCommandBuffers = &frame.cmd,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &frame.render_finished_semaphore,
  };
  graphics_queue_.submit(submit, frame.flight_fence);

  vk::PresentInfoKHR present{
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &frame.render_finished_semaphore,
      .swapchainCount = 1,
      .pSwapchains = &swapchain_,
      .pImageIndices = &image_index,
  };

  if (present_queue_.presentKHR(present) != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to present!");
  }

  current_frame_ = (current_frame_ + 1) % kMaxFramesInFlight;
}

Renderer::~Renderer() {
  vulkan::device.waitIdle();

  for (auto &buffer : chunk_buffer_) {
    vulkan::device.unmapMemory(buffer.memory);
    vulkan::device.destroyBuffer(buffer.buffer);
    vulkan::device.freeMemory(buffer.memory);
  }

  for (auto &frame : frames_) {
    vulkan::device.destroySemaphore(frame.image_available_semaphore);
    vulkan::device.destroySemaphore(frame.render_finished_semaphore);
    vulkan::device.destroyFence(frame.flight_fence);
    vulkan::device.destroyBuffer(frame.uniform_buffer.buffer);
    vulkan::device.freeMemory(frame.uniform_buffer.memory);
    for (auto &buffer : frame.chunk_buffer | std::views::keys) {
      vulkan::device.destroyBuffer(buffer.buffer);
      vulkan::device.freeMemory(buffer.memory);
    }
    vulkan::device.freeCommandBuffers(cmd_pool_, frame.cmd);
  }

  DestroySwapchain();

  vulkan::device.destroySampler(block_texture_sampler_);
  vulkan::device.destroyImageView(block_texture_.view);
  vulkan::device.destroyImage(block_texture_.image);
  vulkan::device.freeMemory(block_texture_.memory);

  vulkan::device.destroyDescriptorPool(descriptor_pool_);
  vulkan::device.destroyDescriptorSetLayout(descriptor_set_layout_);

  vulkan::device.destroyCommandPool(cmd_pool_);
  vulkan::device.destroyPipeline(pipeline_);
  vulkan::device.destroyPipelineLayout(pipeline_layout_);
  vulkan::device.destroyRenderPass(pass_);
}

void Renderer::DestroySwapchain() {
  for (auto buffer : depth_buffers_) {
    vulkan::device.destroyImageView(buffer.view);
    vulkan::device.destroyImage(buffer.image);
    vulkan::device.freeMemory(buffer.memory);
  }
  depth_buffers_.clear();

  for (auto fb : framebuffers_) {
    vulkan::device.destroyFramebuffer(fb);
  }
  framebuffers_.clear();
  for (auto view : imageviews_) {
    vulkan::device.destroyImageView(view);
  }
  imageviews_.clear();
  if (swapchain_) {
    vulkan::device.destroySwapchainKHR(swapchain_);
  }
  swapchain_ = nullptr;
}
