#include <fstream>
#include <algorithm>

#include "shader.h"

respack::ResourceDescriptor
ShaderCollector::CreateResourceDescriptor(std::filesystem::path file) {
  respack::ResourceDescriptor result;
  result.type = respack::ResourceType::kShader;
  result.data_length = std::filesystem::file_size(file);
  return result;
}

void ShaderCollector::WriteData(std::filesystem::path file, Writer &out) {
  std::ifstream ifs(file, std::ios::binary);
  if (!ifs.is_open()) {
    throw std::runtime_error("Failed to open file!");
  }
  for (auto bytes = std::filesystem::file_size(file); bytes; bytes--) {
    char ch = ifs.get();
    out.Write(&ch, 1);
  }
}
