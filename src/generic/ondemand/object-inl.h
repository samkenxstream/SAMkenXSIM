namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

//
// ### Live States
//
// While iterating or looking up values, depth >= iter.depth. at_start may vary. Error is
// always SUCCESS:
//
// - Start: This is the state when the object is first found and the iterator is just past the {.
//   In this state, at_start == true.
// - Next: After we hand a scalar value to the user, or an array/object which they then fully
//   iterate over, the iterator is at the , or } before the next value. In this state,
//   depth == iter.depth, at_start == false, and error == SUCCESS.
// - Unfinished Business: When we hand an array/object to the user which they do not fully
//   iterate over, we need to finish that iteration by skipping child values until we reach the
//   Next state. In this state, depth > iter.depth, at_start == false, and error == SUCCESS.
//
// ## Error States
// 
// In error states, we will yield exactly one more value before stopping. iter.depth == depth
// and at_start is always false. We decrement after yielding the error, moving to the Finished
// state.
//
// - Chained Error: When the object iterator is part of an error chain--for example, in
//   `for (auto tweet : doc["tweets"])`, where the tweet field may be missing or not be an
//   object--we yield that error in the loop, exactly once. In this state, error != SUCCESS and
//   iter.depth == depth, and at_start == false. We decrement depth when we yield the error.
// - Missing Comma Error: When the iterator ++ method discovers there is no comma between fields,
//   we flag that as an error and treat it exactly the same as a Chained Error. In this state,
//   error == TAPE_ERROR, iter.depth == depth, and at_start == false.
//
// Errors that occur while reading a field to give to the user (such as when the key is not a
// string or the field is missing a colon) are yielded immediately. Depth is then decremented,
// moving to the Finished state without transitioning through an Error state at all.
//
// ## Terminal State
//
// The terminal state has iter.depth < depth. at_start is always false.
//
// - Finished: When we have reached a }, we are finished. We signal this by decrementing depth.
//   In this state, iter.depth < depth, at_start == false, and error == SUCCESS.
//

simdjson_really_inline object::object() noexcept = default;
simdjson_really_inline object::object(json_iterator &_parent_iter) noexcept
  : iter{_parent_iter}, at_start{true}, error{SUCCESS}
{
}
simdjson_really_inline object::object(object &&other) noexcept = default;
simdjson_really_inline object &object::operator=(object &&other) noexcept = default;

simdjson_really_inline object::~object() noexcept {
  if (iter.active()) {
    logger::log_event(iter, "unfinished object");
    iter.skip_container();
    iter.release();
  }
}

simdjson_really_inline error_code object::check_has_next() noexcept {
  bool has_next;
  SIMDJSON_TRY( iter.has_next_field().get(has_next) );
  // If it's empty, we're done with the iterator; give it back to the parent.
  if (!has_next) {
    iter.release();
  }
  return SUCCESS;
}

simdjson_really_inline simdjson_result<value> object::operator[](const std::string_view key) noexcept {
  if (error) { return yield_error(); }

  // Unless this is the first field, we need to advance past the , and check for }
  if (at_start) {
    at_start = false;
  } else {
    if (!iter.active()) { return NO_SUCH_FIELD; }
    if ((error = check_has_next() )) { return yield_error(); };
  }
  while (iter.active()) {
    // Get the key
    raw_json_string actual_key;
    if ((error = iter.field_key().get(actual_key) )) { return yield_error(); };
    if ((error = iter.field_value() )) { return yield_error(); }

    // Check if it matches
    if (actual_key == key) {
      logger::log_event(iter, "match", key, -2);
      return value::start(iter);
    }
    logger::log_event(iter, "no match", key, -2);
    iter.skip(); // Skip the value entirely
    if ((error = check_has_next() )) { return yield_error(); };
  }

  // If the loop ended, we're out of fields to look at.
  return NO_SUCH_FIELD;
}

simdjson_really_inline simdjson_result<object> object::start(json_iterator &parent_iter) noexcept {
  bool has_value;
  SIMDJSON_TRY( parent_iter.start_object().get(has_value) );
  return started(parent_iter);
}
simdjson_really_inline object object::started(json_iterator &parent_iter) noexcept {
  if (parent_iter.started_object()) {
    return object(parent_iter);
  } else {
    return object();
  }
}
simdjson_really_inline object::iterator object::begin() noexcept {
  return *this;
}
simdjson_really_inline object::iterator object::end() noexcept {
  return *this;
}

simdjson_really_inline error_code object::yield_error() noexcept {
  error_code result = error;
  error = SUCCESS;
  // Abandon the iterator without giving it back: no one else shall iterate this, either!
  iter.abandon();
  return result;
}

//
// object::iterator
//

simdjson_really_inline object::iterator::iterator(object &_o) noexcept : o{&_o} {}

simdjson_really_inline object::iterator::iterator() noexcept = default;
simdjson_really_inline object::iterator::iterator(const object::iterator &_o) noexcept = default;
simdjson_really_inline object::iterator &object::iterator::operator=(const object::iterator &_o) noexcept = default;

simdjson_really_inline simdjson_result<field> object::iterator::operator*() noexcept {
  if (o->error) { return o->yield_error(); }
  if (o->at_start) { o->at_start = false; }
  return field::start(o->iter);
}
simdjson_really_inline bool object::iterator::operator==(const object::iterator &other) noexcept {
  return !(*this != other);
}
simdjson_really_inline bool object::iterator::operator!=(const object::iterator &) noexcept {
  return o->error || o->iter.active();
}
simdjson_really_inline object::iterator &object::iterator::operator++() noexcept {
  if (o->error) { return *this; }
  o->error = o->check_has_next();
  return *this;
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace {

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object>::simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::object &&value) noexcept
    : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::object>(value)) {}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object>::simdjson_result(error_code error) noexcept
    : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object>(error) {}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object>::begin() noexcept {
  if (error()) { return error(); }
  return SIMDJSON_IMPLEMENTATION::ondemand::object::iterator(first);
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object>::end() noexcept {
  if (error()) { return error(); }
  return {};
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object>::operator[](std::string_view key) noexcept {
  if (error()) { return error(); }
  return first[key];
}

//
// object::iterator
//

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>::simdjson_result() noexcept
  : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>({}, SUCCESS)
{
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::object::iterator &&value
) noexcept
  : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>(value))
{
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>::simdjson_result(error_code error) noexcept
  : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>({}, error)
{
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>::simdjson_result(
  const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator> &o
) noexcept
  : internal::simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>(o)
{
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator> &simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>::operator=(
  const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator> &o
) noexcept {
  first = o.first;
  second = o.second;
  return *this;
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::field> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>::operator*() noexcept {
  if (error()) { return error(); }
  return *first;
}
// Assumes it's being compared with the end. true if depth < iter.depth.
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>::operator==(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator> &other) noexcept {
  if (error()) { return true; }
  return first == other.first;
}
// Assumes it's being compared with the end. true if depth >= iter.depth.
simdjson_really_inline bool simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>::operator!=(const simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator> &other) noexcept {
  if (error()) { return false; }
  return first != other.first;
}
// Checks for ']' and ','
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator> &simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object::iterator>::operator++() noexcept {
  if (error()) { return *this; }
  ++first;
  return *this;
}

} // namespace simdjson
