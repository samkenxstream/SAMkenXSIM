#ifndef SIMDJSON_INLINE_DOCUMENT_PARSER_H
#define SIMDJSON_INLINE_DOCUMENT_PARSER_H

// Inline implementations go in here.

#include "simdjson/document.h"
#include "simdjson/document_stream.h"
#include "simdjson/implementation.h"
#include "simdjson/padded_string.h"
#include "simdjson/internal/jsonformatutils.h"
#include <iostream>

namespace simdjson {

//
// document::parser inline implementation
//
really_inline document::parser::parser(size_t max_capacity, size_t max_depth) noexcept
  : _max_capacity{max_capacity}, _max_depth{max_depth} {
}
inline bool document::parser::is_valid() const noexcept { return valid; }
inline int document::parser::get_error_code() const noexcept { return error; }
inline std::string document::parser::get_error_message() const noexcept { return error_message(int(error)); }
inline bool document::parser::print_json(std::ostream &os) const noexcept {
  if (!is_valid()) { return false; }
  os << minify(doc);
  return true;
}
inline bool document::parser::dump_raw_tape(std::ostream &os) const noexcept {
  return is_valid() ? doc.dump_raw_tape(os) : false;
}
inline const document &document::parser::get_document() const noexcept(false) {
  if (!is_valid()) {
    throw simdjson_error(error);
  }
  return doc;
}

inline document::doc_ref_result document::parser::load(const std::string &path) noexcept {
  auto [json, _error] = padded_string::load(path);
  if (_error) { return doc_ref_result(doc, _error); }
  return parse(json);
}

inline document::stream document::parser::load_many(const std::string &path, size_t batch_size) noexcept {
  auto [json, _error] = padded_string::load(path);
  return stream(*this, reinterpret_cast<const uint8_t*>(json.data()), json.length(), batch_size, _error);
}

inline document::doc_ref_result document::parser::parse(const uint8_t *buf, size_t len, bool realloc_if_needed) noexcept {
  error_code code = ensure_capacity(len);
  if (code) { return document::doc_ref_result(doc, code); }

  if (realloc_if_needed) {
    const uint8_t *tmp_buf = buf;
    buf = (uint8_t *)internal::allocate_padded_buffer(len);
    if (buf == nullptr)
      return document::doc_ref_result(doc, MEMALLOC);
    memcpy((void *)buf, tmp_buf, len);
  }

  code = simdjson::active_implementation->parse(buf, len, *this);

  // We're indicating validity via the doc_ref_result, so set the parse state back to invalid
  valid = false;
  error = UNINITIALIZED;
  if (realloc_if_needed) {
    aligned_free((void *)buf); // must free before we exit
  }
  return document::doc_ref_result(doc, code);
}
really_inline document::doc_ref_result document::parser::parse(const char *buf, size_t len, bool realloc_if_needed) noexcept {
  return parse((const uint8_t *)buf, len, realloc_if_needed);
}
really_inline document::doc_ref_result document::parser::parse(const std::string &s) noexcept {
  return parse(s.data(), s.length(), s.capacity() - s.length() < SIMDJSON_PADDING);
}
really_inline document::doc_ref_result document::parser::parse(const padded_string &s) noexcept {
  return parse(s.data(), s.length(), false);
}

inline document::stream document::parser::parse_many(const uint8_t *buf, size_t len, size_t batch_size) noexcept {
  return stream(*this, buf, len, batch_size);
}
inline document::stream document::parser::parse_many(const char *buf, size_t len, size_t batch_size) noexcept {
  return parse_many((const uint8_t *)buf, len, batch_size);
}
inline document::stream document::parser::parse_many(const std::string &s, size_t batch_size) noexcept {
  return parse_many(s.data(), s.length(), batch_size);
}
inline document::stream document::parser::parse_many(const padded_string &s, size_t batch_size) noexcept {
  return parse_many(s.data(), s.length(), batch_size);
}

really_inline size_t document::parser::capacity() const noexcept {
  return _capacity;
}
really_inline size_t document::parser::max_capacity() const noexcept {
  return _max_capacity;
}
really_inline size_t document::parser::max_depth() const noexcept {
  return _max_depth;
}

WARN_UNUSED
inline error_code document::parser::set_capacity(size_t capacity) noexcept {
  if (_capacity == capacity) {
    return SUCCESS;
  }

  // Set capacity to 0 until we finish, in case there's an error
  _capacity = 0;

  //
  // Reallocate the document
  //
  error_code err = doc.set_capacity(capacity);
  if (err) { return err; }

  //
  // Don't allocate 0 bytes, just return.
  //
  if (capacity == 0) {
    structural_indexes.reset();
    return SUCCESS;
  }

  //
  // Initialize stage 1 output
  //
  uint32_t max_structures = ROUNDUP_N(capacity, 64) + 2 + 7;
  structural_indexes.reset( new (std::nothrow) uint32_t[max_structures]); // TODO realloc
  if (!structural_indexes) {
    return MEMALLOC;
  }

  _capacity = capacity;
  return SUCCESS;
}

really_inline void document::parser::set_max_capacity(size_t max_capacity) noexcept {
  _max_capacity = max_capacity;
}

WARN_UNUSED inline error_code document::parser::set_max_depth(size_t max_depth) noexcept {
  if (max_depth == _max_depth && ret_address) { return SUCCESS; }

  _max_depth = 0;

  if (max_depth == 0) {
    ret_address.reset();
    containing_scope_offset.reset();
    return SUCCESS;
  }

  //
  // Initialize stage 2 state
  //
  containing_scope_offset.reset(new (std::nothrow) uint32_t[max_depth]); // TODO realloc
#ifdef SIMDJSON_USE_COMPUTED_GOTO
  ret_address.reset(new (std::nothrow) void *[max_depth]);
#else
  ret_address.reset(new (std::nothrow) char[max_depth]);
#endif

  if (!ret_address || !containing_scope_offset) {
    // Could not allocate memory
    return MEMALLOC;
  }

  _max_depth = max_depth;
  return SUCCESS;
}

WARN_UNUSED inline bool document::parser::allocate_capacity(size_t capacity, size_t max_depth) noexcept {
  return !set_capacity(capacity) && !set_max_depth(max_depth);
}

inline error_code document::parser::ensure_capacity(size_t desired_capacity) noexcept {
  // If we don't have enough capacity, (try to) automatically bump it.
  if (unlikely(desired_capacity > capacity())) {
    if (desired_capacity > max_capacity()) {
      return error = CAPACITY;
    }

    error = set_capacity(desired_capacity);
    if (error) { return error; }
  }

  // Allocate depth-based buffers if they aren't already.
  error = set_max_depth(max_depth());
  if (error) { return error; }

  // If the last doc was taken, we need to allocate a new one
  if (!doc.tape) {
    error = doc.set_capacity(desired_capacity);
    if (error) { return error; }
  }

  return SUCCESS;
}

} // namespace simdjson

#endif // SIMDJSON_INLINE_DOCUMENT_PARSER_H
