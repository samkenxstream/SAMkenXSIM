#ifndef SIMDJSON_STREAM_FIELD_H
#define SIMDJSON_STREAM_FIELD_H

#include "simdjson/common_defs.h"
#include "simdjson/stream/element.h"
#include "simdjson/stream/raw_json_string.h"

namespace simdjson {
namespace stream {

class field : public element {
public:
  really_inline raw_json_string key() { return raw_key; }
private:
  really_inline field(const uint8_t * raw_key_buf, internal::json_iterator &_json) : element(_json), raw_key(raw_key_buf) {}
  raw_json_string raw_key;
  friend class object;
};

} // namespace stream
} // namespace simdjson

#endif // SIMDJSON_STREAM_FIELD_H
