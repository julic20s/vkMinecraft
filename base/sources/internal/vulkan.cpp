#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "vulkan.h"

namespace {

struct VulkanQueueFamilyDetail {
  std::optional<std::uint32_t> graphics;
  std::optional<std::uint32_t> present;
  VulkanQueueFamilyDetail() = default;
  VulkanQueueFamilyDetail(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
    std::uint32_t index = 0;
    for (auto &family : device.getQueueFamilyProperties()) {
      if (family.queueFlags & vk::QueueFlagBits::eGraphics) {
        graphics = index;
      }
      if (device.getSurfaceSupportKHR(index, surface)) {
        present = index;
      }
      index++;
    }
  }

  [[nodiscard]] bool IsCompatible() noexcept {
    return graphics.has_value() && present.has_value();
  }
};

struct VulkanSwapchainDetail {
  std::vector<vk::SurfaceFormatKHR> surface_formats;
  std::vector<vk::PresentModeKHR> present_modes;

  VulkanSwapchainDetail() = default;
  VulkanSwapchainDetail(vk::PhysicalDevice gpu, vk::SurfaceKHR surface) {
    surface_formats = gpu.getSurfaceFormatsKHR(surface);
    present_modes = gpu.getSurfacePresentModesKHR(surface);
  }
};

class VulkanSuitablePhysicalDevice {
private:
  vk::PhysicalDevice handle_;
  VulkanQueueFamilyDetail qfamily_detail_;
  VulkanSwapchainDetail swapchain_detail_;

  static const char *FindUnsupportedDeviceExtension(
      vk::PhysicalDevice device, std::span<const char *> required
  ) {
    auto available = device.enumerateDeviceExtensionProperties();
    // O(N^2)
    for (auto extension : required) {
      bool found = std::ranges::any_of(available, [extension](auto &a) {
        return std::strcmp(a.extensionName, extension) == 0;
      });
      if (!found) {
        return extension;
      }
    }
    return nullptr;
  }

public:
  VulkanSuitablePhysicalDevice() = default;

  VulkanSuitablePhysicalDevice(
      vk::Instance instance, vk::SurfaceKHR surface, std::span<const char *> extensions
  ) {
    for (auto &device : instance.enumeratePhysicalDevices()) {
      VulkanQueueFamilyDetail qfamily(device, surface);
      if (qfamily.IsCompatible() && !FindUnsupportedDeviceExtension(device, extensions)) {
        VulkanSwapchainDetail swapchain(device, surface);
        if (swapchain.surface_formats.size() && swapchain.present_modes.size()) {
          handle_ = device;
          qfamily_detail_ = std::move(qfamily);
          swapchain_detail_ = std::move(swapchain);
          break;
        }
      }
    }

    if (!handle_) {
      throw std::runtime_error("Failed to find a suitable vulkan physical device!");
    }
  }

  std::uint32_t GetGraphicsQueueIndex() const noexcept {
    return qfamily_detail_.graphics.value();
  }

  std::uint32_t GetPresentQueueIndex() const noexcept {
    return qfamily_detail_.present.value();
  }

  vk::PhysicalDevice GetHandle() const noexcept {
    return handle_;
  }
};

} // namespace

vk::PhysicalDevice vulkan::gpu;
vk::Instance vulkan::instance;
vk::SurfaceKHR vulkan::surface;
vk::Device vulkan::device;
static VulkanSuitablePhysicalDevice suitable_gpu;

static std::vector<const char *> GetRequiredInstanceExtensionNames() noexcept {
  std::vector<const char *> extensions;
  // 1. Always included
  constexpr auto default_extensions = std::array{
      VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
  };
  // 2. For GLFW
  std::uint32_t glfw_extension_count;
  auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
  extensions.reserve(glfw_extension_count + default_extensions.size());

  std::copy(
      glfw_extensions, glfw_extensions + glfw_extension_count,
      std::back_insert_iterator(extensions)
  );
  std::copy(
      default_extensions.begin(), default_extensions.end(),
      std::back_insert_iterator(extensions)
  );
  return extensions;
}

static std::vector<const char *> GetRequiredDeviceExtensionNames() noexcept {
  return {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  };
}

static std::vector<const char *> GetRequiredInstanceLayerNames() noexcept {
#ifdef VKMC_NDEBUG
  return {};
#else
  return {"VK_LAYER_KHRONOS_validation"};
#endif
}

