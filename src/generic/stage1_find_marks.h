// This file contains the common code every implementation uses in stage1
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is included already includes
// "simdjson/stage1_find_marks.h" (this simplifies amalgation)

namespace stage1 {

class bit_indexer {
public:
  uint32_t *tail;

  bit_indexer(uint32_t *index_buf) : tail(index_buf) {}

  // flatten out values in 'bits' assuming that they are are to have values of idx
  // plus their position in the bitvector, and store these indexes at
  // base_ptr[base] incrementing base as we go
  // will potentially store extra values beyond end of valid bits, so base_ptr
  // needs to be large enough to handle this
  really_inline void write_indexes(uint32_t idx, uint64_t bits) {
    // In some instances, the next branch is expensive because it is mispredicted.
    // Unfortunately, in other cases,
    // it helps tremendously.
    if (bits == 0)
        return;
    uint32_t cnt = hamming(bits);

    // Do the first 8 all together
    for (int i=0; i<8; i++) {
      this->tail[i] = idx + trailing_zeroes(bits);
      bits = clear_lowest_bit(bits);
    }

    // Do the next 8 all together (we hope in most cases it won't happen at all
    // and the branch is easily predicted).
    if (unlikely(cnt > 8)) {
      for (int i=8; i<16; i++) {
        this->tail[i] = idx + trailing_zeroes(bits);
        bits = clear_lowest_bit(bits);
      }

      // Most files don't have 16+ structurals per block, so we take several basically guaranteed
      // branch mispredictions here. 16+ structurals per block means either punctuation ({} [] , :)
      // or the start of a value ("abc" true 123) every four characters.
      if (unlikely(cnt > 16)) {
        uint32_t i = 16;
        do {
          this->tail[i] = idx + trailing_zeroes(bits);
          bits = clear_lowest_bit(bits);
          i++;
        } while (i < cnt);
      }
    }

    this->tail += cnt;
  }
};

class json_structural_scanner {
public:
  // Whether the first character of the next iteration is escaped.
  uint64_t prev_escaped = 0ULL;
  // Whether we were still inside a string during the last iteration (all 1's = true, all 0's = false).
  uint64_t prev_in_string = 0ULL;
  // Overflow for the value series calculation we use to validate structure
  uint64_t prev_in_value = 0ULL;
  // Whether the last byte we primitive (i.e. can be followed by primitive or quote)
  uint64_t prev_primitive = 0ULL;
  // Whether the last thing we saw was a separator (i.e. , and spaces)
  uint64_t prev_separator = 0ULL;
  // Mask of structural characters from the last iteration.
  // Kept around for performance reasons, so we can call flatten_bits to soak up some unused
  // CPU capacity while the next iteration is busy with an expensive clmul in compute_quote_mask.
  uint64_t prev_structurals = 0;
  // Whether it has errors of any sort
  uint64_t has_error = 0;
  bit_indexer structural_indexes;

  json_structural_scanner(uint32_t *_structural_indexes) : structural_indexes{_structural_indexes} {}

  //
  // Finish the scan and return any errors.
  //
  // This may detect errors as well, such as unclosed string and certain UTF-8 errors.
  // if streaming is set to true, an unclosed string is allowed.
  //
  really_inline ErrorValues detect_errors_on_eof(bool streaming = false);

  //
  // Return a mask of all string characters plus end quotes.
  //
  // prev_escaped is overflow saying whether the next character is escaped.
  // prev_in_string is overflow saying whether we're still in a string.
  //
  // Backslash sequences outside of quotes will be detected in stage 2.
  //
  really_inline uint64_t find_strings(const simd::simd8x64<uint8_t> in, uint64_t quote);

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
  really_inline uint64_t find_potential_structurals(const simd::simd8x64<uint8_t> in, uint64_t quote, uint64_t &must_be_string);

