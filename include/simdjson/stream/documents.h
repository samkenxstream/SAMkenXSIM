#ifndef SIMDJSON_STREAM_DOCUMENTS_H
#define SIMDJSON_STREAM_DOCUMENTS_H

#include "simdjson/common_defs.h"
#include "simdjson/error.h"
#include "simdjson/internal/json_iterator.h"

namespace simdjson {

namespace dom {
class parser;
}

namespace stream {

class documents {
public:
  //
  // Iterator interface
  //
  class iterator {
    really_inline element const operator*() const noexcept;
    // There is no space between elements, and the previous element has always already advanced us to the next. So ++ does nothing.
    really_inline iterator &operator++() noexcept;
    really_inline bool operator!=(const iterator &other) const noexcept;
  private:
    really_inline iterator(internal::json_iterator &json) noexcept;
    internal::json_iterator &json;
    friend class documents;
  }; // class iterator

  really_inline iterator begin() noexcept;
  really_inline iterator end() noexcept;

private:
  really_inline documents(dom::parser &parser, const uint8_t *buf) noexcept;
  internal::json_iterator json;
  internal::json_iterator end_json;
  friend class dom::parser;
  friend class element;
}; // class documents

} // namespace stream
} // namespace simdjson

#endif // SIMDJSON_STREAM_DOCUMENTS_H
