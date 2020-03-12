#ifndef SIMDJSON_INLINE_DOCUMENT_FORMATTERS_H
#define SIMDJSON_INLINE_DOCUMENT_FORMATTERS_H

// Inline implementations go in here.

#include "simdjson/document_formatters.h"
#include "simdjson/document.h"
#include "simdjson/padded_string.h"
#include "simdjson/internal/jsonformatutils.h"
#include <iostream>

namespace simdjson {

//
// minify inline implementation
//

template<typename T>
inline minify<T>::minify(const T &_value) noexcept : value{_value} {}
template<typename T>
inline minify<T>::operator std::string() const noexcept { std::stringstream s; s << *this; return s.str(); }

template<>
inline std::ostream& minify<document>::print(std::ostream& out) {
  return out << minify<document::element>(value.root());
}
template<>
inline std::ostream& minify<document::element>::print(std::ostream& out) {
  using tape_type=document::tape_type;

  size_t depth = 0;
  constexpr size_t MAX_DEPTH = 16;
  bool is_object[MAX_DEPTH];
  is_object[0] = false;
  bool after_value = false;

  document::tape_ref iter(value.doc, value.json_index);
  do {
    // print commas after each value
    if (after_value) {
      out << ",";
    }
    // If we are in an object, print the next key and :, and skip to the next value.
    if (is_object[depth]) {
      out << '"' << internal::escape_json_string(iter.get_string_view()) << "\":";
      iter.json_index++;
    }
    switch (iter.type()) {

    // Arrays
    case tape_type::START_ARRAY: {
      // If we're too deep, we need to recurse to go deeper.
      depth++;
      if (unlikely(depth >= MAX_DEPTH)) {
        out << minify<document::array>(document::array(iter.doc, iter.json_index));
        iter.json_index = iter.tape_value() - 1; // Jump to the ]
        depth--;
        break;
      }

      // Output start [
      out << '[';
      iter.json_index++;

      // Handle empty [] (we don't want to come back around and print commas)
      if (iter.type() == tape_type::END_ARRAY) {
        out << ']';
        depth--;
        break;
      }

      is_object[depth] = false;
      after_value = false;
      continue;
    }

    // Objects
    case tape_type::START_OBJECT: {
      // If we're too deep, we need to recurse to go deeper.
      depth++;
      if (unlikely(depth >= MAX_DEPTH)) {
        out << minify<document::object>(document::object(iter.doc, iter.json_index));
        iter.json_index = iter.tape_value() - 1; // Jump to the }
        depth--;
        break;
      }

      // Output start {
      out << '{';
      iter.json_index++;

      // Handle empty {} (we don't want to come back around and print commas)
      if (iter.type() == tape_type::END_OBJECT) {
        out << '}';
        depth--;
        break;
      }

      is_object[depth] = true;
      after_value = false;
      continue;
    }

    // Scalars
    case tape_type::STRING:
      out << '"' << internal::escape_json_string(iter.get_string_view()) << '"';
      break;
    case tape_type::INT64:
      out << iter.next_tape_value<int64_t>();
      iter.json_index++; // numbers take up 2 spots, so we need to increment extra
      break;
    case tape_type::UINT64:
      out << iter.next_tape_value<uint64_t>();
      iter.json_index++; // numbers take up 2 spots, so we need to increment extra
      break;
    case tape_type::DOUBLE:
      out << iter.next_tape_value<double>();
      iter.json_index++; // numbers take up 2 spots, so we need to increment extra
      break;
    case tape_type::TRUE_VALUE:
      out << "true";
      break;
    case tape_type::FALSE_VALUE:
      out << "false";
      break;
    case tape_type::NULL_VALUE:
      out << "null";
      break;

    // These are impossible
    case tape_type::END_ARRAY:
    case tape_type::END_OBJECT:
    case tape_type::ROOT:
      abort();
    }
    iter.json_index++;
    after_value = true;

    // Handle multiple ends in a row
    while (depth != 0 && (iter.type() == tape_type::END_ARRAY || iter.type() == tape_type::END_OBJECT)) {
      out << char(iter.type());
      depth--;
      iter.json_index++;
    }

    // Stop when we're at depth 0
  } while (depth != 0);

  return out;
}
template<>
inline std::ostream& minify<document::object>::print(std::ostream& out) {
  out << '{';
  auto pair = value.begin();
  auto end = value.end();
  if (pair != end) {
    out << minify<document::key_value_pair>(*pair);
    for (++pair; pair != end; ++pair) {
      out << "," << minify<document::key_value_pair>(*pair);
    }
  }
  return out << '}';
}
template<>
inline std::ostream& minify<document::array>::print(std::ostream& out) {
  out << '[';
  auto element = value.begin();
  auto end = value.end();
  if (element != end) {
    out << minify<document::element>(*element);
    for (++element; element != end; ++element) {
      out << "," << minify<document::element>(*element);
    }
  }
  return out << ']';
}
template<>
inline std::ostream& minify<document::key_value_pair>::print(std::ostream& out) {
  return out << '"' << internal::escape_json_string(value.key) << "\":" << value.value;
}

template<>
inline std::ostream& minify<document::doc_result>::print(std::ostream& out) {
  if (value.error) { throw simdjson_error(value.error); }
  return out << minify<document>(value.doc);
}
template<>
inline std::ostream& minify<document::doc_ref_result>::print(std::ostream& out) {
  if (value.error) { throw simdjson_error(value.error); }
  return out << minify<document>(value.doc);
}
template<>
inline std::ostream& minify<document::element_result<document::element>>::print(std::ostream& out) {
  if (value.error) { throw simdjson_error(value.error); }
  return out << minify<document::element>(value.value);
}
template<>
inline std::ostream& minify<document::element_result<document::array>>::print(std::ostream& out) {
  if (value.error) { throw simdjson_error(value.error); }
  return out << minify<document::array>(value.value);
}
template<>
inline std::ostream& minify<document::element_result<document::object>>::print(std::ostream& out) {
  if (value.error) { throw simdjson_error(value.error); }
  return out << minify<document::object>(value.value);
}

template<typename T>
inline std::ostream& operator<<(std::ostream& out, minify<T> formatter) { return formatter.print(out); }
inline std::ostream& operator<<(std::ostream& out, const document &value) { return out << minify(value); }
inline std::ostream& operator<<(std::ostream& out, const document::element &value) { return out << minify(value); };
inline std::ostream& operator<<(std::ostream& out, const document::array &value) { return out << minify(value); }
inline std::ostream& operator<<(std::ostream& out, const document::object &value) { return out << minify(value); }
inline std::ostream& operator<<(std::ostream& out, const document::key_value_pair &value) { return out << minify(value); }
inline std::ostream& operator<<(std::ostream& out, const document::doc_result &value) noexcept(false) { return out << minify(value); }
inline std::ostream& operator<<(std::ostream& out, const document::doc_ref_result &value) noexcept(false) { return out << minify(value); }
inline std::ostream& operator<<(std::ostream& out, const document::element_result<document::element> &value) noexcept(false) { return out << minify(value); }
inline std::ostream& operator<<(std::ostream& out, const document::element_result<document::array> &value) noexcept(false) { return out << minify(value); }
inline std::ostream& operator<<(std::ostream& out, const document::element_result<document::object> &value) noexcept(false) { return out << minify(value); }

} // namespace simdjson

#endif // SIMDJSON_INLINE_DOCUMENT_FORMATTERS_H
