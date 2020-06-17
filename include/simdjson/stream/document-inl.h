#ifndef SIMDJSON_STREAM_DOCUMENT_INL_H
#define SIMDJSON_STREAM_DOCUMENT_INL_H

#include "simdjson/stream/document.h"
#include "simdjson/internal/logger.h"

namespace simdjson {
namespace stream {

//
// document
//
really_inline document::document(const dom::parser &parser, const uint8_t *buf) noexcept
  : json{&parser.implementation->structural_indexes[0], buf, parser.doc.string_buf.get()} {
}

really_inline element document::root() noexcept {
  return element(json);
}

really_inline simdjson_result<array> document::get_array() noexcept {
  return root().get_array();
}
really_inline simdjson_result<object> document::get_object() noexcept {
  return root().get_object();
}
really_inline simdjson_result<raw_json_string> document::get_raw_json_string() noexcept {
  return root().get_raw_json_string();
}
// really_inline simdjson_result<std::string_view> document::get_string() noexcept {
//   return root().get_string();
// }
// really_inline simdjson_result<double> document::get_double() noexcept {
//   return root().get_double();
// }
really_inline simdjson_result<uint64_t> document::get_uint64() noexcept {
  return root().get_uint64();
}
really_inline simdjson_result<int64_t> document::get_int64() noexcept {
  return root().get_int64();
}
// really_inline simdjson_result<bool> document::get_bool() noexcept {
//   return root().get_bool();
// }

// TODO users should never have to call this for things to work. Figure out how to make it happen
// in destructors or some other automatic mechanism.
inline error_code document::skip() noexcept {
  return root().skip();
}

#if SIMDJSON_EXCEPTIONS
really_inline document::operator array() noexcept(false) {
  return root();
}
really_inline document::operator object() noexcept(false) {
  return root();
}
really_inline document::operator raw_json_string() noexcept(false) {
  return root();
}
// really_inline document::operator std::string_view() noexcept(false) {
//   return root();
// }
// really_inline document::operator double() noexcept(false) {
//   return root();
// }
really_inline document::operator uint64_t() noexcept(false) {
  return root();
}
really_inline document::operator int64_t() noexcept(false) {
  return root();
}
// really_inline element::operator bool() noexcept(false) {
//   return root();
// }

really_inline array::iterator document::begin() noexcept(false) {
  return get_array().begin();
}
really_inline array::iterator document::end() noexcept(false) {
  return get_array().end();
}
#endif // SIMDJSON_EXCEPTIONS

} // namespace stream

//
// simdjson_result<stream::document>
//
really_inline simdjson_result<stream::document>::simdjson_result(stream::document &&value) noexcept
    : internal::simdjson_result_base<stream::document>(std::forward<stream::document>(value)) {}
really_inline simdjson_result<stream::document>::simdjson_result(stream::document &&value, error_code error) noexcept
    : internal::simdjson_result_base<stream::document>(std::forward<stream::document>(value), error) {}

really_inline simdjson_result<stream::element> simdjson_result<stream::document>::root() noexcept {
  return { first.json, error() };
}

really_inline simdjson_result<stream::array> simdjson_result<stream::document>::get_array() & noexcept {
  return root().get_array();
}
really_inline simdjson_result<stream::object> simdjson_result<stream::document>::get_object() & noexcept {
  return root().get_object();
}
really_inline simdjson_result<stream::raw_json_string> simdjson_result<stream::document>::get_raw_json_string() noexcept {
  return root().get_raw_json_string();
}
// really_inline simdjson_result<std::string_view> simdjson_result<stream::document>::get_string() noexcept {
//  return root().get_string();
// }
// really_inline simdjson_result<double> simdjson_result<stream::document>::get_double() noexcept {
//  return root().get_double();
// }
really_inline simdjson_result<uint64_t> simdjson_result<stream::document>::get_uint64() noexcept {
  return root().get_uint64();
}
really_inline simdjson_result<int64_t> simdjson_result<stream::document>::get_int64() noexcept {
  return root().get_int64();
}
// really_inline simdjson_result<bool> simdjson_result<stream::document>::get_bool() noexcept {
//  return root().get_bool();
// }

really_inline error_code simdjson_result<stream::document>::skip() noexcept {
  return root().skip();
}

#if SIMDJSON_EXCEPTIONS
really_inline simdjson_result<stream::document>::operator stream::array() noexcept(false) {
  return root();
}
really_inline simdjson_result<stream::document>::operator stream::object() noexcept(false) {
  return root();
}
really_inline simdjson_result<stream::document>::operator stream::raw_json_string() noexcept(false) {
  return root();
}
// really_inline simdjson_result<stream::document>::operator std::string_view() noexcept(false) {
//   return root();
// }
// really_inline simdjson_result<stream::document>::operator double() noexcept(false) {
//   return root();
// }
really_inline simdjson_result<stream::document>::operator uint64_t() noexcept(false) {
  return root();
}
really_inline simdjson_result<stream::document>::operator int64_t() noexcept(false) {
  return root();
}
// really_inline simdjson_result<stream::document>::operator bool() noexcept(false) {
//   return root();
// }

really_inline stream::array::iterator simdjson_result<stream::document>::begin() & noexcept(false) {
  printf("begin\n");
  return root().begin();
}
really_inline stream::array::iterator simdjson_result<stream::document>::end() & noexcept(false) {
  // We don't call get_array because it advances the iterator, and we already did that in begin()
  // Here we assume the open array was already checked by begin()
  printf("end\n");
  auto result = root().end();
  printf("end2\n");
  return result;
}

#endif // SIMDJSON_EXCEPTIONS

} // namespace simdjson

#endif // SIMDJSON_STREAM_DOCUMENT_INL_H
