#ifndef SIMDJSON_STREAM_ELEMENT_H
#define SIMDJSON_STREAM_ELEMENT_H

#include "simdjson/common_defs.h"
#include "simdjson/error.h"
#include "simdjson/internal/json_iterator.h"
#include "simdjson/stream/raw_json_string.h"

namespace simdjson {
namespace stream {

class array;
class object;

class element {
public:
  really_inline simdjson_result<array> get_array() noexcept;
  really_inline simdjson_result<object> get_object() noexcept;
  really_inline simdjson_result<raw_json_string> get_raw_json_string() noexcept;
  // really_inline simdjson_result<std::string_view> get_string() noexcept;
  // really_inline simdjson_result<double> get_double() noexcept;
  really_inline simdjson_result<uint64_t> get_uint64() noexcept;
  really_inline simdjson_result<int64_t> get_int64() noexcept;
  // really_inline simdjson_result<bool> get_bool() noexcept;

#ifdef SIMDJSON_EXCEPTIONS
  really_inline operator array() noexcept(false);
  really_inline operator object() noexcept(false);
  really_inline operator raw_json_string() noexcept(false);
  // really_inline operator std::string_view() noexcept(false);
  // really_inline operator double() noexcept(false);
  really_inline operator uint64_t() noexcept(false);
  really_inline operator int64_t() noexcept(false);
  // really_inline operator bool() noexcept(false);
#endif // SIMDJSON_EXCEPTIONS

  // TODO users should never have to call this for things to work. Figure out how to make it happen
  // in destructors or some other automatic mechanism.
  WARN_UNUSED inline error_code skip() noexcept;

protected:
  really_inline element(internal::json_iterator &_json) noexcept;
  internal::json_iterator &json;
  friend class document;
  friend class documents;
  friend class array;
  friend class object;
  friend class simdjson_result<element>;
  friend class simdjson_result<object>;
}; // class element

} // namespace stream

/** The result of a JSON navigation that may fail. */
template<>
struct simdjson_result<stream::element> : public internal::simdjson_result_base<stream::element> {
public:
  really_inline simdjson_result(stream::element &&value) noexcept; ///< @private
  really_inline simdjson_result(stream::element &&value, error_code error) noexcept; ///< @private

  really_inline simdjson_result<stream::array> get_array() noexcept;
  really_inline simdjson_result<stream::object> get_object() noexcept;
  really_inline simdjson_result<stream::raw_json_string> get_raw_json_string() noexcept;
  // really_inline simdjson_result<std::string_view> get_string() noexcept;
  // really_inline simdjson_result<double> get_double() noexcept;
  really_inline simdjson_result<uint64_t> get_uint64() noexcept;
  really_inline simdjson_result<int64_t> get_int64() noexcept;
  // really_inline simdjson_result<bool> get_bool() noexcept;

  WARN_UNUSED inline error_code skip() noexcept;

#ifdef SIMDJSON_EXCEPTIONS
  really_inline operator stream::array() noexcept(false);
  really_inline operator stream::object() noexcept(false);
  really_inline operator stream::raw_json_string() noexcept(false);
  // really_inline operator std::string_view() noexcept(false);
  // really_inline operator double() noexcept(false);
  really_inline operator uint64_t() noexcept(false);
  really_inline operator int64_t() noexcept(false);
  // really_inline operator bool() noexcept(false);
#endif // SIMDJSON_EXCEPTIONS
};

} // namespace simdjson

#endif // SIMDJSON_STREAM_ELEMENT_H
