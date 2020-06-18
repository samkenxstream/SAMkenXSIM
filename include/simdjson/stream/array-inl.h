#ifndef SIMDJSON_STREAM_ARRAY_INL_H
#define SIMDJSON_STREAM_ARRAY_INL_H

#include "simdjson/stream/array.h"
#include "simdjson/stream/element.h"
#include "simdjson/internal/logger.h"

namespace simdjson {
namespace stream {

//
// array
//
really_inline array::array(internal::json_iterator &_json) noexcept
  : json(_json) {
}
really_inline array_iterator array::begin() noexcept {
  return array_iterator(json, true);
}
really_inline array_iterator array::end() noexcept {
  return array_iterator(json, false);
}

//
// array_iterator
//
really_inline array_iterator::array_iterator(internal::json_iterator &json, bool _at_start) noexcept
  : value(json), depth{json.depth}, at_start{_at_start} {
}
really_inline simdjson_result<element&> array_iterator::operator*() noexcept {
  // Check the comma
  error_code error;
  if (at_start) {
    internal::logger::log_event("first array", value.json, true);
    at_start = false; // If we're at the start, there's no comma to check.
    error = SUCCESS;
  } else {
    internal::logger::log_event("next array", value.json);
    error = *value.json.advance() == ',' ? SUCCESS : TAPE_ERROR;
    if (error) { internal::logger::log_error("missing ,", value.json); }
  }

  return { value, error };
}
really_inline array_iterator &array_iterator::operator++() noexcept {
  return *this;
}
really_inline bool array_iterator::operator!=(const array_iterator &) noexcept {
  // Finish the previous value if it wasn't finished already
  if (!at_start) {
    // If finish() fails, it's because it found a stray } or ]
    if (!value.finish(depth)) {
      return true;
    }
  }
  // Stop if we hit ]
  if (*value.json.get() == ']') {
    internal::logger::log_end_event("array", value.json);
    value.json.advance();
    return false;
  }
  return true;
}

} // namespace stream

//
// simdjson_result<stream::array>
//
really_inline simdjson_result<stream::array>::simdjson_result(stream::array &&value) noexcept
    : internal::simdjson_result_base<stream::array>(std::forward<stream::array>(value)) {}
really_inline simdjson_result<stream::array>::simdjson_result(stream::array &&value, error_code error) noexcept
    : internal::simdjson_result_base<stream::array>(std::forward<stream::array>(value), error) {}

#if SIMDJSON_EXCEPTIONS

really_inline stream::array_iterator simdjson_result<stream::array>::begin() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.begin();
}
really_inline stream::array_iterator simdjson_result<stream::array>::end() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.end();
}

#endif // SIMDJSON_EXCEPTIONS

} // namespace simdjson

#endif // SIMDJSON_STREAM_ARRAY_INL_H
