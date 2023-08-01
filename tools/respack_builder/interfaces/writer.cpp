#include "writer.h"

Writer::Writer(std::ostream &stream) : stream_(stream), length_(0) {}

Writer &Writer::Write(const void *ptr, std::size_t size) {
  stream_.write(reinterpret_cast<const char *>(ptr), size);
  length_ += size;
  return *this;
}
