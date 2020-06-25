#ifndef SIMDJSON_STREAM_OBJECT_H
#define SIMDJSON_STREAM_OBJECT_H

#include "simdjson/common_defs.h"
#include "simdjson/error.h"
#include "simdjson/internal/json_iterator.h"

namespace simdjson {
namespace stream {

class field;

class object {
public:
  class iterator {
  public:
    really_inline simdjson_result<field> operator*() noexcept;
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

    friend class object;
  }; // class iterator

  really_inline iterator begin() noexcept;
  really_inline iterator end() const noexcept;
  // Looks up an entry by key, skipping any entries that don't match, with some limitations:
  // - Only supports exact matches of JSON UTF-8 keys. If either the supplied key or the key in JSON
  //   uses escape characters, matching will not work.
  // - The key must be smaller than SIMDJSON_PADDING+4 (20 character max).
  // - Lookups are *ordered*--looking up "a" and then "b" will fail if "b" comes before "a". Doing a
  //   single lookup per object is guaranteed to work, as long as the key is there.
  really_inline simdjson_result<element> operator[](std::string_view key) noexcept;

protected:
  really_inline object(internal::json_iterator &_json) noexcept;

  internal::json_iterator &json;

  friend class element;
  friend class simdjson_result<element>;
  friend class simdjson_result<document>;
  friend class simdjson_result<stream::object>;
}; // class object

} // namespace stream

/** The result of a JSON navigation that may fail. */
template<>
struct simdjson_result<stream::object> : public internal::simdjson_result_base<stream::object> {
public:
  really_inline simdjson_result(stream::object &&value) noexcept; ///< @private
  really_inline simdjson_result(stream::object &&value, error_code error) noexcept; ///< @private

  really_inline simdjson_result<stream::element> operator[](std::string_view key) noexcept;
};

} // namespace simdjson

#endif // SIMDJSON_STREAM_DOCUMENTS_H
