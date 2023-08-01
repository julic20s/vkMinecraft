#pragma once
#ifndef VKMC_COMMON_CLASSES_H_
#define VKMC_COMMON_CLASSES_H_

/// Simplify writing for non-copy types
struct NonCopy {
  NonCopy() = default;
  NonCopy(const NonCopy &) = delete;
};

/// Simplify writing for non-move types
struct NonMove {
  NonMove() = default;
  NonMove(NonMove &&) = delete;
};

/// Simplify writing for non-copy and non-move types
struct NonCopyMove : NonCopy, NonMove {};

#endif // VKMC_COMMON_CLASSES_H_
