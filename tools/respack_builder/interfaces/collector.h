#pragma once
#ifndef RESPACK_COLLECTOR_H_
#define RESPACK_COLLECTOR_H_

#include <filesystem>

#include <respack.h>

#include "writer.h"

struct Collector {

  virtual ~Collector() {}

  [[nodiscard]] virtual std::string_view GetTypeName() noexcept {
    return "unknown";
  }

  [[nodiscard]] virtual respack::ResourceDescriptor
  CreateResourceDescriptor(std::filesystem::path file) = 0;

  virtual void WriteData(std::filesystem::path file, Writer &out) {}
};

#endif // RESPACK_COLLECTOR_H_
