#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <forward_list>
#include <fstream>
#include <map>
#include <string_view>

#include <respack.h>

#include "assets.h"

static std::map<std::string_view, const respack::ResourceDescriptor *> resource_map;
static const std::byte *default_assets;

static void IndexResourceDescriptor(const respack::ResourceDescriptor *res, std::uint8_t *mem) {
  std::string_view name{(char *)(std::size_t(mem) + res->name_offset), res->name_length};
  resource_map.emplace(name, res);
}

void assets::internal::LoadAssetsFile(std::filesystem::path &&file) {
  std::ifstream ifs(file, std::ios::binary);
  if (!ifs.is_open()) {
    throw std::runtime_error("Failed to open respack!");
  }

  auto size = std::filesystem::file_size(file);
  auto memory = new std::uint8_t[size];
  default_assets = reinterpret_cast<const std::byte *>(memory);

  ifs.read(reinterpret_cast<char *>(memory), size);
  auto header = reinterpret_cast<const respack::FileHeader *>(memory);
  if (header->magic != respack::kHeaderMagic) {
    UnloadAssetsFile();
    throw std::runtime_error("Incorrect respack magic!");
  }

  auto offset = sizeof(respack::FileHeader);
  for (auto i = 0; i != header->sections; ++i) {
    auto section = reinterpret_cast<const respack::SectionHeader *>(memory + offset);
    if (section->type == respack::SectionType::kResourceTable) {
      auto resources = reinterpret_cast<const respack::ResourceTableSectionHeader *>(section);
      auto desc = resources->GetResourcesDescriptors();
      for (auto it = desc.data(), ed = it + desc.size(); it != ed; ++it) {
        IndexResourceDescriptor(it, memory);
      }
    }
    offset += section->length;
  }
}

void assets::internal::UnloadAssetsFile() {
  delete[] default_assets;
  resource_map.clear();
}

std::span<const std::byte> assets::Load(std::string_view name) {
  auto it = resource_map.find(name);
  if (it == resource_map.end()) {
    throw std::runtime_error("Could not found resource: " + std::string(name));
  }
  return {default_assets + it->second->data_offset, it->second->data_length};
}

static std::map<std::string_view, std::forward_list<std::function<void(std::string_view)>>> delegate_resources;

void assets::Unload(std::string_view name) {
  auto it = delegate_resources.find(name);
  if (it != delegate_resources.end()) {
    auto &list = it->second;
    while (!list.empty()) {
      list.front()(name);
      list.pop_front();
    }
  }
}

std::span<const std::byte> assets::internal::Load(
    std::string_view name,
    std::function<void(std::string_view)> unload
) {
  delegate_resources[name].emplace_front(unload);
  return assets::Load(name);
}
