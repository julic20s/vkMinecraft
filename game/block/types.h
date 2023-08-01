#pragma once
#ifndef VKMC_BLOCK_TYPES_H_
#define VKMC_BLOCK_TYPES_H_

#include <cstdint>

using BlockId = std::uint32_t;
using TextureId = std::uint32_t;

namespace blocks {

static constexpr BlockId kAir = ~0;

}

enum class FaceDirection : std::uint32_t {
  kNorth = 0,
  kSouth = 1,
  kWest = 2,
  kEast = 3,
  kTop = 4,
  kBottom = 5,
};

#endif // VKMC_BLOCK_TYPES_H_
