// This file contains the common code every implementation uses in stage1
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is included already includes
// "simdjson/stage1_find_marks.h" (this simplifies amalgation)

// return a bitvector indicating where we have characters that end an odd-length
// sequence of backslashes (and thus change the behavior of the next character
// to follow). A even-length sequence of backslashes, and, for that matter, the
// largest even-length prefix of our odd-length sequence of backslashes, simply
// modify the behavior of the backslashes themselves.
// We also update the prev_iter_ends_odd_backslash reference parameter to
// indicate whether we end an iteration on an odd-length sequence of
// backslashes, which modifies our subsequent search for odd-length
// sequences of backslashes in an obvious way.
really_inline uint64_t follows_odd_sequence_of(const uint64_t match, uint64_t &overflow) {
  const uint64_t even_bits = 0x5555555555555555ULL;
  const uint64_t odd_bits = ~even_bits;
  uint64_t start_edges = match & ~(match << 1);
  /* flip lowest if we have an odd-length run at the end of the prior
   * iteration */
  uint64_t even_start_mask = even_bits ^ overflow;
  uint64_t even_starts = start_edges & even_start_mask;
  uint64_t odd_starts = start_edges & ~even_start_mask;
  uint64_t even_carries = match + even_starts;

  uint64_t odd_carries;
  /* must record the carry-out of our odd-carries out of bit 63; this
   * indicates whether the sense of any edge going to the next iteration
   * should be flipped */
  bool new_overflow = add_overflow(match, odd_starts, &odd_carries);

  odd_carries |= overflow; /* push in bit zero as a
                              * potential end if we had an
                              * odd-numbered run at the
                              * end of the previous
                              * iteration */
  overflow = new_overflow ? 0x1ULL : 0x0ULL;
  uint64_t even_carry_ends = even_carries & ~match;
  uint64_t odd_carry_ends = odd_carries & ~match;
  uint64_t even_start_odd_end = even_carry_ends & odd_bits;
  uint64_t odd_start_even_end = odd_carry_ends & even_bits;
  uint64_t odd_ends = even_start_odd_end | odd_start_even_end;
  return odd_ends;
}

//
// Check if the current character immediately follows a matching character.
//
// For example, this checks for quotes with backslashes in front of them:
//
//     const uint64_t backslashed_quote = in.eq('"') & immediately_follows(in.eq('\'), prev_backslash);
//
really_inline uint64_t follows(const uint64_t match, uint64_t &overflow) {
  const uint64_t result = match << 1 | overflow;
  overflow = match >> 63;
  return result;
}

//
// Check if the current character follows a matching character, with possible "filler" between.
// For example, this checks for empty curly braces, e.g. 
//
//     in.eq('}') & follows(in.eq('['), in.eq(' '), prev_empty_array) // { <whitespace>* }
//
really_inline uint64_t follows(const uint64_t match, const uint64_t filler, uint64_t &overflow ) {
  uint64_t follows_match = follows(match, overflow);
  uint64_t result;
  overflow |= add_overflow(follows_match, filler, &result);
  return result;
}

really_inline ErrorValues detect_errors_on_eof(
  uint64_t &unescaped_chars_error,
  const uint64_t prev_in_string) {
  if (prev_in_string) {
    return UNCLOSED_STRING;
  }
  if (unescaped_chars_error) {
    return UNESCAPED_CHARS;
  }
  return SUCCESS;
}

//
// Determine which characters are *structural*:
// - braces: [] and {}
// - the start of primitives (123, true, false, null)
// - the start of invalid non-whitespace (+, &, ture, UTF-8)
//
// Also detects value sequence errors:
// - two values with no separator between ("hello" "world")
// - separators with no values ([1,] [1,,]and [,2])
//
// This method will find all of the above whether it is in a string or not.
//
// To reduce dependency on the expensive "what is in a string" computation, this method treats the
// contents of a string the same as content outside. Errors and structurals inside the string or on
// the trailing quote will need to be removed later when the correct string information is known.
//
really_inline uint64_t find_potential_structurals(uint64_t whitespace, uint64_t op, uint64_t &prev_primitive) {
  // Detect the start of a run of primitive characters. Includes numbers, booleans, and strings (").
  // Everything except whitespace, braces, colon and comma.
  const uint64_t primitive = ~(op | whitespace);
  const uint64_t follows_primitive = follows(primitive, prev_primitive);
  const uint64_t start_primitive = primitive & ~follows_primitive;

  // Return final structurals
  return op | start_primitive;
}

