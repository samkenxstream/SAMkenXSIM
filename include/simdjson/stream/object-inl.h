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
  return iterator(json, true);
}
really_inline object::iterator object::end() noexcept {
  return iterator(json, false);
}
really_inline simdjson_result<element> object::operator[](std::string_view key) noexcept {
  internal::logger::log_event("lookup key", json);
  for (auto [field, error] : *this) {
    if (error || key == field.key()) {
      if (!error) { internal::logger::log_event("found key", json); }
      return { element(field.json), error };
    }
  }
  internal::logger::log_error("no such key", json);
  return { json, NO_SUCH_FIELD };
}

//
// object::iterator
//
really_inline object::iterator::iterator(internal::json_iterator &_json, bool _at_start) noexcept
  : json{_json}, at_start{_at_start} {
}
really_inline simdjson_result<field> object::iterator::operator*() noexcept {
  // Check the comma
  if (at_start) {
    // If we're at the start, there's nothing to check. != would have bailed on empty {}
    internal::logger::log_event("first field", json, true);
    at_start = false;
  } else {
    internal::logger::log_event("next field", json);
    if (*json.advance() != ',') {
      internal::logger::log_error("missing ,", json);
      return { field(json.get(), json), TAPE_ERROR };
    }
  }

  // Get the key and skip the :
  const uint8_t *key = json.advance();
  if (*key != '"') { assert(error); internal::logger::log_error("non-string key", json); }
  auto error = (*key == '"' && *json.advance() == ':') ? SUCCESS : TAPE_ERROR;
  if (*json.peek_prev() != ':') { assert(error); internal::logger::log_error("missing :", json); }
  return { field(key, json), error };
}
really_inline object::iterator &object::iterator::operator++() noexcept {
  return *this;
}
really_inline bool object::iterator::operator!=(const object::iterator &) noexcept {
  // Stop if we hit }
  if (*json.get() == '}') {
    internal::logger::log_end_event("object", json);
    return false;
  }
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

#if SIMDJSON_EXCEPTIONS

really_inline stream::object::iterator simdjson_result<stream::object>::begin() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.begin();
}
really_inline stream::object::iterator simdjson_result<stream::object>::end() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.end();
}

#endif // SIMDJSON_EXCEPTIONS

} // namespace simdjson

#endif // SIMDJSON_STREAM_DOCUMENTS_INL_H
