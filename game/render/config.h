#pragma once
#ifndef VKMC_RENDER_CONFIG_H_
#define VKMC_RENDER_CONFIG_H_

#include <cstddef>

struct RenderConfig {
  static constexpr std::size_t kMaxFramesInFlight = 2;
};

#endif // VKMC_RENDER_CONFIG_H_
