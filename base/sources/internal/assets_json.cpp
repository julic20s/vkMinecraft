#include <map>
#include <span>
#include <string_view>

#include <assets/json.h>

#include <nlohmann/json.hpp>

#include "assets.h"

static std::map<std::string_view, nlohmann::json> store;

static const nlohmann::json &StoreJson(std::string_view name, std::span<const std::byte> bytes) {
  auto [it, add] = store.emplace(name, nlohmann::json::from_msgpack(bytes));
  return it->second;
}

static void UnloadJson(std::string_view name) {
  store.erase(name);
}

const nlohmann::json &assets::LoadJson(std::string_view name) {
  auto find = store.find(name);
  if (find != store.end()) {
    return find->second;
  }

  return StoreJson(name, internal::Load(name, UnloadJson));
}