// All independent tasks that scan the SIMD input go here.
really_inline void stage2_scan_input(
  const simd_input<ARCHITECTURE> in,
  uint64_t &prev_escaped, uint64_t &prev_primitive, utf8_checker<ARCHITECTURE> utf8_state,
  uint64_t &quote, uint64_t &potential_structurals, uint64_t &invalid_string_bytes
) {
  // Finding real quotes, and finding potential structurals, are interleaved here.
  // First we read the input we need from SIMD for them ...
  uint64_t backslash = in.eq('\\');
  uint64_t whitespace, op;
  find_whitespace_and_operators(in, whitespace, op);

  // Then we do the actual work. follows_odd_sequence_of takes longer than find_potential_structurals.
  const uint64_t escaped = follows_odd_sequence_of(backslash, prev_escaped);
  potential_structurals = find_potential_structurals(whitespace, op, prev_primitive);
  uint64_t raw_quote = in.eq('"');
  quote = raw_quote & ~escaped;
  invalid_string_bytes = in.lteq(0x1F);

  // The UTF-8 scan is pretty much independent of anything else, so we stick it at the end here to
  // soak up some CPU.
  utf8_state.check_next_input(in);
}

//
// Return a mask of all string characters plus end quotes.
//
// prev_escaped is overflow saying whether the next character is escaped.
// prev_in_string is overflow saying whether we're still in a string.
//
// Backslash sequences outside of quotes will be detected in stage 2.
//
really_inline uint64_t find_strings(uint64_t quote, uint64_t &prev_in_string) {
  // compute_quote_mask returns start quote plus string contents.
  const uint64_t in_string = compute_quote_mask(quote) ^ prev_in_string;
  /* right shift of a signed value expected to be well-defined and standard
   * compliant as of C++20,
   * John Regher from Utah U. says this is fine code */
  prev_in_string = static_cast<uint64_t>(static_cast<int64_t>(in_string) >> 63);
  // Use ^ to turn the beginning quote off, and the end quote on (i.e. everything except the
  // starting quote is the string's value).
  return in_string ^ quote;
}

really_inline void stage3_find_strings(
  uint64_t quote,
  uint64_t &prev_in_string,
  uint64_t &string,
  const uint64_t potential_structurals, uint64_t &potential_structurals_out, const uint64_t invalid_string_bytes, uint64_t &invalid_string_bytes_out
) {
  string = find_strings(quote, prev_in_string);
  potential_structurals_out = potential_structurals;
  invalid_string_bytes_out = invalid_string_bytes;
}


//
// Find the important bits of JSON in a 128-byte chunk, and add them to :
//
//
//
// PERF NOTES:
// We pipe 2 inputs through these stages:
// 1. Load JSON into registers. This takes a long time and is highly parallelizable, so we load
//    2 inputs' worth at once so that by the time step 2 is looking for them input, it's available.
// 2. Scan the JSON for critical data: strings, primitives and operators. This is the critical path.
//    The output of step 1 depends entirely on this information. These functions don't quite use
//    up enough CPU: the second half of the functions is highly serial, only using 1 execution core
//    at a time. The second input's scans has some dependency on the first ones finishing it, but
//    they can make a lot of progress before they need that information.
// 3. Step 1 doesn't use enough capacity, so we run some extra stuff while we're waiting for that
//    to finish: utf-8 checks and generating the output from the last iteration.
// 
// The reason we run 2 inputs at a time, is steps 2 and 3 are *still* not enough to soak up all
// available capacity with just one input. Running 2 at a time seems to give the CPU a good enough
// workout.
//
really_inline bool stage1_read_input(
  const uint8_t *&buf,
  const uint8_t *buf_end,
  simd_input<ARCHITECTURE> &in
) {
  // If we have a full input, load it up and return it.
  const uint8_t *next_buf = buf+64;
  if (likely(next_buf < buf_end)) {
    in = simd_input<ARCHITECTURE>(buf);
    buf = next_buf;
    return true;
  }

  // If we have a *partial* (< 64 byte) input, load it up and return it.
  if (likely(buf_end > buf)) {
    uint8_t tmp_buf[64];
    memset(tmp_buf, 0x20, 64);
    memcpy(tmp_buf, buf, buf_end-buf);
    in = simd_input<ARCHITECTURE>(tmp_buf);
    buf = next_buf;
    return true;
  }

  return false;
}

