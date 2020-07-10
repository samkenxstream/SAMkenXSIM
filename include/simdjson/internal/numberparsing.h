#ifndef SIMDJSON_INTERNAL_NUMBERPARSING_H
#define SIMDJSON_INTERNAL_NUMBERPARSING_H

#include "simdjson/common_defs.h"
#include "simdjson/internal/jsoncharutils.h"
#include "simdjson/internal/logger.h"
#include "simdjson/internal/compute_float_64.h"
#include <cmath>

namespace simdjson {
namespace internal {

really_inline simdjson_result<uint64_t> parse_unsigned(const uint8_t *buf) noexcept;
really_inline simdjson_result<int64_t> parse_integer(const uint8_t *buf) noexcept;
really_inline simdjson_result<double> parse_double(const uint8_t *buf) noexcept;

namespace {

using namespace internal::logger;

template<typename I>
NO_SANITIZE_UNDEFINED // We deliberately allow overflow here and check later
really_inline bool parse_digit(const uint8_t c, I &i) {
  const uint8_t digit = static_cast<uint8_t>(c - '0');
  if (digit > 9) {
    return false;
  }
  // PERF NOTE: multiplication by 10 is cheaper than arbitrary integer multiplication
  i = 10 * i + digit; // might overflow, we will handle the overflow later
  return true;
}

static really_inline simdjson_result<double> parse_float_strtod(const uint8_t *ptr) {
  char *endptr;
  double d = strtod((const char *)ptr, &endptr);
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
  if (endptr == (const char *)ptr || !std::isfinite(d)) {
    return NUMBER_ERROR;
  }
  return d;
}

}; // namespace {}

// Parse any number from 0 to 18,446,744,073,709,551,615
really_inline simdjson_result<uint64_t> parse_unsigned(const uint8_t * const src) noexcept {
  //
  // Parse the integer part.
  //
  uint64_t i = 0;
  const uint8_t *p = src;
  p += parse_digit(*p, i);
  bool leading_zero = (i == 0);
  while (parse_digit(*p, i)) { p++; }

  //
  // Check for errors
  //
  auto digit_count = src - p;
  if ( !is_structural_or_whitespace(*p) || // . or e
        digit_count == 0                || // no digits
       (leading_zero && digit_count != 1)  // 0123 (zero must be solo)
  ) {
    return NUMBER_ERROR;
  }
  // Overflow checks
  if (digit_count > 20) { return NUMBER_OUT_OF_RANGE; }
  if (digit_count == 20) {
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
    if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return NUMBER_OUT_OF_RANGE; }
  }

  //
  // Return the number.
  //
  return i;
}

// Parse any number from  -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
really_inline simdjson_result<int64_t> parse_integer(const uint8_t *src) noexcept {
  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  src += negative;

  //
  // Parse the integer part.
  //
  uint64_t i = 0;
  const uint8_t *p = src;
  p += parse_digit(*p, i);
  bool leading_zero = (i == 0);
  while (parse_digit(*p, i)) { p++; }

  //
  // Check for errors
  //
  auto digit_count = p - src;
  if ( !is_structural_or_whitespace(*p) || // . or e
        digit_count == 0                || // no digits
       (leading_zero && digit_count != 1)  // 0123 (zero must be solo)
  ) {
    return NUMBER_ERROR;
  }
  // Overflow checks
  if (digit_count > 19) { return NUMBER_OUT_OF_RANGE; }
  if (digit_count == 19) {
    // C++ can't reliably negate uint64_t INT64_MIN, it seems. Special case it.
    if (negative && i == uint64_t(INT64_MAX)+1) { return INT64_MIN; }
    // Anything above INT64_MAX is either invalid or INT64_MIN.
    if (i > uint64_t(INT64_MAX)) { return NUMBER_OUT_OF_RANGE; }
  }

  //
  // Return the number.
  //
  return negative ? 0 - i : i;
}

really_inline simdjson_result<double> parse_double(const uint8_t * src) noexcept {
  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  src += negative;

  //
  // Parse the integer part.
  //
  uint64_t i = 0;
  const uint8_t *p = src;
  p += parse_digit(*p, i);
  bool leading_zero = (i == 0);
  while (parse_digit(*p, i)) { p++; }
  // no integer digits, or 0123 (zero must be solo)
  if ( p == src || (leading_zero && p != src+1)) { return NUMBER_ERROR; }

  //
  // Parse the decimal part.
  //
  int64_t exponent = 0;
  bool overflow;
  if (likely(*p == '.')) {
    p++;
    const uint8_t *start_decimal_digits = p;
    if (!parse_digit(*p, i)) { return NUMBER_ERROR; } // no decimal digits
    p++;
    while (parse_digit(*p, i)) { p++; }
    exponent = -(p - start_decimal_digits);

    // Overflow check. 19 digits (minus the decimal) may be overflow.
    overflow = p-src-1 >= 19;
    if (unlikely(overflow && leading_zero)) {
      // Skip leading 0.00000 and see if it still overflows
      const uint8_t *start_digits = src + 2;
      while (*start_digits == '0') { start_digits++; }
      overflow = start_digits-src >= 19;
    }
  } else {
    overflow = p-src >= 19;
  }

  //
  // Parse the exponent
  //
  if (*p == 'e' || *p == 'E') {
    p++;
    bool exp_neg = *p == '-';
    p += exp_neg || *p == '+';

    uint64_t exp = 0;
    const uint8_t *start_exp_digits = p;
    while (parse_digit(*p, exp)) { p++; }
    // no exp digits, or 20+ exp digits
    if (p-start_exp_digits == 0 || p-start_exp_digits > 19) { return NUMBER_ERROR; }

    exponent += exp_neg ? 0-exp : exp;
    overflow = overflow || exponent < FASTFLOAT_SMALLEST_POWER || exponent > FASTFLOAT_LARGEST_POWER;
  }

  //
  // Assemble (or slow-parse) the float
  //
  if (likely(!overflow)) {
    bool success = false;
    double d = compute_float_64(exponent, i, negative, &success);
    if (success) { return d; }
  }
  return parse_float_strtod(src-negative);
}

} // internal
} // simdjson

#endif // SIMDJSON_INTERNAL_NUMBERPARSING_H