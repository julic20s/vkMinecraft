#pragma once
#ifndef VKMC_EVENT_HANDLER_H_
#define VKMC_EVENT_HANDLER_H_

#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

/// Handler provide a safer function than std::function<...>.
/// Some unsafe function maybe passed to event dispatcher when use
/// std::function<...> such as: [&] (std::any arg) { local_variable++; }
/// We use function pointer to instead std::function<...>.
/// Because lambda function capture was banned for function pointer
class EventHandler {
private:
  std::function<void(const void *)> func_;

  template <class Func, class State, class Tuple, std::size_t... Indices>
  static void ApplyLValueConstReferences(
      Func func, const State &state, const Tuple &tuple,
      std::integer_sequence<std::size_t, Indices...>
  ) {
    func(state, std::get<Indices>(tuple)...);
  }

  template <class Func, class Tuple, std::size_t... Indices>
  static void ApplyLValueConstReferences(
      Func func, const Tuple &tuple,
      std::integer_sequence<std::size_t, Indices...>
  ) {
    func(std::get<Indices>(tuple)...);
  }

public:
  template <class Tp>
  static constexpr auto is_permitted_argument =
      (!std::is_reference_v<Tp>) ||
      (std::is_lvalue_reference_v<Tp> && std::is_const_v<std::remove_reference_t<Tp>>);

  template <class State, class... Args>
  requires(!std::is_reference_v<State> && ... && is_permitted_argument<Args>)
  EventHandler(State &&state, void (*func)(State, Args...)) noexcept {
    // Use value capture on state and func
    func_ = [state = std::move(state), func](const void *arg) {
      auto &tuple = *reinterpret_cast<const std::tuple<std::remove_reference_t<Args>...> *>(arg);
      ApplyLValueConstReferences(func, state, tuple, std::make_index_sequence<sizeof...(Args)>{});
    };
  }

  template <class... Args>
  requires(... && is_permitted_argument<Args>)
  EventHandler(void (*func)(Args...)) noexcept {
    // Use value capture on state and func
    func_ = [func](const void *arg) {
      auto &tuple = *reinterpret_cast<const std::tuple<std::remove_reference_t<Args>...> *>(arg);
      ApplyLValueConstReferences(func, tuple, std::make_index_sequence<sizeof...(Args)>{});
    };
  }

  template <class... Args>
  void operator()(const std::tuple<Args...> &args) noexcept { func_(&args); }
};

#endif // VKMC_EVENT_HANDLER_H_
