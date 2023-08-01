#pragma once
#ifndef VKMC_MATH_PERLIN_H_
#define VKMC_MATH_PERLIN_H_

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstring>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

extern const std::array<glm::vec2, 8> kPerlinGrad;

template <std::size_t Permu>
class Perlin {
public:
  template <std::invocable<> Rnd>
  Perlin(Rnd &&rand) {
    for (std::size_t i = 0; i != Permu; ++i) {
      permu_[i] = i;
    }
    std::shuffle(permu_, permu_ + Permu, rand);
    std::memcpy(permu_ + Permu, permu_, sizeof(std::size_t) * Permu);
  }

  [[nodiscard]] float operator()(glm::vec2 p) noexcept {
    glm::ivec2 flr{glm::floor(p)}, cir{flr.x + 1, flr.y + 1};
    auto pf = glm::fract(p);
    glm::vec2 t{Fade(pf.x), Fade(pf.y)};
    return Lerp(
        Lerp(Grad(flr, pf), Grad({cir.x, flr.y}, pf - glm::vec2(1, 0)), t.x),
        Lerp(Grad({flr.x, cir.y}, pf - glm::vec2(0, 1)), Grad(cir, pf - glm::vec2(1, 1)), t.x),
        t.y
    );
  }

private:
  auto Grad(glm::ivec2 p, glm::vec2 t) noexcept {
    return glm::dot(kPerlinGrad[Hash(p) % kPerlinGrad.size()], t);
  }

  float Fade(float t) noexcept {
    return t * t * t * (t * (t * 6 - 15) + 10);
  }

  float Lerp(float a, float b, float t) noexcept {
    return (1 - t) * a + t * b;
  }

  std::size_t Hash(glm::ivec2 p) noexcept {
    return permu_[permu_[p.x % Permu] + p.y % Permu];
  }

private:
  std::size_t permu_[Permu * 2];
};

#endif // VKMC_MATH_PERLIN_H_
