namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline json_iterator::json_iterator(json_iterator &&other) noexcept
  : token_iterator(std::forward<token_iterator>(other)),
    parser{other.parser},
    current_string_buf_loc{other.current_string_buf_loc},
    depth{other.depth}
{
  other.parser = nullptr;
}
simdjson_really_inline json_iterator &json_iterator::operator=(json_iterator &&other) noexcept {
  buf = other.buf;
  index = other.index;
  parser = other.parser;
  current_string_buf_loc = other.current_string_buf_loc;
  depth = other.depth;
  other.parser = nullptr;
  return *this;
}

simdjson_really_inline json_iterator::json_iterator(ondemand::parser *_parser) noexcept
  : token_iterator(_parser->dom_parser.buf, _parser->dom_parser.structural_indexes.get()),
    parser{_parser},
    current_string_buf_loc{parser->string_buf.get()},
    depth{1}
{
  // Release the string buf so it can be reused by the next document
  logger::log_headers();
}

simdjson_warn_unused simdjson_really_inline error_code json_iterator::skip() noexcept {
  switch (ch) {
    case '[': case '{':
      logger::log_start_value(*this, "skip");
      depth_t parent_depth = depth;
      depth++;
      finish_child(parent_depth);
      break;
  }
}

simdjson_warn_unused simdjson_really_inline error_code json_iterator::finish_child(depth_t parent_depth) noexcept {
  if (depth < parent_depth) { return SUCCESS; }
  // The loop breaks only when depth-- happens.
  auto end = &parser->dom_parser.structural_indexes[parser->dom_parser.n_structural_indexes];
  while (index <= end) {
    uint8_t ch = *advance();
    switch (ch) {
      // TODO consider whether matching braces is a requirement: if non-matching braces indicates
      // *missing* braces, then future lookups are not in the object/arrays they think they are,
      // violating the rule "validate enough structure that the user can be confident they are
      // looking at the right values."
      case ']': case '}':
        logger::log_end_value(*this, "skip");
        depth--;
        if (depth < parent_depth) { return SUCCESS; }
        break;
      // PERF TODO does it skip the depth check when we don't decrement depth?
      case '[': case '{':
        logger::log_start_value(*this, "skip");
        depth++;
        break;
      default:
        logger::log_value(*this, "skip", "");
        break;
    }
  }

  return report_error(TAPE_ERROR, "not enough close braces");
}

simdjson_really_inline bool json_iterator::at_start() const noexcept {
  return index == parser->dom_parser.structural_indexes.get();
}

simdjson_really_inline bool json_iterator::at_eof() const noexcept {
  return index == &parser->dom_parser.structural_indexes[parser->dom_parser.n_structural_indexes];
}

simdjson_really_inline bool json_iterator::is_alive() const noexcept {
  return parser;
}

simdjson_really_inline const uint8_t *json_iterator::advance(int32_t delta_depth) noexcept {
  depth += delta_depth;
  return advance();
}

simdjson_really_inline error_code json_iterator::report_error(error_code _error, const char *message) noexcept {
  SIMDJSON_ASSUME(_error != SUCCESS && _error != UNINITIALIZED && _error != INCORRECT_TYPE && _error != NO_SUCH_FIELD);
  logger::log_error(*this, message);
  error = _error;
  return error;
}

template<int N>
simdjson_warn_unused simdjson_really_inline bool json_iterator::copy_to_buffer(const uint8_t *json, uint32_t max_len, uint8_t (&tmpbuf)[N]) noexcept {
  // Truncate whitespace to fit the buffer.
  if (max_len > N-1) {
    if (jsoncharutils::is_not_structural_or_whitespace(json[N])) { return false; }
    max_len = N-1;
  }

  // Copy to the buffer.
  std::memcpy(tmpbuf, json, max_len);
  tmpbuf[max_len] = ' ';
  return true;
}

template<int N>
simdjson_warn_unused simdjson_really_inline bool json_iterator::peek_to_buffer(uint8_t (&tmpbuf)[N]) noexcept {
  auto max_len = token.peek_length();
  auto json = token.peek();
  return copy_to_buffer(json, max_len, tmpbuf);
}

template<int N>
simdjson_warn_unused simdjson_really_inline bool json_iterator::advance_to_buffer(uint8_t (&tmpbuf)[N]) noexcept {
  auto max_len = peek_length();
  auto json = advance();
  return copy_to_buffer(json, max_len, tmpbuf);
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>::simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::json_iterator &&value) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>(value)) {}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>::simdjson_result(error_code error) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>(error) {}

} // namespace simdjson