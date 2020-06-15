#ifndef SIMDJSON_STREAM_ARRAY_H
#define SIMDJSON_STREAM_ARRAY_H

#include "simdjson/common_defs.h"
#include "simdjson/error.h"
#include "simdjson/internal/json_iterator.h"

namespace simdjson {
namespace stream {

class element;

class array {
public:
  class iterator {
  public:
    really_inline simdjson_result<element> operator*() noexcept;
    really_inline iterator &operator++() noexcept;
    really_inline bool operator!=(const iterator &other) noexcept;
  private:
    really_inline iterator(internal::json_iterator &json) noexcept;

    /** The iterator. This will be updated by element, array and object iterators and get methods */
    internal::json_iterator &json;
    /**
     * true if we're at the beginning.
     *
     * This sorta sucks, but the C++ iterator interface doesn't offer any clever ways to differentiate
     * the first iteration of a loop from subsequent iterations. We are left with hoping that the
     * compiler will notice at_start gets set to false.
     */
    bool at_start{true};

    friend class array;
  }; // class iterator

  really_inline iterator begin() noexcept;
  really_inline iterator end() noexcept;

private:
  really_inline array(internal::json_iterator &json) noexcept;
  internal::json_iterator &json;
  friend class element;
  friend class simdjson_result<array>;
  friend class simdjson_result<element>;
  friend class simdjson_result<document>;
}; // class array

} // namespace stream

/** The result of a JSON navigation that may fail. */
template<>
struct simdjson_result<stream::array> : public internal::simdjson_result_base<stream::array> {
public:
  really_inline simdjson_result(stream::array &&value) noexcept; ///< @private
  really_inline simdjson_result(stream::array &&value, error_code error) noexcept; ///< @private

#if SIMDJSON_EXCEPTIONS
  really_inline stream::array::iterator begin() noexcept(false);
  really_inline stream::array::iterator end() noexcept(false);
#endif
};

} // namespace simdjson

#endif // SIMDJSON_STREAM_ARRAY_H
