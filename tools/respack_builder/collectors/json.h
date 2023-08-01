#pragma once
#ifndef RESPACK_BUILDER_COLLECTORS_JSON_H_
#define RESPACK_BUILDER_COLLECTORS_JSON_H_

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <interfaces/collector.h>

class JsonCollector : public Collector {

  std::string_view GetTypeName() noexcept override {
    return "json";
  }

  respack::ResourceDescriptor CreateResourceDescriptor(std::filesystem::path file) override;

  void WriteData(std::filesystem::path file, Writer &out) override;

private:
  std::unordered_map<std::string, std::vector<std::uint8_t>> store_;
};

#endif // RESPACK_BUILDER_COLLECTORS_JSON_H_
