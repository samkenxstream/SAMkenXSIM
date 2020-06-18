#ifndef SIMDJSON_STREAM_DOCUMENT_H
#define SIMDJSON_STREAM_DOCUMENT_H

#include "simdjson/common_defs.h"
#include "simdjson/error.h"
#include "simdjson/internal/json_iterator.h"
#include "simdjson/stream/element.h"

namespace simdjson {

namespace dom {
class parser;
} // namespace stream

namespace stream {

// A document is basically just a singleton element
class document {
public:
  really_inline simdjson_result<array> get_array() noexcept;
  really_inline simdjson_result<object> get_object() noexcept;
  really_inline simdjson_result<raw_json_string> get_raw_json_string() noexcept;
  // really_inline simdjson_result<std::string_view> get_string() noexcept;
  // really_inline simdjson_result<double> get_double() noexcept;
  really_inline simdjson_result<uint64_t> get_uint64() noexcept;
  really_inline simdjson_result<int64_t> get_int64() noexcept;
  // really_inline simdjson_result<bool> get_bool() noexcept;

#if SIMDJSON_EXCEPTIONS
  really_inline operator array() noexcept(false);
  really_inline operator object() noexcept(false);
  really_inline operator raw_json_string() noexcept(false);
  // really_inline operator std::string_view() noexcept(false);
  // really_inline operator double() noexcept(false);
  really_inline operator uint64_t() noexcept(false);
  really_inline operator int64_t() noexcept(false);
  // really_inline operator bool() noexcept(false);

  really_inline array_iterator begin() noexcept(false);
  really_inline array_iterator end() noexcept(false);
#endif // SIMDJSON_EXCEPTIONS

protected:
  really_inline document(const dom::parser &parser, const uint8_t *buf) noexcept;

  internal::json_iterator json;
  element root;

  friend class dom::parser;
  friend class simdjson_result<document>;
}; // class element

} // namespace stream

/** The result of a JSON navigation that may fail. */
template<>
struct simdjson_result<stream::document> : public internal::simdjson_result_base<stream::document> {
public:
  really_inline simdjson_result(stream::document &&value) noexcept; ///< @private
  really_inline simdjson_result(stream::document &&value, error_code error) noexcept; ///< @private

  really_inline simdjson_result<stream::array> get_array() & noexcept;
  really_inline simdjson_result<stream::object> get_object() & noexcept;
  really_inline simdjson_result<stream::raw_json_string> get_raw_json_string() noexcept;
  // really_inline simdjson_result<std::string_view> get_string() noexcept;
  // really_inline simdjson_result<double> get_double() noexcept;
  really_inline simdjson_result<uint64_t> get_uint64() noexcept;
  really_inline simdjson_result<int64_t> get_int64() noexcept;
  // really_inline simdjson_result<bool> get_bool() noexcept;

#if SIMDJSON_EXCEPTIONS
  really_inline operator stream::array() noexcept(false);
  really_inline operator stream::object() noexcept(false);
  really_inline operator stream::raw_json_string() noexcept(false);
  // really_inline operator std::string_view() noexcept(false);
  // really_inline operator double() noexcept(false);
  really_inline operator uint64_t() noexcept(false);
  really_inline operator int64_t() noexcept(false);
  // really_inline operator bool() noexcept(false);

  really_inline stream::array_iterator begin() & noexcept(false);
  really_inline stream::array_iterator end() & noexcept(false);
#endif // SIMDJSON_EXCEPTIONS

private:
  really_inline simdjson_result<stream::element&> root() noexcept;
};

} // namespace simdjson

#endif // SIMDJSON_STREAM_DOCUMENT_H
