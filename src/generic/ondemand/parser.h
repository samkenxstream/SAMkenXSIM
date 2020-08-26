#include "simdjson/error.h"

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

class document;
class json_iterator;

/**
 * A JSON fragment iterator.
 *
 * This holds the actual iterator as well as the buffer for writing strings.
 */
class parser {
public:
  simdjson_really_inline parser() noexcept = default;
  simdjson_really_inline parser(parser &&other) noexcept = default;
  simdjson_really_inline parser(const parser &other) = delete;
  simdjson_really_inline parser &operator=(const parser &other) = delete;

  SIMDJSON_WARN_UNUSED error_code allocate(size_t capacity, size_t max_depth=DEFAULT_MAX_DEPTH) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_result<document> iterate(const padded_string &json) noexcept;
  SIMDJSON_WARN_UNUSED simdjson_result<json_iterator> iterate_raw(const padded_string &json) noexcept;
private:
  dom_parser_implementation dom_parser{};
  size_t _capacity{0};
  size_t _max_depth{0};
  std::unique_ptr<uint8_t[]> string_buf{};
  uint8_t *current_string_buf_loc{};

  friend class raw_json_string;
  friend class json_iterator;
  friend class value;
};

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {
