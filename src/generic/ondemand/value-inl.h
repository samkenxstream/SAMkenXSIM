namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline value::value() noexcept = default;
simdjson_really_inline value::value(json_iterator_ref &&_iter) noexcept
  : iter{std::forward<json_iterator_ref>(_iter)},
    json{iter->advance()}
{
}
simdjson_really_inline value::value(value &&other) noexcept = default;
simdjson_really_inline value &value::operator=(value &&other) noexcept = default;

simdjson_really_inline value::~value() noexcept {
  if (iter.is_active()) {
    skip();
  }
}

simdjson_really_inline void value::skip() noexcept {
  switch (*json) {
    case '[': case '{':
      logger::log_start_value(*iter, "unused");
      iter->skip_container();
      break;
    default:
      logger::log_value(*iter, "unused");
      break;
  }
  iter.release();
}

simdjson_really_inline simdjson_result<array> value::get_array() noexcept {
  SIMDJSON_ASSUME(iter.is_active());
  if (*json != '[') {
    log_error("not an array");
    return INCORRECT_TYPE;
  }
  return array::started(std::move(iter));
}
simdjson_really_inline simdjson_result<object> value::get_object() noexcept {
  SIMDJSON_ASSUME(iter.is_active());
  if (*json != '{') {
    log_error("not an object");
    return INCORRECT_TYPE;
  }
  return object::started(std::move(iter));
}
simdjson_really_inline simdjson_result<raw_json_string> value::get_raw_json_string() noexcept {
  SIMDJSON_ASSUME(iter.is_active());
  log_value("string");
  if (*json != '"') { log_error("not a string"); return INCORRECT_TYPE; }
  return raw_json_string{&json[1]};
}
simdjson_really_inline simdjson_result<std::string_view> value::get_string() noexcept {
  SIMDJSON_ASSUME(iter.is_active());
  log_value("string");
  if (*json != '"') { log_error("not a string"); return INCORRECT_TYPE; }
  return raw_json_string{&json[1]}.unescape(*(iter->parser));
}
simdjson_really_inline simdjson_result<double> value::get_double() noexcept {
  SIMDJSON_ASSUME(iter.is_active());
  log_value("double");
  double result;
  error_code error;
  if ((error = stage2::numberparsing::parse_double(json).get(result))) { log_error("not a double"); return error; }
  return result;
}
simdjson_really_inline simdjson_result<uint64_t> value::get_uint64() noexcept {
  SIMDJSON_ASSUME(iter.is_active());
  log_value("unsigned");
  uint64_t result;
  error_code error;
  if ((error = stage2::numberparsing::parse_unsigned(json).get(result))) { log_error("not a unsigned integer"); return error; }
  return result;
}
simdjson_really_inline simdjson_result<int64_t> value::get_int64() noexcept {
  SIMDJSON_ASSUME(iter.is_active());
  log_value("integer");
  int64_t result;
  error_code error;
  if ((error = stage2::numberparsing::parse_integer(json).get(result))) { log_error("not an integer"); return error; }
  return result;
}
simdjson_really_inline simdjson_result<bool> value::get_bool() noexcept {
  SIMDJSON_ASSUME(iter.is_active());
  log_value("bool");
  auto not_true = stage2::atomparsing::str4ncmp(json, "true");
  auto not_false = stage2::atomparsing::str4ncmp(json, "fals") | (json[4] ^ 'e');
  bool error = (not_true && not_false) || stage2::is_not_structural_or_whitespace(json[not_true ? 5 : 4]);
  if (error) { log_error("not a boolean"); return INCORRECT_TYPE; }
  return simdjson_result<bool>(!not_true, error ? INCORRECT_TYPE : SUCCESS);
}
simdjson_really_inline bool value::is_null() noexcept {
  SIMDJSON_ASSUME(iter.is_active());
  log_value("null");
  if (stage2::atomparsing::str4ncmp(json, "null")) { return false; }
  return true;
}

