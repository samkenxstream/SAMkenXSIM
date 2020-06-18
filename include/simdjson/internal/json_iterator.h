#ifndef SIMDJSON_INTERNAL_JSON_ITERATOR_H
#define SIMDJSON_INTERNAL_JSON_ITERATOR_H

#include "simdjson/common_defs.h"

namespace simdjson {
namespace internal {

class json_iterator {
public:
  really_inline json_iterator(const uint32_t *_structural_index, const uint8_t *_buf, uint8_t *_string_buf, int _depth)
    : structural_index{_structural_index}, buf{_buf}, string_buf{_string_buf}, depth{_depth} {}
  json_iterator() = delete;
  really_inline const uint8_t * advance() noexcept { return &buf[*(structural_index++)]; }
  really_inline const uint8_t * get() const noexcept { return &buf[*structural_index]; }
  really_inline const uint8_t * prev() noexcept { return &buf[*(--structural_index)]; }
  really_inline const uint8_t * next() noexcept { return &buf[*(++structural_index)]; }
  really_inline const uint8_t * peek_prev() const noexcept { return &buf[*(structural_index-1)]; }
  really_inline const uint8_t * peek_next() const noexcept { return &buf[*(structural_index+1)]; }

  // For iterator interface
  really_inline bool operator!=(json_iterator &other) const noexcept { return structural_index != other.structural_index; }

  /** The current structural indexes */
  const uint32_t * structural_index;
  /** The buffer the structural indexes point at */
  const uint8_t * const buf;
  /**
   * The current location in the buffer to write strings to.
   *
   * This is why it cannot be copied: if there are multiple copies of this, then strings could
   * potentially copy over each other
   */
  uint8_t *string_buf;
  int depth;
}; // class structurals

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_JSON_ITERATOR_H
