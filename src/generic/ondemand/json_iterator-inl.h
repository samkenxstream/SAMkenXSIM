namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline json_iterator::json_iterator() noexcept = default;
simdjson_really_inline json_iterator::json_iterator(json_iterator &&other) noexcept = default;
simdjson_really_inline json_iterator &json_iterator::operator=(json_iterator &&other) noexcept = default;
simdjson_really_inline json_iterator::json_iterator(const uint8_t *_buf, uint32_t *_index, uint32_t _depth) noexcept
  : token_iterator(_buf, _index), depth{_depth}
{
}


SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::start_object(json_iterator::container &c) noexcept {
  SIMDJSON_TRY(start_object());
  c = depth;
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::start_object() noexcept {
  if (*advance() != '{') { logger::log_error(*this, "Not an object"); return INCORRECT_TYPE; }
  started_object();
  return SUCCESS;
}

simdjson_really_inline json_iterator::container json_iterator::started_object() noexcept {
  depth++;
  return depth;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline bool json_iterator::is_empty_object() noexcept {
  if (*peek() == '}') {
    advance();
    depth--;
    logger::log_value(*this, "empty object", "", -2);
    return true;
  }
  logger::log_start_value(*this, "object", -1, -1); 
  return false;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::has_next_field(bool &result) noexcept {
  switch (*advance()) {
    case '}':
      depth--;
      logger::log_end_value(*this, "object");
      result = false;
      return SUCCESS;
    case ',':
      result = true;
      return SUCCESS;
    default:
      logger::log_error(*this, "Missing comma between object fields");
      return TAPE_ERROR;
  }
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::next_field(container c, bool &result) noexcept {
  skip_unfinished_children(c);
  return has_next_field(result);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::find_field_raw(const char *key, bool &has_field) noexcept {
  do {
    raw_json_string actual_key;
    SIMDJSON_TRY( get_raw_json_string(actual_key) );
    if (*advance() != ':') { logger::log_error(*this, "Missing colon in object field"); return TAPE_ERROR; }
    if (actual_key == key) {
      logger::log_event(*this, "match", key);
      has_field = true;
      return SUCCESS;
    }
    logger::log_event(*this, "non-match", key);
    skip(); // Skip the value so we can look at the next key

    SIMDJSON_TRY( has_next_field(has_field) );
  } while (has_field);
  logger::log_event(*this, "no matches", key);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::find_first_field_raw(const char *key, bool &has_field) noexcept {
  SIMDJSON_TRY( start_object() );
  if (is_empty_object()) { has_field = false; return SUCCESS; }
  return find_field_raw(key, has_field);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::find_next_field_raw(const char *key, bool &has_field) noexcept {
  SIMDJSON_TRY( has_next_field(has_field) );
  return find_field_raw(key, has_field);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::field_key(raw_json_string &has_field) noexcept {
  const uint8_t *key = advance();
  if (*(key++) != '"') { logger::log_error(*this, "Object key is not a string"); return TAPE_ERROR; }
  has_field = key;
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::field_value() noexcept {
  if (*advance() != ':') { logger::log_error(*this, "Missing colon in object field"); return TAPE_ERROR; }
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::finish_object() noexcept {
  skip_unfinished_children(depth-1);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::start_array(json_iterator::container &c) noexcept {
  SIMDJSON_TRY( start_array() );
  c = depth;
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::start_array() noexcept {
  if (*advance() != '[') { logger::log_error(*this, "Not an array"); return INCORRECT_TYPE; }
  started_array();
  return SUCCESS;
}

simdjson_really_inline json_iterator::container json_iterator::started_array() noexcept {
  depth++;
  return depth;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline bool json_iterator::is_empty_array() noexcept {
  if (*peek() == ']') {
    advance();
    depth--;
    logger::log_value(*this, "empty array", "", -2);
    return true;
  }
  logger::log_start_value(*this, "array", -1, -1); 
  return false;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::has_next_element(bool &result) noexcept {
  switch (*advance()) {
    case ']':
      depth--;
      logger::log_end_value(*this, "array");
      result = false;
      return SUCCESS;
    case ',':
      result = true;
      return SUCCESS;
    default:
      logger::log_error(*this, "Missing comma between array elements");
      return TAPE_ERROR;
  }
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::next_element(container c, bool &result) noexcept {
  skip_unfinished_children(c);
  return has_next_element(result);
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::finish_array() noexcept {
  skip_unfinished_children(depth-1);
  return SUCCESS;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::get_raw_json_string(raw_json_string &s) noexcept {
  logger::log_value(*this, "string", "", 0);
  auto json = advance();
  if (*(json++) != '"') { return INCORRECT_TYPE; }
  s = json;
  return SUCCESS;
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::get_uint64(uint64_t &i) noexcept {
  logger::log_value(*this, "uint64", "", 0);
  return stage2::numberparsing::parse_unsigned(advance(), i);
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::get_int64(int64_t &i) noexcept {
  logger::log_value(*this, "int64", "", 0);
  return stage2::numberparsing::parse_integer(advance(), i);
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::get_double(double &d) noexcept {
  logger::log_value(*this, "double", "", 0);
  return stage2::numberparsing::parse_double(advance(), d);
}
SIMDJSON_WARN_UNUSED simdjson_really_inline error_code json_iterator::get_bool(bool &b) noexcept {
  logger::log_value(*this, "bool", "", 0);
  auto json = advance();
  auto not_false = stage2::atomparsing::str4ncmp(json, "fals") | (json[4] ^ 'e');
  auto not_true = stage2::atomparsing::str4ncmp(json, "true");
  bool error = (not_true && not_false) || stage2::is_not_structural_or_whitespace(json[not_true ? 5 : 4]);
  if (error) { logger::log_error(*this, "not a boolean"); return INCORRECT_TYPE; }
  b = not_false;
  return SUCCESS;
}
simdjson_really_inline bool json_iterator::is_null() noexcept {
  auto json = peek();
  if (!stage2::atomparsing::str4ncmp(json, "null")) {
    logger::log_value(*this, "null", "", 0);
    advance();
    return true;
  }
  return false;
}

SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<raw_json_string> json_iterator::get_raw_json_string() noexcept {
  raw_json_string result;
  error_code error = get_raw_json_string(result);
  if (error) { return error; }
  return result;
}
SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<uint64_t> json_iterator::get_uint64() noexcept {
  uint64_t result;
  error_code error = get_uint64(result);
  if (error) { return error; }
  return result;
}
SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<int64_t> json_iterator::get_int64() noexcept {
  int64_t result;
  error_code error = get_int64(result);
  if (error) { return error; }
  return result;
}
SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<double> json_iterator::get_double() noexcept {
  double result;
  error_code error = get_double(result);
  if (error) { return error; }
  return result;
}
SIMDJSON_WARN_UNUSED simdjson_really_inline simdjson_result<bool> json_iterator::get_bool() noexcept {
  bool result;
  error_code error = get_bool(result);
  if (error) { return error; }
  return result;
}


template<int N>
SIMDJSON_WARN_UNUSED simdjson_really_inline bool json_iterator::advance_to_buffer(uint8_t (&tmpbuf)[N]) noexcept {
  // Truncate whitespace to fit the buffer.
  auto len = peek_length();
  auto json = advance();
  if (len > N-1) {
    if (stage2::is_not_structural_or_whitespace(json[N])) { return false; }
    len = N-1;
  }

  // Copy to the buffer.
  memcpy(tmpbuf, json, len);
  tmpbuf[len] = ' ';
  return true;
}

constexpr const uint32_t MAX_INT_LENGTH = 1024;

SIMDJSON_WARN_UNUSED error_code json_iterator::get_root_uint64(uint64_t &i) noexcept {
  uint8_t tmpbuf[20+1]; // <20 digits> is the longest possible unsigned integer
  if (!advance_to_buffer(tmpbuf)) { return NUMBER_ERROR; }
  logger::log_value(*this, "uint64", "", 0);
  return stage2::numberparsing::parse_unsigned(buf, i);
}
SIMDJSON_WARN_UNUSED error_code json_iterator::get_root_int64(int64_t &i) noexcept {
  uint8_t tmpbuf[20+1]; // -<19 digits> is the longest possible integer 
  if (!advance_to_buffer(tmpbuf)) { return NUMBER_ERROR; }
  logger::log_value(*this, "int64", "", 0);
  return stage2::numberparsing::parse_integer(buf, i);
}
SIMDJSON_WARN_UNUSED error_code json_iterator::get_root_double(double &d) noexcept {
  // Per https://www.exploringbinary.com/maximum-number-of-decimal-digits-in-binary-floating-point-numbers/, 1074 is the maximum number of significant fractional digits. Add 8 more digits for the biggest number: -0.<fraction>e-308.
  uint8_t tmpbuf[1074+8+1];
  if (!advance_to_buffer(tmpbuf)) { return NUMBER_ERROR; }
  logger::log_value(*this, "double", "", 0);
  return stage2::numberparsing::parse_double(buf, d);
}
SIMDJSON_WARN_UNUSED error_code json_iterator::get_root_bool(bool &b) noexcept {
  uint8_t tmpbuf[5+1];
  if (!advance_to_buffer(tmpbuf)) { return INCORRECT_TYPE; } // Too big! Can't be true or false
  return get_bool(b);
}
simdjson_really_inline bool json_iterator::root_is_null() noexcept {
  uint8_t tmpbuf[4+1];
  if (!advance_to_buffer(tmpbuf)) { return false; } // Too big! Can't be null
  return is_null();
}

simdjson_really_inline void json_iterator::skip_unfinished_children(container c) noexcept {
  SIMDJSON_ASSUME(depth >= c.depth);
  while (depth > c.depth) {
    switch (*advance()) {
      // TODO consider whether matching braces is a requirement: if non-matching braces indicates
      // *missing* braces, then future lookups are not in the object/arrays they think they are,
      // violating the rule "validate enough structure that the user can be confident they are
      // looking at the right values."
      case ']': case '}': depth--; logger::log_end_value(*this, "skip"); break;
      // PERF TODO does it skip the depth check when we don't decrement depth?
      case '[': case '{': logger::log_start_value(*this, "skip"); depth++; break;
      default: logger::log_value(*this, "skip", ""); break;
    }
  }
}

simdjson_really_inline void json_iterator::finish(container c) noexcept {
  while (depth >= c.depth) {
    switch (*advance()) {
      // TODO consider whether matching braces is a requirement: if non-matching braces indicates
      // *missing* braces, then future lookups are not in the object/arrays they think they are,
      // violating the rule "validate enough structure that the user can be confident they are
      // looking at the right values."
      case ']': case '}': depth--; logger::log_end_value(*this, "skip"); break;
      // PERF TODO does it skip the depth check when we don't decrement depth?
      case '[': case '{': logger::log_start_value(*this, "skip"); depth++; break;
      default: logger::log_value(*this, "skip", ""); break;
    }
  }
  SIMDJSON_ASSUME(depth == c.depth-1);
}


simdjson_really_inline void json_iterator::skip() noexcept {
  uint32_t child_depth = 0;
  do {
    switch (*advance()) {
      // TODO consider whether matching braces is a requirement: if non-matching braces indicates
      // *missing* braces, then future lookups are not in the object/arrays they think they are,
      // violating the rule "validate enough structure that the user can be confident they are
      // looking at the right values."
      case ']': case '}': child_depth--; logger::log_end_value(*this, "skip", -1, child_depth); break;
      // PERF TODO does it skip the depth check when we don't decrement depth?
      case '[': case '{': logger::log_start_value(*this, "skip", -1, child_depth); child_depth++; break;
      default: logger::log_value(*this, "skip", "", -1, child_depth); break;
    }
  } while (child_depth > 0);
}

simdjson_really_inline bool json_iterator::in_container(container c) const noexcept {
  return depth >= c.depth;
}

simdjson_really_inline json_iterator::container json_iterator::current_container() const noexcept {
  return depth;
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {
