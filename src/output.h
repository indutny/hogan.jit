#ifndef _SRC_OUTPUT_H_
#define _SRC_OUTPUT_H_

#include <assert.h> // assert
#include <sys/types.h> // size_t
#include <string.h> // strlen, memcpy

namespace hogan {

const int initialCapacity = 128;

class TemplateOutput {
 public:
  TemplateOutput() {
    chunks = new size_t[initialCapacity << 2];
    capacity = initialCapacity;
    total = 0;
    items = 0;
  }

  ~TemplateOutput() {
    delete chunks;
  }

  inline void Realloc() {
    if (capacity != 0) return;

    // Reallocate chunks
    size_t* new_chunks = new size_t[items << 1];
    memcpy(new_chunks, chunks, items * sizeof(*chunks));
    delete chunks;
    chunks = new_chunks;

    // Increase capactiy and size
    capacity += items >> 2;
  }

  void Push(const char* chunk, const size_t size) {
    size_t size_ = size == 0 ? strlen(chunk) : size;
    chunks[items] = reinterpret_cast<const size_t>(chunk);
    chunks[items + 1] = size_;

    items += 4;
    capacity--;
    total += size_;
    Realloc();
  }

  inline char* Join() {
    char* result = new char[total + 1];
    result[total] = 0;

    off_t offset = 0;
    for (size_t i = 0; i < items; i += 4) {
      size_t size = chunks[i + 1];
      memcpy(result + offset,
             reinterpret_cast<char*>(chunks[i]),
             size);
      offset += size;
    }
    assert(static_cast<size_t>(offset) == total);

    return result;
  }

 private:
  size_t* chunks;
  size_t capacity; // how many item we can insert
  size_t items; // count of items
  size_t total; // total byte (sum of chunks' lengths)
};

} // namespace hogan

#endif // _SRC_OUTPUT_H_
