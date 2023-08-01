#include <cstdint>
#include <map>
#include <span>
#include <string_view>

#include <assets/shader.h>
#include <vulkan.h>

#include "assets.h"

static std::map<std::string_view, vk::ShaderModule> store;

static const vk::ShaderModule StoreShader(std::string_view name, std::span<const std::byte> bytes) {
  vk::ShaderModuleCreateInfo ci{
      .codeSize = bytes.size(),
      .pCode = reinterpret_cast<const std::uint32_t *>(bytes.data()),
  };
  auto [it, add] = store.emplace(name, vulkan::device.createShaderModule(ci));
  return it->second;
}

static void UnloadShader(std::string_view name) {
  auto find = store.find(name);
  vulkan::device.destroyShaderModule(find->second);
  store.erase(find);
}

vk::ShaderModule assets::LoadShader(std::string_view name) {
  auto find = store.find(name);
  if (find != store.end()) {
    return find->second;
  }
  return StoreShader(name, internal::Load(name, UnloadShader));
}