static const char *FindUnsupportedInstanceExtension(std::span<const char *> required) {
  auto available = vk::enumerateInstanceExtensionProperties();
  // O(N^2)
  for (auto extension : required) {
    bool found = std::ranges::any_of(available, [extension](auto &a) {
      return std::strcmp(a.extensionName, extension) == 0;
    });
    if (!found) {
      return extension;
    }
  }
  return nullptr;
}

static const char *FindUnsupportedInstanceLayer(std::span<const char *> required) {
  auto available = vk::enumerateInstanceLayerProperties();
  // O(N^2)
  for (auto layer : required) {
    bool found = std::ranges::any_of(available, [layer](auto &a) {
      return std::strcmp(a.layerName, layer) == 0;
    });
    if (!found) {
      return layer;
    }
  }
  return nullptr;
}

static void CheckInstanceRequirements(
    std::span<const char *> layers, std::span<const char *> extensions
) {
  {
    auto unsupported = FindUnsupportedInstanceExtension(extensions);
    if (unsupported) {
      auto msg = "Unsupported Vulkan instance extension: " + std::string(unsupported);
      throw std::runtime_error(std::move(msg));
    }
  }
  {
    auto unsupported = FindUnsupportedInstanceLayer(layers);
    if (unsupported) {
      auto msg = "Unsupported Vulkan instance layer: " + std::string(unsupported);
      throw std::runtime_error(std::move(msg));
    }
  }
}

static vk::Instance CreateInstance(
    const std::string &name,
    std::span<const char *> layers, std::span<const char *> extensions
) {
  vk::ApplicationInfo appinfo{
      .pApplicationName = name.c_str(),
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = name.c_str(),
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_0,
  };

  vk::InstanceCreateInfo ci{
      .flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
      .pApplicationInfo = &appinfo,
      .enabledLayerCount = std::uint32_t(layers.size()),
      .ppEnabledLayerNames = layers.data(),
      .enabledExtensionCount = std::uint32_t(extensions.size()),
      .ppEnabledExtensionNames = extensions.data(),
  };

  return vk::createInstance(ci);
}

static vk::SurfaceKHR CreateSurface(vk::Instance instance, GLFWwindow *window) {
  VkSurfaceKHR surface;
  if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create vulkan surface!");
  }
  return surface;
}

static vk::Device CreateLogicalDevice(
    const VulkanSuitablePhysicalDevice &device, std::span<const char *> extensions
) {
  auto priority = 1.f;
  vk::DeviceQueueCreateInfo grahics_ci{
      .queueFamilyIndex = device.GetGraphicsQueueIndex(),
      .queueCount = 1,
      .pQueuePriorities = &priority,
  };
  vk::DeviceQueueCreateInfo present_ci{
      .queueFamilyIndex = device.GetPresentQueueIndex(),
      .queueCount = 1,
      .pQueuePriorities = &priority,
  };
  std::array queue_ci{grahics_ci, present_ci};
  vk::PhysicalDeviceFeatures features[2];

  vk::DeviceCreateInfo ci{
      .queueCreateInfoCount = 2,
      .pQueueCreateInfos = queue_ci.data(),
      .enabledExtensionCount = std::uint32_t(extensions.size()),
      .ppEnabledExtensionNames = extensions.data(),
      .pEnabledFeatures = features,
  };

  return device.GetHandle().createDevice(ci);
}

void vulkan::internal::Initialize(GLFWwindow *window, const std::string &name) {
  auto layers = GetRequiredInstanceLayerNames();
  auto extensions = GetRequiredInstanceExtensionNames();
  CheckInstanceRequirements(layers, extensions);

  instance = CreateInstance(name, layers, extensions);
  surface = CreateSurface(instance, window);

  auto device_extensions = GetRequiredDeviceExtensionNames();
  suitable_gpu = VulkanSuitablePhysicalDevice(instance, surface, device_extensions);
  gpu = suitable_gpu.GetHandle();
  device = CreateLogicalDevice(suitable_gpu, device_extensions);
}

void vulkan::internal::Uninitialize() {
  device.destroy();
  instance.destroySurfaceKHR(surface);
  instance.destroy();
}

std::uint32_t vulkan::GetGraphicsQueue() {
  return suitable_gpu.GetGraphicsQueueIndex();
}

std::uint32_t vulkan::GetPresentQueue() {
  return suitable_gpu.GetPresentQueueIndex();
}
