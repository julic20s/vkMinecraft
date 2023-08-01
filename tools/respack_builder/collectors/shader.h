#pragma once
#ifndef RESPACK_BUILDER_COLLECTORS_BINARY_H_
#define RESPACK_BUILDER_COLLECTORS_BINARY_H_

#include <interfaces/collector.h>

class ShaderCollector : public Collector {

  std::string_view GetTypeName() noexcept override {
    return "shader";
  }

  respack::ResourceDescriptor CreateResourceDescriptor(std::filesystem::path file) override;

  void WriteData(std::filesystem::path file, Writer &out) override;
};

#endif // RESPACK_BUILDER_COLLECTORS_BINARY_H_
