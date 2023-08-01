#pragma once
#ifndef VKMC_EVENT_AUTOMATIC_H_
#define VKMC_EVENT_AUTOMATIC_H_

#include <forward_list>
#include <utility>

#include "subscriber.h"
#include "dispatcher.h"

class EventScope {
private:
  std::forward_list<EventSubscriber *> subscribers_;

public:
  ~EventScope() noexcept {
    for (auto subscriber : subscribers_) {
      subscriber->Unsubscribe();
    }
  }

  /// Subscribe the event until the scope destroy
  template <class... Args>
  void SubscribeInScope(EventDispatcher &d, Args &&...args) noexcept {
    subscribers_.emplace_front(&d.SubscribeEvent(std::forward<Args>(args)...));
  }
};

#endif // VKMC_EVENT_AUTOMATIC_H_
