#ifndef SIMDJSON_STREAM_FIELD_H
#define SIMDJSON_STREAM_FIELD_H

#include "simdjson/common_defs.h"
#include "simdjson/stream/element.h"
#include "simdjson/stream/raw_json_string.h"

namespace simdjson {
namespace stream {

class field {
public:
  really_inline raw_json_string key() noexcept { return raw_key; }
  really_inline element& value() noexcept { return _value; }
private:
  really_inline field(const uint8_t * raw_key_buf, element &v) noexcept
    : raw_key(raw_key_buf+1), _value{v} {
  }

  raw_json_string raw_key;
  element &_value;

  friend class object;
};


} // namespace stream
} // namespace simdjson

#endif // SIMDJSON_STREAM_FIELD_H
