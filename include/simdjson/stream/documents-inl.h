#ifndef SIMDJSON_STREAM_DOCUMENTS_INL_H
#define SIMDJSON_STREAM_DOCUMENTS_INL_H

#include "simdjson/stream/documents.h"
#include "simdjson/stream/element.h"

namespace simdjson {
namespace stream {

//
// documents
//
really_inline documents::documents(dom::parser &parser, const uint8_t *buf) noexcept
  : json(&parser.implementation->structural_indexes[0], buf, parser.doc.string_buf.get()),
    end_json(&parser.implementation->structural_indexes[parser.implementation->n_structural_indexes], buf, parser.doc.string_buf.get()) {
}
really_inline documents::iterator documents::begin() noexcept {
  return iterator(json);
}
really_inline documents::iterator documents::end() noexcept {
  return iterator(end_json);
}

//
// documents::iterator
//
really_inline documents::iterator::iterator(internal::json_iterator &_json) noexcept
  : json{_json} {
}
really_inline element const documents::iterator::operator*() const noexcept {
  return element(json);
}
really_inline documents::iterator &documents::iterator::operator++() noexcept {
  return *this;
};
really_inline bool documents::iterator::operator!=(const documents::iterator &other) const noexcept {
  return json != other.json;
}

} // namespace stream
} // namespace simdjson

#endif // SIMDJSON_STREAM_DOCUMENTS_INL_H
