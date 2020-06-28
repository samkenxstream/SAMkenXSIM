namespace stage2 {
namespace numberparsing {

#ifdef JSON_TEST_NUMBERS
#define INVALID_NUMBER(SRC, ERROR) (printf("ERROR: %s (parsing %s)\n", (ERROR), (SRC)), found_invalid_number((const uint8_t *)(SRC)), false)
#define WRITE_INTEGER(VALUE, SRC, WRITER) (found_integer((VALUE), (const uint8_t *)(SRC)), writer.append_s64((VALUE)))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) (found_unsigned_integer((VALUE), (const uint8_t *)(SRC)), writer.append_u64((VALUE)))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) (found_float((VALUE), (const uint8_t *)(SRC)), writer.append_double((VALUE)))
#else
#define INVALID_NUMBER(SRC, ERROR) (printf("ERROR: %s (parsing %s)\n", (ERROR), (SRC)), false)
#define WRITE_INTEGER(VALUE, SRC, WRITER) writer.append_s64((VALUE))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) writer.append_u64((VALUE))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) writer.append_double((VALUE))
#endif

// Attempts to compute i * 10^(power) exactly; and if "negative" is
// true, negate the result.
// This function will only work in some cases, when it does not work, success is
// set to false. This should work *most of the time* (like 99% of the time).
// We assume that power is in the [FASTFLOAT_SMALLEST_POWER,
// FASTFLOAT_LARGEST_POWER] interval: the caller is responsible for this check.
really_inline double compute_float_64(int64_t power, uint64_t i, bool negative, bool *success) {
  // we start with a fast path
  // It was described in
  // Clinger WD. How to read floating point numbers accurately.
  // ACM SIGPLAN Notices. 1990
  if (-22 <= power && power <= 22 && i <= 9007199254740991) {
    // convert the integer into a double. This is lossless since
    // 0 <= i <= 2^53 - 1.
    double d = double(i);
    //
    // The general idea is as follows.
    // If 0 <= s < 2^53 and if 10^0 <= p <= 10^22 then
    // 1) Both s and p can be represented exactly as 64-bit floating-point
    // values
    // (binary64).
    // 2) Because s and p can be represented exactly as floating-point values,
    // then s * p
    // and s / p will produce correctly rounded values.
    //
    if (power < 0) {
      d = d / power_of_ten[-power];
    } else {
      d = d * power_of_ten[power];
    }
    if (negative) {
      d = -d;
    }
    *success = true;
    return d;
  }
  // When 22 < power && power <  22 + 16, we could
  // hope for another, secondary fast path.  It wa
  // described by David M. Gay in  "Correctly rounded
  // binary-decimal and decimal-binary conversions." (1990)
  // If you need to compute i * 10^(22 + x) for x < 16,
  // first compute i * 10^x, if you know that result is exact
  // (e.g., when i * 10^x < 2^53),
  // then you can still proceed and do (i * 10^x) * 10^22.
  // Is this worth your time?
  // You need  22 < power *and* power <  22 + 16 *and* (i * 10^(x-22) < 2^53)
  // for this second fast path to work.
  // If you you have 22 < power *and* power <  22 + 16, and then you
  // optimistically compute "i * 10^(x-22)", there is still a chance that you
  // have wasted your time if i * 10^(x-22) >= 2^53. It makes the use cases of
  // this optimization maybe less common than we would like. Source:
  // http://www.exploringbinary.com/fast-path-decimal-to-floating-point-conversion/
  // also used in RapidJSON: https://rapidjson.org/strtod_8h_source.html

  // The fast path has now failed, so we are failing back on the slower path.

  // In the slow path, we need to adjust i so that it is > 1<<63 which is always
  // possible, except if i == 0, so we handle i == 0 separately.
  if(i == 0) {
    return 0.0;
  }

  // We are going to need to do some 64-bit arithmetic to get a more precise product.
  // We use a table lookup approach.
  components c =
      power_of_ten_components[power - FASTFLOAT_SMALLEST_POWER];
      // safe because
      // power >= FASTFLOAT_SMALLEST_POWER
      // and power <= FASTFLOAT_LARGEST_POWER
  // we recover the mantissa of the power, it has a leading 1. It is always
  // rounded down.
  uint64_t factor_mantissa = c.mantissa;

  // We want the most significant bit of i to be 1. Shift if needed.
  int lz = leading_zeroes(i);
  i <<= lz;
  // We want the most significant 64 bits of the product. We know
  // this will be non-zero because the most significant bit of i is
  // 1.
  value128 product = full_multiplication(i, factor_mantissa);
  uint64_t lower = product.low;
  uint64_t upper = product.high;

  // We know that upper has at most one leading zero because
  // both i and  factor_mantissa have a leading one. This means
  // that the result is at least as large as ((1<<63)*(1<<63))/(1<<64).

  // As long as the first 9 bits of "upper" are not "1", then we
  // know that we have an exact computed value for the leading
  // 55 bits because any imprecision would play out as a +1, in
  // the worst case.
  if (unlikely((upper & 0x1FF) == 0x1FF) && (lower + i < lower)) {
    uint64_t factor_mantissa_low =
        mantissa_128[power - FASTFLOAT_SMALLEST_POWER];
    // next, we compute the 64-bit x 128-bit multiplication, getting a 192-bit
    // result (three 64-bit values)
    product = full_multiplication(i, factor_mantissa_low);
    uint64_t product_low = product.low;
    uint64_t product_middle2 = product.high;
    uint64_t product_middle1 = lower;
    uint64_t product_high = upper;
    uint64_t product_middle = product_middle1 + product_middle2;
    if (product_middle < product_middle1) {
      product_high++; // overflow carry
    }
    // We want to check whether mantissa *i + i would affect our result.
    // This does happen, e.g. with 7.3177701707893310e+15.
    if (((product_middle + 1 == 0) && ((product_high & 0x1FF) == 0x1FF) &&
         (product_low + i < product_low))) { // let us be prudent and bail out.
      *success = false;
      return 0;
    }
    upper = product_high;
    lower = product_middle;
  }
  // The final mantissa should be 53 bits with a leading 1.
  // We shift it so that it occupies 54 bits with a leading 1.
  ///////
  uint64_t upperbit = upper >> 63;
  uint64_t mantissa = upper >> (upperbit + 9);
  lz += int(1 ^ upperbit);

  // Here we have mantissa < (1<<54).

  // We have to round to even. The "to even" part
  // is only a problem when we are right in between two floats
  // which we guard against.
  // If we have lots of trailing zeros, we may fall right between two
  // floating-point values.
  if (unlikely((lower == 0) && ((upper & 0x1FF) == 0) &&
               ((mantissa & 3) == 1))) {
      // if mantissa & 1 == 1 we might need to round up.
      //
      // Scenarios:
      // 1. We are not in the middle. Then we should round up.
      //
      // 2. We are right in the middle. Whether we round up depends
      // on the last significant bit: if it is "one" then we round
      // up (round to even) otherwise, we do not.
      //
      // So if the last significant bit is 1, we can safely round up.
      // Hence we only need to bail out if (mantissa & 3) == 1.
      // Otherwise we may need more accuracy or analysis to determine whether
      // we are exactly between two floating-point numbers.
      // It can be triggered with 1e23.
      // Note: because the factor_mantissa and factor_mantissa_low are
      // almost always rounded down (except for small positive powers),
      // almost always should round up.
      *success = false;
      return 0;
  }

  mantissa += mantissa & 1;
  mantissa >>= 1;

  // Here we have mantissa < (1<<53), unless there was an overflow
  if (mantissa >= (1ULL << 53)) {
    //////////
    // This will happen when parsing values such as 7.2057594037927933e+16
    ////////
    mantissa = (1ULL << 52);
    lz--; // undo previous addition
  }
  mantissa &= ~(1ULL << 52);
  uint64_t real_exponent = c.exp - lz;
  // we have to check that real_exponent is in range, otherwise we bail out
  if (unlikely((real_exponent < 1) || (real_exponent > 2046))) {
    *success = false;
    return 0;
  }
  mantissa |= real_exponent << 52;
  mantissa |= (((uint64_t)negative) << 63);
  double d;
  memcpy(&d, &mantissa, sizeof(d));
  *success = true;
  return d;
}

static bool parse_float_strtod(const char *ptr, double *outDouble) {
  char *endptr;
  *outDouble = strtod(ptr, &endptr);
  // Some libraries will set errno = ERANGE when the value is subnormal,
  // yet we may want to be able to parse subnormal values.
  // However, we do not want to tolerate NAN or infinite values.
  //
  // Values like infinity or NaN are not allowed in the JSON specification.
  // If you consume a large value and you map it to "infinity", you will no
  // longer be able to serialize back a standard-compliant JSON. And there is
  // no realistic application where you might need values so large than they
  // can't fit in binary64. The maximal value is about  1.7976931348623157 x
  // 10^308 It is an unimaginable large number. There will never be any piece of
  // engineering involving as many as 10^308 parts. It is estimated that there
  // are about 10^80 atoms in the universe.  The estimate for the total number
  // of electrons is similar. Using a double-precision floating-point value, we
  // can represent easily the number of atoms in the universe. We could  also
  // represent the number of ways you can pick any three individual atoms at
  // random in the universe. If you ever encounter a number much larger than
  // 10^308, you know that you have a bug. RapidJSON will reject a document with
  // a float that does not fit in binary64. JSON for Modern C++ (nlohmann/json)
  // will flat out throw an exception.
  //
  if ((endptr == ptr) || (!std::isfinite(*outDouble))) {
    return false;
  }
  return true;
}

really_inline bool is_integer(char c) {
  return (c >= '0' && c <= '9');
  // this gets compiled to (uint8_t)(c - '0') <= 9 on all decent compilers
}


// check quickly whether the next 8 chars are made of digits
// at a glance, it looks better than Mula's
// http://0x80.pl/articles/swar-digits-validate.html
really_inline bool is_made_of_eight_digits_fast(const char *chars) {
  uint64_t val;
  // this can read up to 7 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(7 <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be bigger than 7");
  memcpy(&val, chars, 8);
  // a branchy method might be faster:
  // return (( val & 0xF0F0F0F0F0F0F0F0 ) == 0x3030303030303030)
  //  && (( (val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0 ) ==
  //  0x3030303030303030);
  return (((val & 0xF0F0F0F0F0F0F0F0) |
           (((val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0) >> 4)) ==
          0x3333333333333333);
}

template<typename W>
bool slow_float_parsing(UNUSED const char * src, W writer) {
  double d;
  if (parse_float_strtod(src, &d)) {
    WRITE_DOUBLE(d, src, writer);
    return true;
  }
  return INVALID_NUMBER(src, "parse_float_strtod failed");
}

really_inline bool parse_exponent(UNUSED const uint8_t *const src, const char *&p, int64_t &exponent) {
  bool neg_exp = false;
  if ('-' == *p) {
    neg_exp = true;
    ++p;
  } else if ('+' == *p) {
    ++p;
  }

  // e[+-] must be followed by a number
  if (!is_integer(*p)) { return INVALID_NUMBER(src, "e followed by non-number"); }
  unsigned char digit = static_cast<unsigned char>(*p - '0');
  int64_t exp_number = digit;
  p++;
  if (is_integer(*p)) {
    digit = static_cast<unsigned char>(*p - '0');
    exp_number = 10 * exp_number + digit;
    ++p;
  }
  if (is_integer(*p)) {
    digit = static_cast<unsigned char>(*p - '0');
    exp_number = 10 * exp_number + digit;
    ++p;
  }
  while (is_integer(*p)) {
    // we need to check for overflows; we refuse to parse this
    if (exp_number > 0x100000000) { return INVALID_NUMBER(src, "exponent too large"); }
    digit = static_cast<unsigned char>(*p - '0');
    exp_number = 10 * exp_number + digit;
    ++p;
  }
  exponent += (neg_exp ? -exp_number : exp_number);
  return true;
}

template<typename W>
really_inline bool write_float(const char *const src, bool negative, uint64_t i, int64_t exponent, size_t digit_count, W &writer) {
  //
  // Check for integer (unsigned) overflow
  //
  // - The longest positive 64-bit number is 20 digits.
  //
  if (digit_count > 20) { return INVALID_NUMBER(src, "More than 20 digits in decimal integer"); }
  if (digit_count == 20) {
    //
    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    if ((src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX))) { return INVALID_NUMBER(src, "Overflowed 20-digit decimal integer"); }
  }

  if (unlikely(exponent < FASTFLOAT_SMALLEST_POWER || exponent > FASTFLOAT_LARGEST_POWER)) { // this is uncommon!!!
    // this is almost never going to get called!!!
    // we start anew, going slowly!!!
    bool success = slow_float_parsing(src, writer);
    // The number was already written, but we made a copy of the writer when we passed it to the
    // slow_float_parsing() function, so we have to skip those tape spots now that we've returned
    writer.skip_double();
    return success;
  }
  bool success = true;
  double d = compute_float_64(exponent, i, negative, &success);
  if (!success) {
    // we are almost never going to get here.
    if (!parse_float_strtod((const char *)src, &d)) { return INVALID_NUMBER(src, "parse_float_strtod failed"); }
  }
  WRITE_DOUBLE(d, src, writer);
  return true;
}

really_inline void parse_more_digits(const char *&p, uint64_t &i) {
  // the is_made_of_eight_digits_fast routine is unlikely to help here because
  // we rarely see large integer parts like 123456789
  while (is_integer(*p)) {
    unsigned char digit = static_cast<unsigned char>(*p - '0');
    // a multiplication by 10 is cheaper than an arbitrary integer
    // multiplication
    i = 10 * i + digit; // might overflow, we will handle the overflow later
    ++p;
  }
}

really_inline void parse_many_more_digits(const char *&p, uint64_t &i) {
#ifdef SWAR_NUMBER_PARSING
  // this helps if we have lots of decimals!
  // this turns out to be frequent enough.
  if (is_made_of_eight_digits_fast(p)) {
    i = i * 100000000 + parse_eight_digits_unrolled(p);
    p += 8;
  }
#endif
  while (is_integer(*p)) {
    unsigned char digit = static_cast<unsigned char>(*p - '0');
    ++p;
    i = i * 10 + digit; // in rare cases, this will overflow, but that's ok
                        // because we have parse_highprecision_float later.
  }
}

// parse the number at src
// define JSON_TEST_NUMBERS for unit testing
//
// It is assumed that the number is followed by a structural ({,},],[) character
// or a white space character. If that is not the case (e.g., when the JSON
// document is made of a single number), then it is necessary to copy the
// content and append a space before calling this function.
//
// Our objective is accurate parsing (ULP of 0) at high speed.
template<typename W>
really_inline bool parse_number(UNUSED const uint8_t *const src,
                                UNUSED bool found_minus,
                                W &writer) {
#ifdef SIMDJSON_SKIPNUMBERPARSING // for performance analysis, it is sometimes
                                  // useful to skip parsing
  writer.append_s64(0);        // always write zero
  return true;                    // always succeeds
#else
  const char *p = reinterpret_cast<const char *>(src);
  //
  // Handle the leading -
  //
  bool negative = false;
  if (found_minus) {
    ++p;
    negative = true;
    // a negative sign must be followed by an integer
    if (!is_integer(*p)) { return INVALID_NUMBER(src, "- followed by non-digit"); }
  }

  //
  // Parse xxx.yyy as if it were an integer
  //
  size_t digit_count;
  uint64_t i;
  int64_t exponent;

  if ('0' == *p) {
    //
    // Leading 0. (0 or 0.00000123456)
    //
    p++;
    if ('.' == *p) {
      //
      // 0.0000123456
      //

      // Skip leading decimal zeroes (0.0000) to find the first significant digit
      p++;
      const char *start_decimal_digits = p;
      i = 0;
      if (!is_integer(*p)) { return INVALID_NUMBER(src, "0. followed by non-digit"); } // 0. must be followed by at least one digit
      while ('0' == *p) { p++; } // Skip leading zeroes

      // Parse the significant part
      const char *start_significant_digits = p;
      parse_more_decimal_digits(p, i);
      digit_count = p - start_significant_digits;
      exponent = -(p - start_decimal_digits);

    } else if (!is_structural_or_whitespace(*p)) {
      //
      // 0e123
      //
      if ('e' != *p && 'E' != *p) { return INVALID_NUMBER(src, "0 not followed by e or end of number"); } // 0x or something equally dubious
      i = 0;
      exponent = 0;
      digit_count = 1;

    } else {
      //
      // 0
      //
      WRITE_INTEGER(0, src, writer);
      return true;
    }
  } else {
    //
    // 123456 or 123.456
    //
    const char *start_digits = p;

    // Parse the integer part (123456 or 123). We are guaranteed the first character is a digit,
    // so we don't check that.
    const char *start_decimal_digits;
    unsigned char digit = static_cast<unsigned char>(*p - '0');
    i = digit;
    p++;
    parse_more_decimal_digits(p, i);

    if ('.' == *p) {
      //
      // Decimal (123.456)
      //
      p++;
      start_decimal_digits = p;
      // Parse the first decimal digit (if there isn't one, it's an error)
      if (!is_integer(*p)) { return INVALID_NUMBER(src, ". followed by non-digit"); }
      digit = static_cast<unsigned char>(*p - '0');
      i = i*10 + digit;
      p++;

      // Parse remaining decimal digits
      parse_more_digits(p, i);
      exponent = -(p - start_decimal_digits);
      digit_count = p - start_digits - 1;

    } else if (!is_structural_or_whitespace(*p)) {
      //
      // Integer with exponent (123e456)
      //
      if ('e' != *p && 'E' != *p) { return INVALID_NUMBER(src, "integer not followed by e or end of number"); }
      digit_count = p - start_digits;
      exponent = 0;

    } else {
      //
      // Pure integer (123456)
      //
      // Check if the integer might have overflowed:
      // - The longest negative 64-bit number is 19 digits.
      // - The longest positive 64-bit number is 20 digits.
      // - Negative 64-bit numbers will start with "-".
      // - So we check if (current position - start of string) == 20.
      digit_count = p - (const char *)src;
      if (digit_count > 20) { return INVALID_NUMBER(src, "more than 20 digits in integer"); }
      if (digit_count == 20) {
        // Anything negative above INT64_MAX is either invalid or INT64_MIN.
        if (negative && i > uint64_t(INT64_MAX)) {
          // If the number is negative and can't fit in a signed integer, it's invalid.
          if (i > uint64_t(INT64_MAX)+1) { return INVALID_NUMBER(src, "negative integer overflow"); }

          // If it's negative, it has to be INT64_MAX+1 now (or INT64_MIN).
          // C++ can't reliably negate uint64_t INT64_MIN, it seems. Special case it.
          WRITE_INTEGER(INT64_MIN, src, writer);
          return is_structural_or_whitespace(*p);
        }

        //
        // Positive overflow check:
        // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
        //   biggest uint64_t.
        // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
        //   If we got here, it's a 20 digit number starting with the digit "1".
        // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
        //   than 1,553,255,926,290,448,384.
        // - That is smaller than the smallest possible 20-digit number the user could write:
        //   10,000,000,000,000,000,000.
        // - Therefore, if the number is positive and lower than that, it's overflow.
        // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
        //
        if (!negative && (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX))) { return INVALID_NUMBER(src, "positive number overflow"); }
      }

      // Write unsigned if it doesn't fit in a signed integer.
      if (i > uint64_t(INT64_MAX)) {
        WRITE_UNSIGNED(i, src, writer);
      } else {
        WRITE_INTEGER(negative ? 0 - i : i, src, writer);
      }
      return true;
    } // 123456
  } // 123456 or 123.456

  //
  // Handle the exponent part, if any
  //
  if (('e' == *p) || ('E' == *p)) {
    p++;
    if (!parse_exponent(src, p, exponent)) { return false; }
  }
  return write_float((const char *)src, negative, i, exponent, digit_count, writer);

#endif // SIMDJSON_SKIPNUMBERPARSING
}

} // namespace numberparsing
} // namespace stage2
