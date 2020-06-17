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
  : json{_json} {
}
really_inline array::iterator array::begin() noexcept {
  return iterator(json, true);
}
really_inline array::iterator array::end() noexcept {
  return iterator(json, false);
}

//
// array::iterator
//
really_inline array::iterator::iterator(internal::json_iterator &_json, bool _at_start) noexcept
  : json{_json}, at_start{_at_start} {
}
really_inline simdjson_result<element> array::iterator::operator*() noexcept {
  // Check the comma
  error_code error;
  if (at_start) {
    internal::logger::log_event("first array", json, true);
    at_start = false; // If we're at the start, there's no comma to check.
    error = SUCCESS;
  } else {
    internal::logger::log_event("next array", json);
    error = *json.advance() == ',' ? SUCCESS : TAPE_ERROR;
    if (error) { internal::logger::log_error("missing ,", json); }
  }

  return { element(json), error };
}
really_inline array::iterator &array::iterator::operator++() noexcept {
  return *this;
}
really_inline bool array::iterator::operator!=(const array::iterator &) noexcept {
  // Stop if we hit ]
  if (*json.get() == ']') {
    internal::logger::log_end_event("array", json);
    json.advance();
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

really_inline stream::array::iterator simdjson_result<stream::array>::begin() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.begin();
}
really_inline stream::array::iterator simdjson_result<stream::array>::end() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.end();
}

#endif // SIMDJSON_EXCEPTIONS

} // namespace simdjson

#endif // SIMDJSON_STREAM_ARRAY_INL_H
