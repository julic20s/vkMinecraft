#include <fstream>

#include <nlohmann/json.hpp>

#include "json.h"

respack::ResourceDescriptor
JsonCollector::CreateResourceDescriptor(std::filesystem::path file) {
  std::ifstream ifs(file);
  auto json = nlohmann::json::parse(ifs);
  auto data = nlohmann::json::to_msgpack(json);

  respack::ResourceDescriptor result;
  result.type = respack::ResourceType::kJson;
  result.data_length = data.size();

  store_[std::move(file).string()] = std::move(data);
  return result;
}

void JsonCollector::WriteData(std::filesystem::path file, Writer &out) {
  auto &data = store_[file.string()];
  out.Write(data.data(), data.size());
}