really_inline void stage4_output_structurals(
  const uint64_t string, const uint64_t structurals, const uint64_t invalid_string_bytes,
  uint32_t *&base_ptr, size_t &idx,
  uint64_t &unescaped_chars_error
) {
  flatten_bits(base_ptr, idx, structurals & ~string);
  unescaped_chars_error |= invalid_string_bytes & string;
}

int find_structural_bits(const uint8_t *buf, size_t len, simdjson::ParsedJson &pj) {
  if (unlikely(len > pj.byte_capacity)) {
    std::cerr << "Your ParsedJson object only supports documents up to "
              << pj.byte_capacity << " bytes but you are trying to process "
              << len << " bytes" << std::endl;
    return simdjson::CAPACITY;
  }

  // Stage 1 state and output
  const uint8_t *buf_end = buf + len;
  simd_input<ARCHITECTURE> in;

  // Stage 2 state and output
  uint64_t quote, potential_structurals, invalid_string_bytes;
  // Whether the first character of the next iteration is escaped.
  uint64_t prev_escaped = 0;
  // Whether the last character of the previous iteration is a primitive value character
  // (anything except whitespace, braces, comma or colon).
  uint64_t prev_primitive = 0;
  utf8_checker<ARCHITECTURE> utf8_state;

  // Stage 3 state and output
  uint64_t prev_in_string = 0;
  uint64_t string, potential_structurals2, invalid_string_bytes2;

  // Stage 4 state and output
  uint32_t *base_ptr = pj.structural_indexes;
  uint64_t idx = 0;
  // Errors with unescaped characters in strings (ASCII codepoints < 0x20)
  uint64_t unescaped_chars_error = 0;

  // Read the first input (if any)
  if (stage1_read_input(buf, buf_end, in)) {
    // Stage 2 is ready. Push it so stage 3 is ready.
    stage2_scan_input(in, prev_escaped, prev_primitive, utf8_state, quote, potential_structurals, invalid_string_bytes);

    // Read the second input (if any)
    if (stage1_read_input(buf, buf_end, in)) {
      // stages 2 and 3 are ready. Push them so 3 and 4 are ready.
      stage3_find_strings(quote, prev_in_string, string, potential_structurals, potential_structurals2, invalid_string_bytes, invalid_string_bytes2);
      stage2_scan_input(in, prev_escaped, prev_primitive, utf8_state, quote, potential_structurals, invalid_string_bytes);

      // Read remaining inputs
      while (stage1_read_input(buf, buf_end, in)) {
        // stages 2, 3 and 4 are ready. Push them so 3 and 4 are ready.
        stage4_output_structurals(string, potential_structurals2, invalid_string_bytes2, base_ptr, idx, unescaped_chars_error);
        stage3_find_strings(quote, prev_in_string, string, potential_structurals, potential_structurals2, invalid_string_bytes, invalid_string_bytes2);
        stage2_scan_input(in, prev_escaped, prev_primitive, utf8_state, quote, potential_structurals, invalid_string_bytes);
      }
      // stages 3 and 4 are ready. Push 4 so only 3 is ready.
      stage4_output_structurals(string, potential_structurals2, invalid_string_bytes2, base_ptr, idx, unescaped_chars_error);
    }

    // stage 3 is ready. Finish it.
    stage3_find_strings(quote, prev_in_string, string, potential_structurals, potential_structurals2, invalid_string_bytes, invalid_string_bytes2);
    stage4_output_structurals(string, potential_structurals2, invalid_string_bytes2, base_ptr, idx, unescaped_chars_error);
  }

  simdjson::ErrorValues error = detect_errors_on_eof(unescaped_chars_error, prev_in_string);
  if (unlikely(error != simdjson::SUCCESS)) {
    return error;
  }

  pj.n_structural_indexes = base_ptr - pj.structural_indexes;
  /* a valid JSON file cannot have zero structural indexes - we should have
   * found something */
  if (unlikely(pj.n_structural_indexes == 0u)) {
    return simdjson::EMPTY;
  }
  if (unlikely(pj.structural_indexes[pj.n_structural_indexes - 1] > len)) {
    return simdjson::UNEXPECTED_ERROR;
  }
  if (len != pj.structural_indexes[pj.n_structural_indexes - 1]) {
    /* the string might not be NULL terminated, but we add a virtual NULL
     * ending character. */
    pj.structural_indexes[pj.n_structural_indexes++] = len;
  }
  /* make it safe to dereference one beyond this array */
  pj.structural_indexes[pj.n_structural_indexes] = 0;
  return utf8_state.errors();
}