  //
  // Find the important bits of JSON in a STEP_SIZE-byte chunk, and add them to structural_indexes.
  //
  template<size_t STEP_SIZE>
  really_inline void scan_step(const uint8_t *buf, const size_t idx, utf8_checker &utf8_checker);

  //
  // Parse the entire input in STEP_SIZE-byte chunks.
  //
  template<size_t STEP_SIZE>
  really_inline void scan(const uint8_t *buf, const size_t len, utf8_checker &utf8_checker);
};

// Routines to print masks and text for debugging bitmask operations
UNUSED static char * format_input_text(const simd8x64<uint8_t> in) {
  static char *buf = (char*)malloc(64 + 1);
  in.store((uint8_t*)buf);
  for (size_t i=0; i<64; i++) {
    if (buf[i] < ' ') { buf[i] = '_'; }
  }
  buf[64] = '\0';
  return buf;
}

UNUSED static char * format_mask(uint64_t mask) {
  static char *buf = (char*)malloc(64 + 1);
  for (size_t i=0; i<64; i++) {
    if (mask & (size_t(1) << i)) {
      buf[i] = 'X';
    } else {
      buf[i] = ' ';
    }    
  }
  buf[64] = '\0';
  return buf;
}

UNUSED static char * format_masked_text(uint64_t mask, simd8x64<uint8_t> in) {
  static char *buf = (char*)malloc(64 + 1);
  in.store((uint8_t*)buf);
  for (size_t i=0; i<64; i++) {
    if (mask & (size_t(1) << i)) {
      if (buf[i] <= ' ') { buf[i] = '_'; }
    } else {
      buf[i] = ' ';
    }
  }
  buf[64] = '\0';
  return buf;
}

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
// Finds series of matches, separated by the given separator.
//
// Marks each series start (first match) and terminating separator with 0, and *other*
// characters (characters that are neither match nor separator) with 1 if they are inside a series.
//
// Example
// -------
// Imagine you have a mask of alpha characters, and a mask of commas:
//
// ```
// find_series(alpha, comma)
//
// | Text                      |  , abc def, , ghi , jkl |
// | Alpha Mask                |    abc def    ghi   jkl |
// | Comma Mask                |  ,        , ,     ,     |
// | find_series(alpha, comma) | 00010010000010100100100 |
// |                           |    a  _     , g  _  j   |
//
// alpha &  find_series(alpha, comma) == first alpha after a comma (or first alpha period)
// alpha & ~find_series(alpha, comma) == trailing alpha
// comma &  find_series(alpha, comma) == "extra" commas (for example, if you want to validate that all commas come after a value)
// comma & ~find_series(alpha, comma) == separator commas
// ~(alpha|comma) & find_series(alpha, comma) == all other characters inside the alpha region (for example, the spaces)
// ```
//
// NOTE: this is a fancy description of a simple subtraction operation.
//
really_inline uint64_t find_series(const uint64_t match, const uint64_t separator, uint64_t &overflow) {
  uint64_t result;
  overflow = sub_overflow(separator, overflow, &result);
  overflow |= sub_overflow(result, match, &result);
  return result;
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
//     in.eq('}') & follows(in.eq('{'), in.eq(' '), prev_open_curly) // { <whitespace>* }
//
really_inline uint64_t follows_series(const uint64_t match, const uint64_t filler, uint64_t &overflow) {
  uint64_t result;
  overflow = add_overflow(match, match|filler, &result);
  return result;
}

really_inline ErrorValues json_structural_scanner::detect_errors_on_eof(bool streaming) {
  if ((prev_in_string) and (not streaming)) {
    return UNCLOSED_STRING;
  }
  if (!prev_in_value) {
    return TAPE_ERROR; // open or comma at the end is invalid; there must be at least one value, too.
  }
  if (has_error) {
    return UNESCAPED_CHARS; // TODO also out of order JSON
  }
  // TODO validate that it either has one value, or starts with OPEN
  // (maybe that gets validated in the collection validator ...)
  return SUCCESS;
}

//
// Return a mask of all string characters plus end quotes.
//
// prev_escaped is overflow saying whether the next character is escaped.
// prev_in_string is overflow saying whether we're still in a string.
//
// Backslash sequences outside of quotes will be detected in stage 2.
//
really_inline uint64_t json_structural_scanner::find_strings(const simd::simd8x64<uint8_t> in, uint64_t quote) {
  const uint64_t backslash = in.eq('\\');
  const uint64_t escaped = follows_odd_sequence_of(backslash, prev_escaped);
  const uint64_t real_quote = quote & ~escaped;
  // prefix_xor flips on bits inside the string (and flips off the end quote).
  const uint64_t in_string = prefix_xor(real_quote) ^ prev_in_string;
  /* right shift of a signed value expected to be well-defined and standard
  * compliant as of C++20,
  * John Regher from Utah U. says this is fine code */
  prev_in_string = static_cast<uint64_t>(static_cast<int64_t>(in_string) >> 63);
  // Use ^ to turn the beginning quote off, and the end quote on.
  return in_string ^ real_quote;
}

//
// Validate the sequence of expressions in the JSON:
//
// Open* Prim|Close Close* Separator ...
//
// To reduce dependency on the expensive "what is in a string" computation, this method treats the
// contents of a string the same as content outside. Errors and structurals inside the string or on
// the trailing quote will need to be removed later when the correct string information is known.
//
really_inline uint64_t json_structural_scanner::find_potential_structurals(const simd8x64<uint8_t> in, uint64_t quote, uint64_t &must_be_string) {
  //
  // Classify the characters first ...
  //
  // 0x20 and under is whitespace. We're including plenty of invalid values here, but that's OK; we'll be validating those
  // separately. As long as this *doesn't* include operators or valid value characters, we're good.
  uint64_t space = in.eq(' ') | in.eq('\t') | in.eq('\r') | in.eq('\n');
  uint64_t open = in.eq('[') | in.eq('{');
  uint64_t close = in.eq(']') | in.eq('}');
  uint64_t colon = in.eq(':');
  uint64_t separator = colon | in.eq(',');

  //
  // Validate the operators: Open Prim|Close Close* Separator ...
  //
  // Where Prim is any non-whitespace, non-operator.
  //
  // We subtract Separator - (Prim|Close) to show the following series:
  //
  //        <Open>             |            <Value><Close>             | <Separator>
  // (Separator | Open|Space)* | Prim|Close (Prim|Close | Open|Space)* | Separator
  //     ERR           0       |    1            0       ERR    1      |    0
  //
  uint64_t primitive = ~(separator|open|space|close);
  uint64_t start_value = find_series(primitive|close, separator, prev_in_value);

  //
  // Validate: all of these examples are invalid *unless inside a string* (which we don't know yet):
  //
  // | Category  | Example | Detection                                  |
  // |-----------|---------|--------------------------------------------|
  // | Open      | `1 {`   | Open = 1                                   |
  // |           | `} {`   |                                            |
  // |-----------|---------|--------------------------------------------|
  // | Separator | `, ,`   | Separator = 1                              |
  // |           | `{ ,`   |                                            |
  // |-----------|---------|--------------------------------------------|
  // | Close     | `, }`   | Close = 1 MUST be empty:                   |
  // |           |         | - invalid=Separator Space* Close           |
  // |           |         | - valid=Open Space* Close                  |
  // |           |         | Detected later, in collection validation   |
  // |-----------|---------|--------------------------------------------|
  // | Primitive | `1 1`   | Primitive = 0 preceded by Space|Close      |
  // |           | `1}1`   |                                            |
  // |           | `}1`    |                                            |
  // |-----------|---------|--------------------------------------------|
  // | Quote     | `1"`    | Quote = 0 (any " after the first *must*    |
  // |           | `1"1`   | be in/end of a string).                    |
  // |           | `"""`   |                                            |
  // |           |         | Because of this, the tail quotes in `""`,  |
  // |           |         | `" "`, `\"` and `"\""` will end up valid   |
  // |           |         | (`\"` can occur in a document like         |
  // |           |         | `[1,2,"3,\""]` because separators can      |
  // |           |         | occur inside strings).                     |
  // |-----------|---------|--------------------------------------------|
  // | Space     |         | Whitespace is always cool.                 |
  // |-----------|---------|--------------------------------------------|
  //

  // Separator/Open = 1: `1 {`, `} {`, `, ,`, `{ ,`
  must_be_string = (separator|open) & start_value;

  // Quote = 0: `1"` `1"1` `"""`
  must_be_string |= quote & ~start_value;

  // Value: Error on `1 1`, `1}1`, `""1`, `}1`
  // All primitive chars except the first must be preceded by a non-quote primitive
  // (first " is OK)
  uint64_t tail_primitive = primitive & ~start_value;
  uint64_t tail_quote = quote & ~start_value;
  must_be_string |= tail_primitive & ~follows(primitive & ~tail_quote, prev_primitive);

  // Close: Error on `, }`
  must_be_string |= close & follows_series(separator, space, prev_separator);

  // NOTE: we have not attempted to validate individual characters yet: space and value
  // include a lot of invalid characters. Still, at *this* point we can at least get rid
  // of commas ...
  //
  // NOTE: at this point, *only* quote&start_value can be the beginning of a string.
  // Further, only quotes followed by space+close+separator can be the end of a string (which may
  // include the beginning-of-string quote). Non-backslashed quotes at the end of a string, further,
  // are *always* end of string.
  //
  // The only string ends we can't be sure of are nothing-but-quote-and-space values like
  // `"(Close|Space*)Separator` (e.g. `",",",",","`) or backslashed end strings with separators
  // after them like `...\"(Close|Space)*Separator`, e.g.`"abc\","`.
  //
  // This leaves a very small (though not empty) number of cases where we need clmul or even
  // backslash detection before we can determine a string. We may be able to safely shove those
  // processes into a branch.
  //
  return (start_value & ~space) | open | close | colon;
}

//
// Find the important bits of JSON in a 128-byte chunk, and add them to structural_indexes.
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
template<>
really_inline void json_structural_scanner::scan_step<128>(const uint8_t *buf, const size_t idx, utf8_checker &utf8_checker) {
  //
  // Load up all 128 bytes into SIMD registers
  //
  simd::simd8x64<uint8_t> in_1(buf);
  simd::simd8x64<uint8_t> in_2(buf+64);

  //
  // Find the strings and potential structurals (operators / primitives).
  //
  // This will include false structurals that are *inside* strings--we'll filter strings out
  // before we return.
  //
  uint64_t quote_1 = in_1.eq('"');
  uint64_t string_1 = this->find_strings(in_1, quote_1);
  uint64_t must_be_string_1;
  uint64_t structurals_1 = this->find_potential_structurals(in_1, quote_1, must_be_string_1);
  uint64_t quote_2 = in_2.eq('"');
  uint64_t must_be_string_2;
  uint64_t string_2 = this->find_strings(in_2, quote_2);
  uint64_t structurals_2 = this->find_potential_structurals(in_2, quote_2, must_be_string_2);

  //
  // Do miscellaneous work while the processor is busy calculating strings and structurals.
  //
  // After that, weed out structurals that are inside strings and find invalid string characters.
  //
  uint64_t unescaped_1 = in_1.lteq(0x1F);
  utf8_checker.check_next_input(in_1);
  this->structural_indexes.write_indexes(idx-64, this->prev_structurals); // Output *last* iteration's structurals to ParsedJson
  this->prev_structurals = structurals_1 & ~string_1;
  this->has_error |= must_be_string_1 & ~string_1;
  this->has_error |= unescaped_1 & string_1;

  uint64_t unescaped_2 = in_2.lteq(0x1F);
  utf8_checker.check_next_input(in_2);
  this->structural_indexes.write_indexes(idx, this->prev_structurals); // Output *last* iteration's structurals to ParsedJson
  this->prev_structurals = structurals_2 & ~string_2;
  this->has_error |= must_be_string_2 & ~string_2;
  this->has_error |= unescaped_2 & string_2;
}

//
// Find the important bits of JSON in a 64-byte chunk, and add them to structural_indexes.
//
template<>
really_inline void json_structural_scanner::scan_step<64>(const uint8_t *buf, const size_t idx, utf8_checker &utf8_checker) {
  //
  // Load up bytes into SIMD registers
  //
  simd::simd8x64<uint8_t> in_1(buf);

  //
  // Find the strings and potential structurals (operators / primitives).
  //
  // This will include false structurals that are *inside* strings--we'll filter strings out
  // before we return.
  //
  uint64_t quote_1 = in_1.eq('"');
  uint64_t string_1 = this->find_strings(in_1, quote_1);
  uint64_t must_be_string_1;
  uint64_t structurals_1 = this->find_potential_structurals(in_1, quote_1, must_be_string_1);

  //
  // Do miscellaneous work while the processor is busy calculating strings and structurals.
  //
  // After that, weed out structurals that are inside strings and find invalid string characters.
  //
  uint64_t unescaped_1 = in_1.lteq(0x1F);
  utf8_checker.check_next_input(in_1);
  this->structural_indexes.write_indexes(idx-64, this->prev_structurals); // Output *last* iteration's structurals to ParsedJson
  this->prev_structurals = structurals_1 & ~string_1;
  this->has_error |= must_be_string_1 & ~string_1;
  this->has_error |= unescaped_1 & string_1;
}

template<size_t STEP_SIZE>
really_inline void json_structural_scanner::scan(const uint8_t *buf, const size_t len, utf8_checker &utf8_checker) {
  size_t lenminusstep = len < STEP_SIZE ? 0 : len - STEP_SIZE;
  size_t idx = 0;

  for (; idx < lenminusstep; idx += STEP_SIZE) {
    this->scan_step<STEP_SIZE>(&buf[idx], idx, utf8_checker);
  }

  /* If we have a final chunk of less than STEP_SIZE bytes, pad it to STEP_SIZE with
  * spaces  before processing it (otherwise, we risk invalidating the UTF-8
  * checks). */
  if (likely(idx < len)) {
    uint8_t tmp_buf[STEP_SIZE];
    memset(tmp_buf, 0x20, STEP_SIZE);
    memcpy(tmp_buf, buf + idx, len - idx);
    this->scan_step<STEP_SIZE>(&tmp_buf[0], idx, utf8_checker);
    idx += STEP_SIZE;
  }

  /* finally, flatten out the remaining structurals from the last iteration */
  this->structural_indexes.write_indexes(idx-64, this->prev_structurals);
}

// Setting the streaming parameter to true allows the find_structural_bits to tolerate unclosed strings.
// The caller should still ensure that the input is valid UTF-8. If you are processing substrings,
// you may want to call on a function like trimmed_length_safe_utf8.
template<size_t STEP_SIZE>
int find_structural_bits(const uint8_t *buf, size_t len, simdjson::ParsedJson &pj, bool streaming) {
  if (unlikely(len > pj.byte_capacity)) {
    return simdjson::CAPACITY;
  }
  utf8_checker utf8_checker{};
  json_structural_scanner scanner{pj.structural_indexes.get()};
  scanner.scan<STEP_SIZE>(buf, len, utf8_checker);
  // we might tolerate an unclosed string if streaming is true
  simdjson::ErrorValues error = scanner.detect_errors_on_eof(streaming);
  if (unlikely(error != simdjson::SUCCESS)) {
    return error;
  }
  pj.n_structural_indexes = scanner.structural_indexes.tail - pj.structural_indexes.get();
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
  return utf8_checker.errors();
}

} // namespace stage1