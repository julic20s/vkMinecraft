#pragma once
#ifndef VKMC_EVENT_DISPATCHER_H_
#define VKMC_EVENT_DISPATCHER_H_

#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

#include "handler.h"
#include "publisher.h"
#include "subscriber.h"

class EventDispatcher {
public:
  [[nodiscard]] EventPublisher &RegisterEvent(std::string &&name) noexcept {
    return publishers_[std::move(name)];
  }

  template <class... Args>
  EventSubscriber &SubscribeEvent(std::string &&name, Args &&...args) noexcept {
    return SubscribeEvent(std::move(name), EventHandler(std::forward<Args>(args)...));
  }

  EventSubscriber &SubscribeEvent(std::string &&name, EventHandler func) noexcept {
    auto &pub = RegisterEvent(std::move(name));
    pub.subscribers_.emplace_back(std::move(func));
    return pub.subscribers_.back().subscriber;
  }

private:
  std::unordered_map<std::string, EventPublisher> publishers_;
};

#endif // VKMC_EVENT_DISPATCHER_H_
