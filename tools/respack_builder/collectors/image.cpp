#include <stb_image.h>

#include "image.h"

respack::ResourceDescriptor
ImageCollector::CreateResourceDescriptor(std::filesystem::path file) {
  respack::ResourceDescriptor result;
  result.type = respack::ResourceType::kImage;
  int x, y, n;
  auto info = stbi_info(file.string().c_str(), &x, &y, &n);
  result.data_length = x * y * 4 + sizeof(respack::ImageData);
  return result;
}

void ImageCollector::WriteData(std::filesystem::path file, Writer &out) {
  int x, y, n;
  auto data = stbi_load(file.string().c_str(), &x, &y, &n, 4);
  respack::ImageData meta{
      .width = std::uint32_t(x),
      .height = std::uint32_t(y),
      .components = 4,
  };
  out.Write(&meta, sizeof(meta));
  out.Write(data, x * y * n);
  stbi_image_free(data);
}
