#pragma once
#ifndef VKMC_EVENT_H_
#define VKMC_EVENT_H_

#include "event/dispatcher.h"
#include "event/handler.h"
#include "event/publisher.h"
#include "event/scope.h"
#include "event/subscriber.h"

namespace events {

constexpr auto kMouse = "mouse";
constexpr auto kMouseLeftClick = "mouse_left_click";
constexpr auto kWindowSize = "window_size";

/// The global event dispatcher
extern EventDispatcher global;

} // namespace events

#endif // VKMC_EVENT_H_
