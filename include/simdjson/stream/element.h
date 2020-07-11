#ifndef SIMDJSON_STREAM_ELEMENT_H
#define SIMDJSON_STREAM_ELEMENT_H

#include "simdjson/common_defs.h"
#include "simdjson/error.h"
#include "simdjson/internal/json_iterator.h"
#include "simdjson/stream/raw_json_string.h"

namespace simdjson {
namespace stream {

class array;
class array_iterator;
class object;

class element {
public:
  // No copying; you only get references to this!
  element(const element &) = delete;
  bool operator=(const element &) = delete;

  really_inline simdjson_result<array> get_array() noexcept;
  really_inline simdjson_result<object> get_object() noexcept;
  really_inline simdjson_result<raw_json_string> get_raw_json_string() noexcept;
  really_inline simdjson_result<std::string_view> get_string() noexcept;
  really_inline simdjson_result<double> get_double() noexcept;
  really_inline simdjson_result<uint64_t> get_uint64() noexcept;
  really_inline simdjson_result<int64_t> get_int64() noexcept;
  // really_inline simdjson_result<bool> get_bool() noexcept;

#if SIMDJSON_EXCEPTIONS
  really_inline operator array() noexcept(false);
  really_inline operator object() noexcept(false);
  really_inline operator raw_json_string() noexcept(false);
  really_inline operator std::string_view() noexcept(false);
  really_inline operator double() noexcept(false);
  really_inline operator uint64_t() noexcept(false);
  really_inline operator int64_t() noexcept(false);
  // really_inline operator bool() noexcept(false);

  really_inline array_iterator begin() noexcept(false);
  really_inline array_iterator end() noexcept(false);
#endif // SIMDJSON_EXCEPTIONS

protected:
  really_inline element(element && other) noexcept;
  really_inline element(internal::json_iterator &_json, bool consumed = false) noexcept;

  WARN_UNUSED really_inline bool finish(int parent_depth) noexcept;

  internal::json_iterator &json;
  bool consumed;

  friend class document;
  friend class documents;
  friend class array;
  friend class array_iterator;
  friend class object;
  friend class simdjson_result<document>;
  friend class simdjson_result<array>;
  friend class simdjson_result<element&>;
  friend class simdjson_result<object>;
}; // class element

} // namespace stream

/** The result of a JSON navigation that may fail. */
template<>
struct simdjson_result<stream::element&> : public internal::simdjson_result_base<stream::element&> {
public:
  really_inline simdjson_result(stream::element &value) noexcept; ///< @private
  really_inline simdjson_result(stream::element &value, error_code error) noexcept; ///< @private

  really_inline simdjson_result<stream::array> get_array() noexcept;
  really_inline simdjson_result<stream::object> get_object() noexcept;
  really_inline simdjson_result<stream::raw_json_string> get_raw_json_string() noexcept;
  really_inline simdjson_result<std::string_view> get_string() noexcept;
  really_inline simdjson_result<double> get_double() noexcept;
  really_inline simdjson_result<uint64_t> get_uint64() noexcept;
  really_inline simdjson_result<int64_t> get_int64() noexcept;
  // really_inline simdjson_result<bool> get_bool() noexcept;

#if SIMDJSON_EXCEPTIONS
  really_inline operator stream::array() noexcept(false);
  really_inline operator stream::object() noexcept(false);
  really_inline operator stream::raw_json_string() noexcept(false);
  really_inline operator std::string_view() noexcept(false);
  really_inline operator double() noexcept(false);
  really_inline operator uint64_t() noexcept(false);
  really_inline operator int64_t() noexcept(false);
  // really_inline operator bool() noexcept(false);

  really_inline stream::array_iterator begin() noexcept(false);
  really_inline stream::array_iterator end() noexcept(false);
#endif // SIMDJSON_EXCEPTIONS
};

} // namespace simdjson

#endif // SIMDJSON_STREAM_ELEMENT_H
