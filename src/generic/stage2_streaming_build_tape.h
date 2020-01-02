namespace stage2 {

struct streaming_structural_parser: structural_parser {
  really_inline streaming_structural_parser(const uint8_t *_buf, size_t _len, ParsedJson &_pj, size_t _i) : structural_parser(_buf, _len, _pj, _i) {}

  // override to add streaming
  WARN_UNUSED really_inline int start(ret_address finish_parser) {
    pj.init(); // sets is_valid to false
    // Capacity ain't no thang for streaming, so we don't check it.
    // Advance to the first character as soon as possible
    advance_char();
    // Push the root scope (there is always at least one scope)
    if (push_start_scope(finish_parser, 'r')) {
      return DEPTH_ERROR;
    }
    return SUCCESS;
  }

  // override to add streaming
  WARN_UNUSED really_inline int finish() {
    /* the string might not be NULL terminated. */
    if ( i + 1 > pj.n_structural_indexes ) {
      return set_error_code(TAPE_ERROR);
    }
    bool finished = i + 1 == pj.n_structural_indexes;
    if (finished && buf[idx+2] != '\0') {
      return set_error_code(TAPE_ERROR);
    }
    pop_root_scope();
    if (depth != 0) {
      return set_error_code(TAPE_ERROR);
    }
    if (pj.containing_scope_offset[depth] != 0) {
      return set_error_code(TAPE_ERROR);
    }

    pj.valid = true;
    return set_error_code(finished ? SUCCESS : SUCCESS_AND_HAS_MORE);
  }

  WARN_UNUSED int parse(size_t &next_json) {
    static constexpr unified_machine_addresses addresses = INIT_ADDRESSES();
    int result = start(addresses.finish);
    if (result) { return result; }

    //
    // Read first value
    //
    switch (c) {
    case '{':
      FAIL_IF( push_start_scope(addresses.finish) );
      goto object_begin;
    case '[':
      FAIL_IF( push_start_scope(addresses.finish) );
      goto array_begin;
    case '"':
      FAIL_IF( parse_string() );
      goto finish;
    case 't': case 'f': case 'n':
      FAIL_IF(
        with_space_terminated_copy([&](auto copy, auto offset) {
          return parse_atom(copy, offset);
        })
      );
      goto finish;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      FAIL_IF(
        with_space_terminated_copy([&](auto copy, auto offset) {
          return parse_number(copy, offset, false);
        })
      );
      goto finish;
    case '-':
      FAIL_IF(
        with_space_terminated_copy([&](auto copy, auto offset) {
          return parse_number(copy, offset, true);
        })
      );
      goto finish;
    default:
      goto error;
    }

  //
  // Object parser parsers
  //
  object_begin:
    advance_char();
    switch (c) {
    case '"': {
      FAIL_IF( parse_string() );
      goto object_key_parser;
    }
    case '}':
      goto scope_end; // could also go to object_continue
    default:
      goto error;
    }

  object_key_parser:
    FAIL_IF( advance_char() != ':' );

    advance_char();
    GOTO( parse_value(addresses, addresses.object_continue) );

  object_continue:
    switch (advance_char()) {
    case ',':
      FAIL_IF( advance_char() != '"' );
      FAIL_IF( parse_string() );
      goto object_key_parser;
    case '}':
      goto scope_end;
    default:
      goto error;
    }

  scope_end:
    CONTINUE( pop_scope() );

  //
  // Array parser parsers
  //
  array_begin:
    if (advance_char() == ']') {
      goto scope_end; // could also go to array_continue
    }

  main_array_switch:
    /* we call update char on all paths in, so we can peek at c on the
    * on paths that can accept a close square brace (post-, and at start) */
    GOTO( parse_value(addresses, addresses.array_continue) );

  array_continue:
    switch (advance_char()) {
    case ',':
      advance_char();
      goto main_array_switch;
    case ']':
      goto scope_end;
    default:
      goto error;
    }

  finish:
    next_json = i;
    return finish();

  error:
    return error();
  }
};

} // namespace stage2