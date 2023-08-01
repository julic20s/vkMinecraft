#pragma once
#ifndef VKMC_BASE_ASSETS_JSON_H_
#define VKMC_BASE_ASSETS_JSON_H_

#include <string_view>

#include <nlohmann/json.hpp>

namespace assets {

/// Load json from default.assets
[[nodiscard]] const nlohmann::json &LoadJson(std::string_view name);

} // namespace assets

#endif // VKMC_BASE_ASSETS_JSON_H_
