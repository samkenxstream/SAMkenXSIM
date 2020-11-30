namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline value_iterator::value_iterator(json_iterator *_iter, uint32_t _depth) noexcept
  : iter{_iter},
    depth{_depth}
{
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::start_object() noexcept {
  if (*iter->advance(0) != '{') { logger::log_error(*iter, "Not an object"); return INCORRECT_TYPE; }
  return started_object();
}
simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::try_start_object() noexcept {
  if (*iter->peek() != '{') { logger::log_error(*iter, "Not an object"); return INCORRECT_TYPE; }
  iter->advance(0);
  return started_object();
}

simdjson_warn_unused simdjson_really_inline bool value_iterator::started_object() noexcept {
  if (*iter->peek() == '}') {
    logger::log_value(*iter, "empty object");
    iter->advance(-1);
    return false;
  }
  iter->depth++;
  logger::log_start_value(*iter, "object");
  return true;
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::has_next_field() noexcept {
  switch (*iter->advance(0)) {
    case '}':
      logger::log_end_value(*iter, "object");
      _depth--;
      return false;
    case ',':
      _depth++;
      return true;
    default:
      return report_error(TAPE_ERROR, "Missing comma between object fields");
  }
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::find_field_raw(const char *key) noexcept {
  bool has_next;
  do {
    raw_json_string actual_key;
    SIMDJSON_TRY( require_raw_json_string().get(actual_key) );
    if (*iter->advance(0) != ':') { return report_error(TAPE_ERROR, "Missing colon in object field"); }
    if (actual_key == key) {
      logger::log_event(*iter, "match", key);
      return true;
    }
    logger::log_event(*iter, "non-match", key);
    SIMDJSON_TRY( skip() ); // Skip the value so we can look at the next key

    SIMDJSON_TRY( has_next_field().get(has_next) );
  } while (has_next);
  logger::log_event(*iter, "no matches", key);
  return false;
}

simdjson_warn_unused simdjson_really_inline simdjson_result<raw_json_string> value_iterator::field_key() noexcept {
  const uint8_t *key = iter->advance(0);
  if (*(key++) != '"') { return report_error(TAPE_ERROR, "Object key is not a string"); }
  return raw_json_string(key);
}

simdjson_warn_unused simdjson_really_inline error_code value_iterator::field_value() noexcept {
  if (*iter->advance(0) != ':') { return report_error(TAPE_ERROR, "Missing colon in object field"); }
  return SUCCESS;
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::start_array() noexcept {
  if (*iter->advance(0) != '[') { logger::log_error(*iter, "Not an array"); return INCORRECT_TYPE; }
  return started_array();
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::try_start_array() noexcept {
  if (*iter->peek() != '[') { logger::log_error(*iter, "Not an array"); return INCORRECT_TYPE; }
  iter->advance(0);
  return started_array();
}

simdjson_warn_unused simdjson_really_inline bool value_iterator::started_array() noexcept {
  if (*iter->peek() == ']') {
    logger::log_value(*iter, "empty array");
    iter->advance(-1);
    return false;
  }
  logger::log_start_value(*iter, "array");
  _depth++;
  return true;
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::has_next_element() noexcept {
  switch (*iter->advance(0)) {
    case ']':
      logger::log_end_value(*iter, "array");
      _depth--;
      return false;
    case ',':
      _depth++;
      return true;
    default:
      return report_error(TAPE_ERROR, "Missing comma between array elements");
  }
}

simdjson_warn_unused simdjson_really_inline simdjson_result<std::string_view> value_iterator::try_get_string() noexcept {
  return try_get_raw_json_string().unescape(current_string_buf_loc);
}
simdjson_warn_unused simdjson_really_inline simdjson_result<std::string_view> value_iterator::require_string() noexcept {
  return require_raw_json_string().unescape(current_string_buf_loc);
}
simdjson_warn_unused simdjson_really_inline simdjson_result<raw_json_string> value_iterator::try_get_raw_json_string() noexcept {
  logger::log_value(*iter, "string", "");
  auto json = iter->peek();
  if (*json != '"') { logger::log_error(*iter, "Not a string"); return INCORRECT_TYPE; }
  iter->advance(-1);
  return raw_json_string(json+1);
}
simdjson_warn_unused simdjson_really_inline simdjson_result<raw_json_string> value_iterator::require_raw_json_string() noexcept {
  logger::log_value(*iter, "string", "");
  auto json = iter->advance(-1);
  if (*json != '"') { logger::log_error(*iter, "Not a string"); return INCORRECT_TYPE; }
  return raw_json_string(json+1);
}
simdjson_warn_unused simdjson_really_inline simdjson_result<uint64_t> value_iterator::try_get_uint64() noexcept {
  logger::log_value(*iter, "uint64", "");
  uint64_t result;
  SIMDJSON_TRY( numberparsing::parse_unsigned(iter->peek()).get(result) );
  iter->advance(-1);
  return result;
}
simdjson_warn_unused simdjson_really_inline simdjson_result<uint64_t> value_iterator::require_uint64() noexcept {
  logger::log_value(*iter, "uint64", "");
  return numberparsing::parse_unsigned(iter->advance(-1));
}
simdjson_warn_unused simdjson_really_inline simdjson_result<int64_t> value_iterator::try_get_int64() noexcept {
  logger::log_value(*iter, "int64", "");
  int64_t result;
  SIMDJSON_TRY( numberparsing::parse_integer(iter->peek()).get(result) );
  iter->advance(-1);
  return result;
}
simdjson_warn_unused simdjson_really_inline simdjson_result<int64_t> value_iterator::require_int64() noexcept {
  logger::log_value(*iter, "int64", "");
  return numberparsing::parse_integer(iter->advance(-1));
}
simdjson_warn_unused simdjson_really_inline simdjson_result<double> value_iterator::try_get_double() noexcept {
  logger::log_value(*iter, "double", "");
  double result;
  SIMDJSON_TRY( numberparsing::parse_double(iter->peek()).get(result) );
  iter->advance(-1);
  return result;
}
simdjson_warn_unused simdjson_really_inline simdjson_result<double> value_iterator::require_double() noexcept {
  logger::log_value(*iter, "double", "");
  return numberparsing::parse_double(iter->advance(-1));
}
simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::parse_bool(const uint8_t *json) noexcept {
  logger::log_value(*iter, "bool", "");
  auto not_true = atomparsing::str4ncmp(json, "true");
  auto not_false = atomparsing::str4ncmp(json, "fals") | (json[4] ^ 'e');
  bool error = (not_true && not_false) || jsoncharutils::is_not_structural_or_whitespace(json[not_true ? 5 : 4]);
  if (error) { logger::log_error(*iter, "Not a boolean"); return INCORRECT_TYPE; }
  return simdjson_result<bool>(!not_true);
}
simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::try_get_bool() noexcept {
  bool result;
  SIMDJSON_TRY( parse_bool(iter->peek()).get(result) );
  iter->advance(-1);
  return result;
}
simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::require_bool() noexcept {
  return parse_bool(iter->advance(-1));
}
simdjson_really_inline bool value_iterator::is_null(const uint8_t *json) noexcept {
  if (!atomparsing::str4ncmp(json, "null")) {
    logger::log_value(*iter, "null", "");
    return true;
  }
  return false;
}
simdjson_really_inline bool value_iterator::is_null() noexcept {
  if (is_null(iter->peek())) {
    iter->advance(-1);
    return true;
  }
  return false;
}
simdjson_really_inline bool value_iterator::require_null() noexcept {
  return is_null(iter->advance(-1));
}

template<int N>
simdjson_warn_unused simdjson_really_inline bool value_iterator::copy_to_buffer(const uint8_t *json, uint8_t (&tmpbuf)[N]) noexcept {
  // Truncate whitespace to fit the buffer.
  auto len = peek_length(-1);
  if (len > N-1) {
    if (jsoncharutils::is_not_structural_or_whitespace(json[N])) { return false; }
    len = N-1;
  }

  // Copy to the buffer.
  std::memcpy(tmpbuf, json, len);
  tmpbuf[len] = ' ';
  return true;
}

constexpr const uint32_t MAX_INT_LENGTH = 1024;

simdjson_warn_unused simdjson_really_inline simdjson_result<uint64_t> value_iterator::parse_root_uint64(const uint8_t *json) noexcept {
  uint8_t tmpbuf[20+1]; // <20 digits> is the longest possible unsigned integer
  if (!copy_to_buffer(json, tmpbuf)) { logger::log_error(*iter, "Root number more than 20 characters"); return NUMBER_ERROR; }
  logger::log_value(*iter, "uint64", "");
  auto result = numberparsing::parse_unsigned(tmpbuf);
  if (result.error()) { logger::log_error(*iter, "Error parsing unsigned integer"); return result.error(); }
  return result;
}
simdjson_warn_unused simdjson_really_inline simdjson_result<uint64_t> value_iterator::try_get_root_uint64() noexcept {
  uint64_t result;
  SIMDJSON_TRY( parse_root_uint64(iter->peek()).get(result) );
  iter->advance(-1);
  return result;
}
simdjson_warn_unused simdjson_really_inline simdjson_result<uint64_t> value_iterator::require_root_uint64() noexcept {
  return parse_root_uint64(iter->advance(-1));
}
simdjson_warn_unused simdjson_really_inline simdjson_result<int64_t> value_iterator::parse_root_int64(const uint8_t *json) noexcept {
  uint8_t tmpbuf[20+1]; // -<19 digits> is the longest possible integer
  if (!copy_to_buffer(json, tmpbuf)) { logger::log_error(*iter, "Root number more than 20 characters"); return NUMBER_ERROR; }
  logger::log_value(*iter, "int64", "");
  auto result = numberparsing::parse_integer(tmpbuf);
  if (result.error()) { report_error(result.error(), "Error parsing integer"); }
  return result;
}
simdjson_warn_unused simdjson_really_inline simdjson_result<int64_t> value_iterator::try_get_root_int64() noexcept {
  int64_t result;
  SIMDJSON_TRY( parse_root_int64(iter->peek()).get(result) );
  iter->advance(-1);
  return result;
}
simdjson_warn_unused simdjson_really_inline simdjson_result<int64_t> value_iterator::require_root_int64() noexcept {
  return parse_root_int64(iter->advance(-1));
}
simdjson_warn_unused simdjson_really_inline simdjson_result<double> value_iterator::parse_root_double(const uint8_t *json) noexcept {
  // Per https://www.exploringbinary.com/maximum-number-of-decimal-digits-in-binary-floating-point-numbers/, 1074 is the maximum number of significant fractional digits. Add 8 more digits for the biggest number: -0.<fraction>e-308.
  uint8_t tmpbuf[1074+8+1];
  if (!copy_to_buffer(json, tmpbuf)) { logger::log_error(*iter, "Root number more than 1082 characters"); return NUMBER_ERROR; }
  logger::log_value(*iter, "double", "");
  auto result = numberparsing::parse_double(tmpbuf);
  if (result.error()) { report_error(result.error(), "Error parsing double"); }
  return result;
}
simdjson_warn_unused simdjson_really_inline simdjson_result<double> value_iterator::try_get_root_double() noexcept {
  double result;
  SIMDJSON_TRY( parse_root_double(iter->peek()).get(result) );
  iter->advance(-1);
  return result;
}
simdjson_warn_unused simdjson_really_inline simdjson_result<double> value_iterator::require_root_double() noexcept {
  return parse_root_double(iter->advance(-1));
}
simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::parse_root_bool(const uint8_t *json) noexcept {
  uint8_t tmpbuf[5+1];
  if (!copy_to_buffer(json, tmpbuf)) { logger::log_error(*iter, "Not a boolean"); return INCORRECT_TYPE; }
  return parse_bool(tmpbuf);
}
simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::try_get_root_bool() noexcept {
  bool result;
  SIMDJSON_TRY( parse_root_bool(iter->peek()).get(result) );
  iter->advance(-1);
  return result;
}
simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::require_root_bool() noexcept {
  return parse_root_bool(iter->advance(-1));
}
simdjson_really_inline bool value_iterator::is_root_null(const uint8_t *json) noexcept {
  uint8_t tmpbuf[4+1];
  if (!copy_to_buffer(json, tmpbuf)) { return false; }
  return is_null(tmpbuf);
}
simdjson_really_inline bool value_iterator::is_root_null() noexcept {
  if (!is_root_null(iter->peek())) { return false; }
  iter->advance(-1);
  return true;
}
simdjson_really_inline bool value_iterator::require_root_null() noexcept {
  return is_root_null(iter->advance(-1));
}

simdjson_warn_unused simdjson_really_inline error_code value_iterator::skip() noexcept {
  return skip(_depth);
}

simdjson_warn_unused simdjson_really_inline error_code value_iterator::skip(depth_t value_depth) noexcept {
  if (_depth < value_depth) { return SUCCESS; }
  // The loop breaks only when _depth-- happens.
  auto end = &parser->dom_parser.structural_indexes[parser->dom_parser.n_structural_indexes];
  while (index <= end) {
    uint8_t ch = *iter->advance();
    switch (ch) {
      // TODO consider whether matching braces is a requirement: if non-matching braces indicates
      // *missing* braces, then future lookups are not in the object/arrays they think they are,
      // violating the rule "validate enough structure that the user can be confident they are
      // looking at the right values."
      case ']': case '}':
        logger::log_end_value(*iter, "skip");
        _depth--;
        if (_depth < value_depth) { return SUCCESS; }
        break;
      // PERF TODO does it skip the _depth check when we don't decrement _depth?
      case '[': case '{':
        logger::log_start_value(*iter, "skip");
        _depth++;
        break;
      default:
        logger::log_value(*iter, "skip", "");
        break;
    }
  }

  return report_error(TAPE_ERROR, "not enough close braces");
}

simdjson_really_inline bool value_iterator::at_start() const noexcept {
  return index == parser->dom_parser.structural_indexes.get();
}

simdjson_really_inline bool value_iterator::at_eof() const noexcept {
  return index == &parser->dom_parser.structural_indexes[parser->dom_parser.n_structural_indexes];
}

simdjson_really_inline bool value_iterator::is_alive() const noexcept {
  return parser;
}

simdjson_really_inline const uint8_t *value_iterator::iter->advance(int32_t delta_depth) noexcept {
  _depth += delta_depth;
  return iter->advance();
}

simdjson_really_inline error_code value_iterator::report_error(error_code error, const char *message) noexcept {
  SIMDJSON_ASSUME(error != SUCCESS && error != UNINITIALIZED && error != INCORRECT_TYPE && error != NO_SUCH_FIELD);
  logger::log_error(*iter, message);
  _error = error;
  return error;
}
simdjson_really_inline error_code value_iterator::error() const noexcept {
  return _error;
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value_iterator>::simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::value_iterator &&value) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::value_iterator>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value_iterator>(value)) {}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value_iterator>::simdjson_result(error_code error) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::value_iterator>(error) {}

} // namespace simdjson