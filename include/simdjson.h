#ifndef SIMDJSON_H
#define SIMDJSON_H

/**
 * @mainpage
 *
 * Check the [README.md](https://github.com/lemire/simdjson/blob/master/README.md#simdjson--parsing-gigabytes-of-json-per-second).
 */

#include "simdjson/compiler_check.h"
#include "simdjson/common_defs.h"

SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_UNDESIRED_WARNINGS

// Public API
#include "simdjson/simdjson_version.h"
#include "simdjson/error.h"
#include "simdjson/padded_string.h"
#include "simdjson/implementation.h"
#include "simdjson/dom/array.h"
#include "simdjson/dom/document_stream.h"
#include "simdjson/dom/document.h"
#include "simdjson/dom/element.h"
#include "simdjson/dom/object.h"
#include "simdjson/dom/parser.h"
#include "simdjson/stream/array.h"
#include "simdjson/stream/document.h"
#include "simdjson/stream/documents.h"
#include "simdjson/stream/element.h"
#include "simdjson/stream/field.h"
#include "simdjson/stream/object.h"
#include "simdjson/stream/raw_json_string.h"

// Deprecated API
#include "simdjson/dom/jsonparser.h"
#include "simdjson/dom/parsedjson.h"
#include "simdjson/dom/parsedjson_iterator.h"

// Inline functions
#include "simdjson/inline/array.h"
#include "simdjson/inline/document_stream.h"
#include "simdjson/inline/document.h"
#include "simdjson/inline/element.h"
#include "simdjson/inline/error.h"
#include "simdjson/inline/object.h"
#include "simdjson/inline/padded_string.h"
#include "simdjson/inline/parsedjson_iterator.h"
#include "simdjson/inline/parser.h"
#include "simdjson/inline/tape_ref.h"
#include "simdjson/stream/array-inl.h"
#include "simdjson/stream/document-inl.h"
#include "simdjson/stream/documents-inl.h"
#include "simdjson/stream/element-inl.h"
#include "simdjson/stream/object-inl.h"

SIMDJSON_POP_DISABLE_WARNINGS

#endif // SIMDJSON_H
