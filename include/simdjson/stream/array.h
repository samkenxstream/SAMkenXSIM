#ifndef SIMDJSON_STREAM_ARRAY_H
#define SIMDJSON_STREAM_ARRAY_H

#include "simdjson/common_defs.h"
#include "simdjson/error.h"
#include "simdjson/internal/json_iterator.h"
#include "simdjson/stream/element.h"

namespace simdjson {
namespace stream {

class array {
public:
  really_inline array_iterator begin() noexcept;
  really_inline array_iterator end() noexcept;

private:
  really_inline array(internal::json_iterator &json) noexcept;

  internal::json_iterator &json;

  friend class array_iterator;
  friend class element;
  friend class document;
  friend class simdjson_result<array>;
  friend class simdjson_result<element&>;
  friend class simdjson_result<document>;
}; // class array

class array_iterator {
public:
  really_inline simdjson_result<element&> operator*() noexcept;
  really_inline array_iterator &operator++() noexcept;
  really_inline bool operator!=(const array_iterator &other) noexcept;
private:
  really_inline array_iterator(internal::json_iterator &json, bool at_start) noexcept;

  element value;
  int depth;

  /**
    * true if we're at the beginning.
    *
    * This sorta sucks, but the C++ array_iterator interface doesn't offer any clever ways to differentiate
    * the first iteration of a loop from subsequent iterations. We are left with hoping that the
    * compiler will notice at_start gets set to false.
    */
  bool at_start{true};

  friend class array;
}; // class array_iterator

} // namespace stream

/** The result of a JSON navigation that may fail. */
template<>
struct simdjson_result<stream::array> : public internal::simdjson_result_base<stream::array> {
public:
  really_inline simdjson_result(stream::array &&value) noexcept; ///< @private
  really_inline simdjson_result(stream::array &&value, error_code error) noexcept; ///< @private

#if SIMDJSON_EXCEPTIONS
  really_inline stream::array_iterator begin() noexcept(false);
  really_inline stream::array_iterator end() noexcept(false);
#endif
};

} // namespace simdjson

#endif // SIMDJSON_STREAM_ARRAY_H
