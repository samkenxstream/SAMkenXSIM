namespace stage1 {

/**
 * State stored in the parser for stage 1
 */
struct parser_state {
  /** Allocate anything that needs allocating */
  really_inline error_code allocate_stage1(parser &parser, size_t capacity, size_t max_depth);
}; // struct parser_state

really_inline error_code parser_state::allocate_stage1(parser &parser, size_t capacity, UNUSED size_t max_depth) {
  if (capacity == 0) {
    parser.structural_indexes.reset();
    return SUCCESS;
  }

  size_t max_structures = ROUNDUP_N(capacity, 64) + 2 + 7;
  parser.structural_indexes.reset( new (std::nothrow) uint32_t[max_structures] ); // TODO realloc
  if (!parser.structural_indexes) {
    return MEMALLOC;
  }

  return SUCCESS;
}

} // namespace stage1