#if SIMDJSON_EXCEPTIONS
simdjson_really_inline value::operator array() noexcept(false) { return get_array(); }
simdjson_really_inline value::operator object() noexcept(false) { return get_object(); }
simdjson_really_inline value::operator uint64_t() noexcept(false) { return get_uint64(); }
simdjson_really_inline value::operator int64_t() noexcept(false) { return get_int64(); }
simdjson_really_inline value::operator double() noexcept(false) { return get_double(); }
simdjson_really_inline value::operator std::string_view() noexcept(false) { return get_string(); }
simdjson_really_inline value::operator raw_json_string() noexcept(false) { return get_raw_json_string(); }
simdjson_really_inline value::operator bool() noexcept(false) { return get_bool(); }
#endif

simdjson_really_inline simdjson_result<array::iterator> value::begin() noexcept { return get_array().begin(); }
simdjson_really_inline simdjson_result<array::iterator> value::end() noexcept { return {}; }
// TODO this CANNOT be reused. Each time you try, it will get you a new object.
// Probably make it move-only to avoid this issue.
simdjson_really_inline simdjson_result<value> value::operator[](std::string_view key) noexcept {
  return get_object()[key];
}
simdjson_really_inline simdjson_result<value> value::operator[](const char *key) noexcept {
  return get_object()[key];
}

simdjson_really_inline void value::log_value(const char *type) const noexcept {
  logger::log_value(*iter, type);
}
simdjson_really_inline void value::log_error(const char *message) const noexcept {
  logger::log_error(*iter, message);
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::value &&value
) noexcept :
    internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::value>(
      std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(value)
    )
{
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::value &&value,
  error_code error
) noexcept :
    internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::value>(
      std::forward<SIMDJSON_IMPLEMENTATION::ondemand::value>(value),
      error
    )
{
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::simdjson_result(
  error_code error
) noexcept :
    internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::value>(error)
{
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::begin() noexcept {
  if (error()) { return error(); }
  return std::move(first.begin());
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array::iterator> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::end() noexcept {
  if (error()) { return error(); }
  return std::move(first.end());
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator[](std::string_view key) noexcept {
  if (error()) { return error(); }
  return first[key];
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator[](const char *key) noexcept {
  if (error()) { return error(); }
  return first[key];
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_array() noexcept {
  if (error()) { return error(); }
  return first.get_array();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_object() noexcept {
  if (error()) { return error(); }
  return first.get_object();
}
simdjson_really_inline simdjson_result<uint64_t> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_uint64() noexcept {
  if (error()) { return error(); }
  return first.get_uint64();
}
simdjson_really_inline simdjson_result<int64_t> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_int64() noexcept {
  if (error()) { return error(); }
  return first.get_int64();
}
simdjson_really_inline simdjson_result<double> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_double() noexcept {
  if (error()) { return error(); }
  return first.get_double();
}
simdjson_really_inline simdjson_result<std::string_view> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_string() noexcept {
  if (error()) { return error(); }
  return first.get_string();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_raw_json_string() noexcept {
  if (error()) { return error(); }
  return first.get_raw_json_string();
}
simdjson_really_inline simdjson_result<bool> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::get_bool() noexcept {
  if (error()) { return error(); }
  return first.get_bool();
}
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::is_null() noexcept {
  if (error()) { return false; }
  return first.is_null();
}

#if SIMDJSON_EXCEPTIONS
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator SIMDJSON_IMPLEMENTATION::ondemand::array() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator SIMDJSON_IMPLEMENTATION::ondemand::object() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator uint64_t() noexcept(false)  {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator int64_t() noexcept(false)  {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator double() noexcept(false)  {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator std::string_view() noexcept(false)  {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator SIMDJSON_IMPLEMENTATION::ondemand::raw_json_string() noexcept(false)  {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value>::operator bool() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
#endif

} // namespace simdjson
