#pragma once
#ifndef RESPACK_WRITER_H_
#define RESPACK_WRITER_H_

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <ostream>
#include <span>

class Writer {
public:
  Writer(std::ostream &stream);

  Writer &Write(const void *, std::size_t);

  [[nodiscard]] std::size_t Length() noexcept { return length_; }

private:
  std::ostream &stream_;
  std::size_t length_;
};

#endif // RESPACK_WRITER_H_
