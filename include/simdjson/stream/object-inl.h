#ifndef SIMDJSON_STREAM_OBJECT_INL_H
#define SIMDJSON_STREAM_OBJECT_INL_H

#include "simdjson/stream/object.h"
#include "simdjson/stream/field.h"

namespace simdjson {
namespace stream {

//
// object
//
really_inline object::object(internal::json_iterator &_json) noexcept
  : json{_json} {
}

really_inline object::iterator object::begin() noexcept {
  return iterator(json); // If it's empty {}, we want it to be at_start (don't iterate)
}
really_inline object::iterator object::end() const noexcept {
  return iterator(json);
}
really_inline simdjson_result<element> object::operator[](std::string_view key) noexcept {
  for (auto [field, error] : *this) {
    if (error || key == field.key()) { return { element(field.json), error }; }
  }
  return { json, NO_SUCH_FIELD };
}

//
// object::iterator
//
really_inline object::iterator::iterator(internal::json_iterator &_json) noexcept
  : json{_json} {
}
really_inline simdjson_result<field> object::iterator::operator*() noexcept {
  // Check the comma
  if (at_start) {
    // If we're at the start, there's nothing to check. != would have bailed on empty {}
    at_start = false;
  } else {
    if (*json.advance() != ',') { return { field(json.get(), json), TAPE_ERROR }; }
  }

  // Get the key and skip the :
  const uint8_t *key = json.advance();
  auto error = (*key == '"' && *json.advance() == ':') ? SUCCESS : TAPE_ERROR;
  return { field(key, json), error };
}
really_inline object::iterator &object::iterator::operator++() noexcept {
  return *this;
}
really_inline bool object::iterator::operator!=(const object::iterator &) noexcept {
  // Stop if we hit }
  if (*json.get() == '}') { json.advance(); return false; }
  return true;
}

} // namespace stream

//
// simdjson_result<stream::object>
//
really_inline simdjson_result<stream::object>::simdjson_result(stream::object &&value) noexcept
    : internal::simdjson_result_base<stream::object>(std::forward<stream::object>(value)) {}
really_inline simdjson_result<stream::object>::simdjson_result(stream::object &&value, error_code error) noexcept
    : internal::simdjson_result_base<stream::object>(std::forward<stream::object>(value), error) {}

really_inline simdjson_result<stream::element> simdjson_result<stream::object>::operator[](std::string_view key) noexcept {
  if (error()) { return { first.json, error() }; }
  return first[key];
}

} // namespace simdjson

#endif // SIMDJSON_STREAM_DOCUMENTS_INL_H
