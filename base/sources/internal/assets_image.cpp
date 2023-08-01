#include <string_view>

#include <respack.h>

#include <assets/image.h>

#include "assets.h"

assets::Image assets::LoadImage(std::string_view name) {
  auto data = Load(name);
  auto image = reinterpret_cast<const respack::ImageData *>(data.data());
  return {image->width, image->height, image->GetData().data()};
}
