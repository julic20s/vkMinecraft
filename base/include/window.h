#pragma once
#ifndef VKMC_BASE_WINDOW_H_
#define VKMC_BASE_WINDOW_H_

#include <cstdint>

namespace window {

[[nodiscard]] std::uint16_t GetWidth() noexcept;

[[nodiscard]] std::uint16_t GetHeight() noexcept;

} // namespace window

#endif // VKMC_BASE_WINDOW_H_
