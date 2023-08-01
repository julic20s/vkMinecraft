#pragma once
#ifndef VKMC_EVENT_PUBLISHER_H_
#define VKMC_EVENT_PUBLISHER_H_

#include <tuple>
#include <list>
#include <utility>

#include "handler.h"
#include "subscriber.h"

class EventPublisher : NonCopyMove {
  friend class EventDispatcher;

private:
  struct FunctionSubscriber {
    EventSubscriber subscriber;
    EventHandler handler;

    FunctionSubscriber(EventHandler &&h) noexcept : handler(std::move(h)) {}
  };

  std::list<FunctionSubscriber> subscribers_;

public:
  template <class... Args>
  void EmitArgs(Args &&...args) {
    EmitTuple(std::make_tuple(std::forward<Args>(args)...));
  }

  template <class... Args>
  void EmitTuple(const std::tuple<Args...> &args) {
    for (auto it = subscribers_.begin(); it != subscribers_.end();) {
      if (it->subscriber.IsUnsubscribeRequested()) {
        it = subscribers_.erase(it);
      } else {
        it->handler(args);
        ++it;
      }
    }
  }
};

#endif // VKMC_EVENT_PUBLISHER_H_
