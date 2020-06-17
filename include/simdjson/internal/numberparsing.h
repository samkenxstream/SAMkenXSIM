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

// Integers 19 digits or more: 10,000,000,000,000,000,000 to 18,446,744,073,709,551,615
simdjson_result<uint64_t> convert_large_unsigned(const uint8_t *buf, int digits, uint64_t magnitude) {
  log_event("  (large unsigned)", buf);
  assert(digits >= 19);
  if (digits > 19) {
    log_error("20+ digits", buf);
    return NUMBER_OUT_OF_RANGE;
  }
  if (!is_structural_or_whitespace(buf[digits])) {
    log_error("followed by non-ws/struct", buf);
    return NUMBER_OUT_OF_RANGE;
  }
  if (buf[0] != '1') {
    log_error("greater than 2e18", buf);
    return NUMBER_OUT_OF_RANGE;
  }

  // We have 19 digits and a leading 1.
  // 19,999,999,999,999,999,999 is the biggest number the user could have written.
  // 18,446,744,073,709,551,615 is the biggest number uint64_t could store.
  //  1,553,255,926,290,448,383 is the overflow of the biggest number we could store.
  // 10,000,000,000,000,000,000 is the smallest number the user could have written.
  // We assume that an overflow is lower than that.
  if (magnitude < 10000000000000000000ULL) {
    log_error("19-digit overflow", buf);
    return NUMBER_OUT_OF_RANGE;
  }
  return magnitude;
}

// Integers 18 digits or more: 1,000,000,000,000,000,000 to  9,223,372,036,854,775,807
//                        and -1,000,000,000,000,000,000 to -9,223,372,036,854,775,808
simdjson_result<int64_t> convert_large_integer(const uint8_t *buf, int digits, uint64_t magnitude, bool negative) {
  log_event("  (large integer)", buf);
  assert(digits >= 18);
  if (digits > 18) {
    log_error("19+ digits", buf);
    return NUMBER_OUT_OF_RANGE;
  }
  if (!is_structural_or_whitespace(buf[digits])) {
    log_error("followed by non-ws/struct", buf);
    return NUMBER_OUT_OF_RANGE;
  }

  // The number cannot have actually overflowed since it's stored in an unsigned integer;
  // we just have to check whether it's bigger than INT64_MAX

  // C++ can't reliably negate uint64_t INT64_MIN, it seems
  if (negative && magnitude == (uint64_t(INT64_MAX)+1)) {
    log_event("  (INT64_MIN)", buf);
    return INT64_MIN;
  }
  if (magnitude > uint64_t(INT64_MAX)) {
    log_error("18-digit overflow", buf);
    return NUMBER_OUT_OF_RANGE;
  }
  return negative ? -static_cast<int64_t>(magnitude) : static_cast<int64_t>(magnitude);

}

} // namespace {}

// Parse any number from 0 to 18,446,744,073,709,551,615
really_inline simdjson_result<uint64_t> parse_unsigned(const uint8_t * const buf) noexcept {
  // Parse the first digit
  uint64_t magnitude = buf[0] - '0';
  int digits = 1;
  if (magnitude > 0) { // 0 cannot be followed by other digits
    if (magnitude > 9) { // First thing is not a digit at all
      log_error("non-digit start", buf);
      return INCORRECT_TYPE;
    }

    // Parse remaining digits
    while (1) {
      uint8_t digit = static_cast<uint8_t>(buf[digits] - '0');
      if (digit > 9) { break; }
      magnitude = magnitude * 10 + digit;
      digits++;
    }

    // Check for massive numbers
    if (unlikely(digits >= 19)) {
      return convert_large_unsigned(buf, digits, magnitude);
    }
  }

  // Next character can't be . or e--it must be whitespace, comma, end array or end bracket
  if (!is_structural_or_whitespace(buf[digits])) {
    log_error("followed by non-ws/struct", buf);
    return INCORRECT_TYPE;
  }
  return magnitude;
}

// Parse any number from  -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
really_inline simdjson_result<int64_t> parse_integer(const uint8_t * buf) noexcept {
  bool negative = (buf[0] == '-');
  if (negative) { buf++; }

  // Parse the first digit
  uint64_t magnitude = buf[0] - '0';
  int digits = 1;
  if (magnitude > 0) { // 0 cannot be followed by other digits
    if (magnitude > 9) { // First thing is not a digit at all
      log_error("non-digit start", buf);
      return INCORRECT_TYPE;
    }

    // Parse remaining digits
    while (1) {
      uint8_t digit = static_cast<uint8_t>(buf[digits] - '0');
      if (digit > 9) { break; }
      magnitude = magnitude * 10 + digit;
      digits++;
    }

    // Check for massive numbers
    if (unlikely(digits >= 19)) {
      return convert_large_integer(buf, digits, magnitude, negative);
    }
  }

  // Next character can't be . or e--it must be whitespace, comma, end array or end bracket
  if (!is_structural_or_whitespace(buf[digits])) {
    log_error("followed by non-ws/struct", buf);
    return INCORRECT_TYPE;
  }
  return magnitude;
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