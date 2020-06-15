#ifndef SIMDJSON_STREAM_ELEMENT_INL_H
#define SIMDJSON_STREAM_ELEMENT_INL_H

#include "simdjson/stream/element.h"
#include "simdjson/stream/array.h"
#include "simdjson/stream/object.h"
#include "simdjson/internal/numberparsing.h"

namespace simdjson {
namespace stream {

//
// element
//
really_inline element::element(internal::json_iterator &_json) noexcept
  : json{_json} {
}

really_inline simdjson_result<array> element::get_array() noexcept {
  printf("start array\n");
  if (*json.advance() != '[') { return { json, INCORRECT_TYPE }; }
  return array(json);
}
really_inline simdjson_result<object> element::get_object() noexcept {
  if (*json.advance() != '{') { return { json, INCORRECT_TYPE }; }
  return object(json);
}
really_inline simdjson_result<raw_json_string> element::get_raw_json_string() noexcept {
  auto str = json.advance();
  auto error = *str == '"' ? SUCCESS : INCORRECT_TYPE;
  return { raw_json_string(str+1), error };
}
// really_inline simdjson_result<std::string_view> element::get_string() noexcept {
//   auto [str, error] = raw_string();
//   if (error) { return error; }
//   return str.unescape(json.string_buf);
// }
// really_inline simdjson_result<double> element::get_double() noexcept {
//   return parse_double(json.advance());
// }
really_inline simdjson_result<uint64_t> element::get_uint64() noexcept {
  return internal::parse_unsigned(json.advance());
}
really_inline simdjson_result<int64_t> element::get_int64() noexcept {
  return internal::parse_integer(json.advance());
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

inline array::iterator element::begin() noexcept(false) {
  return get_array().begin();
}
inline array::iterator element::end() noexcept(false) {
  return get_array().end();
}

#endif // SIMDJSON_EXCEPTIONS

WARN_UNUSED really_inline error_code element::skip() noexcept {
  printf("skip()\n");
  // Skip a matching number of open and close braces. We don't validate *anything* inside them;
  // nor do we validate that they match each other. We only validate elements you actually use.
  switch (*json.advance()) {
    case '[':
    case '{':
      break;
    case ']':
    case '}':
      return TAPE_ERROR;
    default:
      return SUCCESS;
  }
  // It's an array or hash. Count brackets until we are back to current depth.
  int depth = 1;
  do {
    printf("skip depth %d\n", depth);
    switch (*json.advance()) {
    case '[':
    case '{':
      depth++;
      break;
    case ']':
    case '}':
      depth--;
      break;
    default:
      break;
    }
  } while (depth > 0);
  return SUCCESS;
}

} // namespace stream

//
// simdjson_result<stream::element>
//
really_inline simdjson_result<stream::element>::simdjson_result(stream::element &&value) noexcept
    : internal::simdjson_result_base<stream::element>(std::forward<stream::element>(value)) {}
really_inline simdjson_result<stream::element>::simdjson_result(stream::element &&value, error_code error) noexcept
    : internal::simdjson_result_base<stream::element>(std::forward<stream::element>(value), error) {}

really_inline simdjson_result<stream::array> simdjson_result<stream::element>::get_array() noexcept {
  if (error()) { return { first.json, error() }; }
  return first.get_array();
}
really_inline simdjson_result<stream::object> simdjson_result<stream::element>::get_object() noexcept {
  if (error()) { return { first.json, error() }; }
  return first.get_object();
}
really_inline simdjson_result<stream::raw_json_string> simdjson_result<stream::element>::get_raw_json_string() noexcept {
  if (error()) { return error(); }
  return first.get_raw_json_string();
}
// really_inline simdjson_result<std::string_view> simdjson_result<stream::element>::get_string() noexcept {
//  if (error()) { return error(); }
//  return first.get_string();
// }
// really_inline simdjson_result<double> simdjson_result<stream::element>::get_double() noexcept {
//  if (error()) { return error(); }
//  return first.get_double();
// }
really_inline simdjson_result<uint64_t> simdjson_result<stream::element>::get_uint64() noexcept {
  if (error()) { return error(); }
  return first.get_uint64();
}
really_inline simdjson_result<int64_t> simdjson_result<stream::element>::get_int64() noexcept {
  if (error()) { return error(); }
  return first.get_int64();
}
// really_inline simdjson_result<bool> simdjson_result<stream::element>::get_bool() noexcept {
//  if (error()) { return error(); }
//  return first.get_bool();
// }

#if SIMDJSON_EXCEPTIONS
really_inline simdjson_result<stream::element>::operator stream::array() noexcept(false) {
  return get_array();
}
really_inline simdjson_result<stream::element>::operator stream::object() noexcept(false) {
  return get_object();
}
really_inline simdjson_result<stream::element>::operator stream::raw_json_string() noexcept(false) {
  return get_raw_json_string();
}
// really_inline simdjson_result<stream::element>::operator std::string_view() noexcept(false) {
//   return get_string();
// }
// really_inline simdjson_result<stream::element>::operator double() noexcept(false) {
//   return get_double();
// }
really_inline simdjson_result<stream::element>::operator uint64_t() noexcept(false) {
  return get_uint64();
}
really_inline simdjson_result<stream::element>::operator int64_t() noexcept(false) {
  return get_int64();
}
// really_inline simdjson_result<stream::element>::operator bool() noexcept(false) {
//   return get_bool();
// }

really_inline stream::array::iterator simdjson_result<stream::element>::begin() noexcept(false) {
  return get_array().begin();
}
really_inline stream::array::iterator simdjson_result<stream::element>::end() noexcept(false) {
  return get_array().end();
}
#endif // SIMDJSON_EXCEPTIONS

WARN_UNUSED really_inline error_code simdjson_result<stream::element>::skip() noexcept {
  if (error()) { return error(); }
  return first.skip();
}

} // namespace simdjson

#endif // SIMDJSON_STREAM_ELEMENT_INL_H
