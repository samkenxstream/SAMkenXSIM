#ifndef SIMDJSON_STREAM_DOCUMENT_INL_H
#define SIMDJSON_STREAM_DOCUMENT_INL_H

#include "simdjson/stream/document.h"

namespace simdjson {
namespace stream {

//
// document
//
really_inline document::document(const dom::parser &parser, const uint8_t *buf) noexcept
  : json{&parser.implementation->structural_indexes[0], buf, parser.doc.string_buf.get()} {
}

really_inline simdjson_result<array> document::get_array() noexcept {
  return element(json).get_array();
}
really_inline simdjson_result<object> document::get_object() noexcept {
  return element(json).get_object();
}
really_inline simdjson_result<raw_json_string> document::get_raw_json_string() noexcept {
  return element(json).get_raw_json_string();
}
// really_inline simdjson_result<std::string_view> document::get_string() noexcept {
//   return element(json).get_string();
// }
// really_inline simdjson_result<double> document::get_double() noexcept {
//   return element(json).get_double();
// }
really_inline simdjson_result<uint64_t> document::get_uint64() noexcept {
  return element(json).get_uint64();
}
really_inline simdjson_result<int64_t> document::get_int64() noexcept {
  return element(json).get_int64();
}
// really_inline simdjson_result<bool> document::get_bool() noexcept {
//   return element(json).get_bool();
// }

// TODO users should never have to call this for things to work. Figure out how to make it happen
// in destructors or some other automatic mechanism.
inline error_code document::skip() noexcept {
  return element(json).skip();
}

#ifdef SIMDJSON_EXCEPTIONS
really_inline document::operator array() noexcept(false) {
  return element(json);
}
really_inline document::operator object() noexcept(false ){
  return element(json);
}
really_inline document::operator raw_json_string() noexcept(false) {
  return element(json);
}
// really_inline document::operator std::string_view() noexcept(false) {
//   return element(json);
// }
// really_inline document::operator double() noexcept(false) {
//   return element(json);
// }
really_inline document::operator uint64_t() noexcept(false) {
  return element(json);
}
really_inline document::operator int64_t() noexcept(false) {
  return element(json);
}
// really_inline element::operator bool() noexcept(false) {
//   return element(json);
// }
#endif // SIMDJSON_EXCEPTIONS

} // namespace stream

//
// simdjson_result<stream::document>
//
really_inline simdjson_result<stream::document>::simdjson_result(stream::document &&value) noexcept
    : internal::simdjson_result_base<stream::document>(std::forward<stream::document>(value)) {}
really_inline simdjson_result<stream::document>::simdjson_result(stream::document &&value, error_code error) noexcept
    : internal::simdjson_result_base<stream::document>(std::forward<stream::document>(value), error) {}

really_inline simdjson_result<stream::array> simdjson_result<stream::document>::get_array() noexcept {
  if (error()) { return { first.json, error() }; }
  return first.get_array();
}
really_inline simdjson_result<stream::object> simdjson_result<stream::document>::get_object() noexcept {
  if (error()) { return { first.json, error() }; }
  return first.get_object();
}
really_inline simdjson_result<stream::raw_json_string> simdjson_result<stream::document>::get_raw_json_string() noexcept {
  if (error()) { return error(); }
  return first.get_raw_json_string();
}
// really_inline simdjson_result<std::string_view> simdjson_result<stream::document>::get_string() noexcept {
//  if (error()) { return error(); }
//  return first.get_string();
// }
// really_inline simdjson_result<double> simdjson_result<stream::document>::get_double() noexcept {
//  if (error()) { return error(); }
//  return first.get_double();
// }
really_inline simdjson_result<uint64_t> simdjson_result<stream::document>::get_uint64() noexcept {
  if (error()) { return error(); }
  return first.get_uint64();
}
really_inline simdjson_result<int64_t> simdjson_result<stream::document>::get_int64() noexcept {
  if (error()) { return error(); }
  return first.get_int64();
}
// really_inline simdjson_result<bool> simdjson_result<stream::document>::get_bool() noexcept {
//  if (error()) { return error(); }
//  return first.get_bool();
// }

#ifdef SIMDJSON_EXCEPTIONS
really_inline simdjson_result<stream::document>::operator stream::array() noexcept(false) {
  return get_array();
}
really_inline simdjson_result<stream::document>::operator stream::object() noexcept(false) {
  return get_object();
}
really_inline simdjson_result<stream::document>::operator stream::raw_json_string() noexcept(false) {
  return get_raw_json_string();
}
// really_inline simdjson_result<stream::document>::operator std::string_view() noexcept(false) {
//   return get_string();
// }
// really_inline simdjson_result<stream::document>::operator double() noexcept(false) {
//   return get_double();
// }
really_inline simdjson_result<stream::document>::operator uint64_t() noexcept(false) {
  return get_uint64();
}
really_inline simdjson_result<stream::document>::operator int64_t() noexcept(false) {
  return get_int64();
}
// really_inline simdjson_result<stream::document>::operator bool() noexcept(false) {
//   return get_bool();
// }
#endif // SIMDJSON_EXCEPTIONS

} // namespace simdjson

#endif // SIMDJSON_STREAM_DOCUMENT_INL_H
