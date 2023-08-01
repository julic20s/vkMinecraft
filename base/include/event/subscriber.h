#pragma once
#ifndef VKMC_EVENT_SUBSCRIBER_H_
#define VKMC_EVENT_SUBSCRIBER_H_

#include <common/classes.h>

/// Provide an object that can unsubscribe event manually
class EventSubscriber : NonCopy {
private:
  bool request_unsubscribed_;

public:
  EventSubscriber() noexcept : request_unsubscribed_(false) {}

  EventSubscriber(EventSubscriber &&mov) noexcept {
    request_unsubscribed_ = mov.request_unsubscribed_;
  }

  void Unsubscribe() noexcept {
    request_unsubscribed_ = true;
  }

  [[nodiscard]] bool IsUnsubscribeRequested() noexcept {
    return request_unsubscribed_;
  }

  ~EventSubscriber() noexcept {
    Unsubscribe();
  }
};

#endif // VKMC_EVENT_SUBSCRIBER_H_
