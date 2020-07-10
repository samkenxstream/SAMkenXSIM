#ifndef SIMDJSON_INTERNAL_NUMBERPARSING_H
#define SIMDJSON_INTERNAL_NUMBERPARSING_H

#include "simdjson/common_defs.h"
#include "simdjson/internal/jsoncharutils.h"
#include "simdjson/internal/logger.h"

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

// really_inline simdjson_result<double> parse_double(const uint8_t * buf) noexcept {
//   //
//   // Negative
//   //
//   //    -123.456e-78
//   //    ^
//   //
//   bool negative = (buf[0] == '-');
//   if (negative) { buf++; }

//   //
//   // Magnitude
//   //
//   //    -123.456e-78
//   //     ^^^
//   //

//   // Parse the first digit
//   uint64_t magnitude = buf[0] - '0';
//   int digits = 1;
//   if (magnitude > 0) { // 0 cannot be followed by other digits
//     if (magnitude > 9) { return INCORRECT_TYPE; } // First thing is not a digit

//     // Parse remaining digits
//     while (1) {
//       uint8_t digit = static_cast<uint8_t>(buf[digits] - '0');
//       if (digit > 9) { break; }
//       magnitude = magnitude * 10 + digit;
//       digits++;
//     }
//   }

//   //
//   // Decimal Magnitude
//   //
//   //    -123.456e-78
//   //        ^^^^
//   //
//   if (buf[digits] != '.') {
//     if (buf[digits] == 'e') {
//       return parse_double_exp(buf, digits, digits, magnitude, negative);
//     }
//   }

//   int decimal_point = digits;
//   // Parse remaining decimal point digits
//   if (buf[digits] == 'e') {
//     return parse_double_exp(buf, digits, digits, magnitude, negative);
//   }
//     decimal_point
//     // Parse the rest of the digits after the decimal
//     digits++;
//     while (1) {
//       uint8_t digit = buf[digits] - '0';
//       if (digit > 9) { break; }
//       magnitude = magnitude * 10 + digit;
//       digits++;
//     }
//   }

//   //
//   // Exponent
//   //
//   //    -123.456e-78
//   //            ^^^^
//   //
//   if (buf[digits] == 'e') {
//     int exp_digits = digits+1;
//     bool exp_negative = buf[exp_digits] == '-';
//     if (exp_negative) {
//       exp_digits++;
//     }
//     if (exp_digits) {

//     }

//     if () {
//       exp_digits++;
//     }
//     bool negative_exp = (buf[digits+1] == '-');
//     uint16_t exp = buf[exp_signifier+]
//     if (buf[1] == )
    
//   }

//   uint64_t  = buf[0] - '0';
//   if (result > 0) {
//     if (result > 9) { return INCORRECT_TYPE; } // First thing is not a digit

//     int digits = 1;
//     while (1) {
//       uint8_t digit = buf[digits] - '0';
//       if (digit > 9) { break; }
//       result = result * 10 + digit;
//       digits++;
//     }
//     if (digits >= 19) { return OUT_OF_RANGE; } // TODO add parse_large_unsigned
//   }
//   if (!is_structural_or_whitespace(buf[digits])) {
//     return INCORRECT_TYPE;
//   }
//   return result;
// }

} // internal
} // simdjson

#endif // SIMDJSON_INTERNAL_NUMBERPARSING_H