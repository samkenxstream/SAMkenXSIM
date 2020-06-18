#ifndef SIMDJSON_STREAM_ELEMENT_INL_H
#define SIMDJSON_STREAM_ELEMENT_INL_H

#include "simdjson/stream/element.h"
#include "simdjson/stream/array.h"
#include "simdjson/stream/object.h"
#include "simdjson/internal/logger.h"
#include "simdjson/internal/numberparsing.h"

namespace simdjson {
namespace stream {

//
// element
//
really_inline element::element(internal::json_iterator &_json) noexcept
  : json{_json} {
}
really_inline element::element(element && other) noexcept
  : json{other.json} {
}

really_inline simdjson_result<array> element::get_array() noexcept {
  assert(!consumed);
  consumed = true;
  internal::logger::log_start_event("array", json);
  if (*json.advance() != '[') { internal::logger::log_error("not an array", json); return { json, INCORRECT_TYPE }; }
  json.depth++;
  return array(json);
}
really_inline simdjson_result<object> element::get_object() noexcept {
  assert(!consumed);
  consumed = true;
  internal::logger::log_start_event("object", json);
  if (*json.advance() != '{') { internal::logger::log_error("not an object", json); return { json, INCORRECT_TYPE }; }
  json.depth++;
  return object(json);
}
really_inline simdjson_result<raw_json_string> element::get_raw_json_string() noexcept {
  assert(!consumed);
  consumed = true;
  internal::logger::log_event("raw_json_string", json);
  auto str = json.advance();
  auto error = *str == '"' ? SUCCESS : INCORRECT_TYPE;
  if (error) { internal::logger::log_error("not a string", json); }
  return { raw_json_string(str+1), error };
}
// really_inline simdjson_result<std::string_view> element::get_string() noexcept {
//   assert(!consumed);
//   consumed = true;
//   internal::logger::log_event("string", json);
//   auto [str, error] = get_raw_json_string();
//   if (error) { return error; }
//   return str.unescape(json.string_buf);
// }
// really_inline simdjson_result<double> element::get_double() noexcept {
//   assert(!consumed);
//   consumed = true;
//   internal::logger::log_event("double", json);
//   return parse_double(json.advance());
// }
really_inline simdjson_result<uint64_t> element::get_uint64() noexcept {
  assert(!consumed);
  consumed = true;
  internal::logger::log_event("unsigned", json);
  return internal::parse_unsigned(json.advance());
}
really_inline simdjson_result<int64_t> element::get_int64() noexcept {
  assert(!consumed);
  consumed = true;
  internal::logger::log_event("integer", json);
  return internal::parse_integer(json.advance());
}

WARN_UNUSED really_inline bool element::finish(int parent_depth) noexcept {
  // If the user didn't even do anything with the element, we have to skip it wholesale.
  // Figure out whether it's an array, hash or other, and move on from there.
  // NOTE we are hoping the compiler optimizes this one away. It's a flag whose true/false value
  // is almost always statically knowable.
  if (!consumed) {
    // Skip a matching number of open and close braces. We don't validate *anything* inside them;
    // nor do we validate that they match each other. We only validate elements you actually use.
    switch (*json.advance()) {
      case '[':
      case '{':
        internal::logger::log_start_event("skip", json, true);
        json.depth++;
        break;
      case '}':
      case ']':
      case ',':
      case ':':
        // This is not OK. Go backwards.
        json.prev();
        return false;
      default:
        return true;
    }
  }
  // Now we're past any scalar values.
  // If we're inside an array or hash, count brackets until we are back to current depth.
  if (json.depth > parent_depth) {
    while (true) {
      switch (*json.advance()) {
      case '[':
      case '{':
        internal::logger::log_start_event("skip", json, true);
        json.depth++;
        break;
      case ']':
      case '}':
        internal::logger::log_end_event("skip", json, true);
        json.depth--;
        if (json.depth == parent_depth) { return true; }
        break;
      default:
        internal::logger::log_event("skip", json, true);
        break;
      }
    }
  }
  return true;
}

#if SIMDJSON_EXCEPTIONS
really_inline element::operator array() noexcept(false) {
  return get_array();
}
really_inline element::operator object() noexcept(false ){
  return get_object();
}
really_inline element::operator raw_json_string() noexcept(false) {
  return get_raw_json_string();
}
// really_inline element::operator std::string_view() noexcept(false) {
//   return get_string();
// }
// really_inline element::operator double() noexcept(false) {
//   return get_double();
// }
really_inline element::operator uint64_t() noexcept(false) {
  return get_uint64();
}
really_inline element::operator int64_t() noexcept(false) {
  return get_int64();
}
// really_inline element::operator bool() noexcept(false) {
//   return get_bool();
// }

inline array_iterator element::begin() noexcept(false) {
  return get_array().begin();
}
inline array_iterator element::end() noexcept(false) {
  // We don't call get_array because it advances the iterator, and we already did that in begin()
  // Here we assume the open array was already checked by begin()
  return array(json).end();
}

#endif // SIMDJSON_EXCEPTIONS

} // namespace stream

//
// simdjson_result<stream::element&>
//
really_inline simdjson_result<stream::element&>::simdjson_result(stream::element &value) noexcept
    : internal::simdjson_result_base<stream::element&>(value) {}
really_inline simdjson_result<stream::element&>::simdjson_result(stream::element &value, error_code error) noexcept
    : internal::simdjson_result_base<stream::element&>(value, error) {}

really_inline simdjson_result<stream::array> simdjson_result<stream::element&>::get_array() noexcept {
  if (error()) { return { first.json, error() }; }
  return first.get_array();
}
really_inline simdjson_result<stream::object> simdjson_result<stream::element&>::get_object() noexcept {
  if (error()) { return { first.json, error() }; }
  return first.get_object();
}
really_inline simdjson_result<stream::raw_json_string> simdjson_result<stream::element&>::get_raw_json_string() noexcept {
  if (error()) { return error(); }
  return first.get_raw_json_string();
}
// really_inline simdjson_result<std::string_view> simdjson_result<stream::element&>::get_string() noexcept {
//   if (error()) { return error(); }
//   return first.get_string();
// }
// really_inline simdjson_result<double> simdjson_result<stream::element&>::get_double() noexcept {
//   if (error()) { return error(); }
//   return first.get_double();
// }
really_inline simdjson_result<uint64_t> simdjson_result<stream::element&>::get_uint64() noexcept {
  if (error()) { return error(); }
  return first.get_uint64();
}
really_inline simdjson_result<int64_t> simdjson_result<stream::element&>::get_int64() noexcept {
  if (error()) { return error(); }
  return first.get_int64();
}
// really_inline simdjson_result<bool> simdjson_result<stream::element&>::get_bool() noexcept {
//   if (error()) { return error(); }
//   return first.get_bool();
// }

#if SIMDJSON_EXCEPTIONS

really_inline simdjson_result<stream::element&>::operator stream::array() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
really_inline simdjson_result<stream::element&>::operator stream::object() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
really_inline simdjson_result<stream::element&>::operator stream::raw_json_string() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
// really_inline simdjson_result<stream::element&>::operator std::string_view() noexcept(false) {
//   if (error()) { throw simdjson_error(error()); }
//   return first;
// }
// really_inline simdjson_result<stream::element&>::operator double() noexcept(false) {
//   if (error()) { throw simdjson_error(error()); }
//   return first;
// }
really_inline simdjson_result<stream::element&>::operator uint64_t() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
really_inline simdjson_result<stream::element&>::operator int64_t() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
// really_inline simdjson_result<stream::element&>::operator bool() noexcept(false) {
//   if (error()) { throw simdjson_error(error()); } }
//   return first;
// }

really_inline stream::array_iterator simdjson_result<stream::element&>::begin() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.begin();
}
really_inline stream::array_iterator simdjson_result<stream::element&>::end() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.end();
}

#endif // SIMDJSON_EXCEPTIONS

} // namespace simdjson

#endif // SIMDJSON_STREAM_ELEMENT_INL_H
