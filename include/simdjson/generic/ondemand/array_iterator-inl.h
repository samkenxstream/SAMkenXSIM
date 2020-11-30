namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline array_iterator::array_iterator(json_iterator *_iter, depth_t _depth) noexcept
  : iter{_iter}, depth{_depth}
{}

simdjson_really_inline simdjson_result<value> array_iterator::operator*() noexcept {
  if (iter->error()) { return iter->error(); }
  return iter;
}
simdjson_really_inline bool array_iterator::operator==(const array_iterator &other) noexcept {
  return !(*this != other);
}
simdjson_really_inline bool array_iterator::operator!=(const array_iterator &) noexcept {
  return iter->is_below(depth);
}
simdjson_really_inline array_iterator &array_iterator::operator++() noexcept {
  error_code error;
  // PERF NOTE this is a safety rail ... users should exit loops as soon as they receive an error, so we'll never get here.
  // However, it does not seem to make a perf difference, so we add it out of an abundance of caution.
  if ((error = iter->error()) ) { return *this; }
  if ((error = iter->finish_iterator_child() )) { return *this; }
  if ((error = iter->get_iterator().has_next_element().error() )) { return *this; }
  return *this;
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::array_iterator &&value
) noexcept
  : SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator>(value))
{
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator>::simdjson_result(error_code error) noexcept
  : SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator>({}, error)
{
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator>::operator*() noexcept {
  if (this->error()) { this->second = SUCCESS; return this->error(); }
  return *this->first;
}
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator>::operator==(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> &other) noexcept {
  if (this->error()) { return true; }
  return this->first == other.first;
}
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator>::operator!=(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> &other) noexcept {
  if (this->error()) { return false; }
  return this->first != other.first;
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> &simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator>::operator++() noexcept {
  if (this->error()) { return *this; }
  ++(this->first);
  return *this;
}

} // namespace simdjson
