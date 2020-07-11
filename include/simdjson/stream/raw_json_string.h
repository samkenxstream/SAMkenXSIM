#ifndef SIMDJSON_STREAM_RAW_JSON_STRING_H
#define SIMDJSON_STREAM_RAW_JSON_STRING_H

#include "simdjson/common_defs.h"
#include "simdjson/error.h"
#include "simdjson/internal/stringparsing.h"

namespace simdjson {
namespace stream {

class element;
class field;

/**
 * A string escaped per JSON rules, terminated with quote (")
 *
 * (In other words, a pointer to the beginning of a string, just after the start quote, inside a
 * JSON file.)
 */
class raw_json_string {
public:
  really_inline const char * raw() const noexcept { return (const char *)buf; }
  really_inline WARN_UNUSED simdjson_result<std::string_view> unescape(uint8_t *&dst) const noexcept {
    uint8_t *end = internal::stringparsing::parse_string(buf, dst);
    if (!end) { return STRING_ERROR; }
    std::string_view result((const char *)dst, end-dst);
    dst = end;
    return result;
  }
private:
  const uint8_t * const buf;

  really_inline raw_json_string(const uint8_t * _buf) noexcept : buf{_buf} {}
  really_inline raw_json_string() noexcept : buf{nullptr} {} // for constructing a simdjson_result

  friend class field;
  friend class element;
  friend class simdjson_result<element>;
  friend class simdjson_result<raw_json_string>;
  friend class internal::simdjson_result_base<raw_json_string>;
};

really_inline bool operator==(const raw_json_string &a, std::string_view b) noexcept {
  return !strncmp(a.raw(), b.data(), b.size());
}

really_inline bool operator==(std::string_view a, const raw_json_string &b) noexcept {
  return b == a;
}

} // namespace simdjson
} // namespace stream

#endif // SIMDJSON_STREAM_RAW_JSON_STRING_H
