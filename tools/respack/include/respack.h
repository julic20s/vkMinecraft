#pragma once
#ifndef MCRES_H_
#define MCRES_H_

#include <cstddef>
#include <cstdint>
#include <span>

namespace respack {

constexpr std::uint32_t kHeaderMagic = 0x434d4b56;

struct FileHeader {
  std::uint32_t magic;
  std::uint32_t sections;
};

enum class SectionType : std::uint32_t {
  kDataPool = 0,
  kResourceTable = 1,
};

struct SectionHeader {
  SectionType type;
  std::uint32_t length;
};

enum class ResourceType : std::uint16_t {
  kImage = 0,
  kJson = 1,
  kShader = 2,
};

struct ResourceDescriptor {
  ResourceType type;
  std::uint16_t name_length;
  std::uint32_t name_offset;
  std::uint32_t data_length;
  std::uint32_t data_offset;
};

struct ResourceTableSectionHeader : SectionHeader {
  std::uint32_t count;

  [[nodiscard]] std::span<const ResourceDescriptor> GetResourcesDescriptors() const noexcept {
    auto location = std::uintptr_t(&this->count) + sizeof(count);
    return {reinterpret_cast<ResourceDescriptor *>(location), count};
  }
};

struct ImageData {
  std::uint32_t width, height;
  std::uint32_t components;

  [[nodiscard]] std::span<const std::byte> GetData() const noexcept {
    auto location = std::uintptr_t(&this->components) + sizeof(components);
    return {reinterpret_cast<const std::byte *>(location), width * height * components};
  }
};

}; // namespace mcres

#endif // MCRES_H_
