#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <string>

#include "NullBuffer.h"


extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {

    // inspired by doc/basics.md#newline-delimited-json-ndjson-and-json-lines
    try {
        simdjson::dom::parser pj;
        auto docs=pj.parse_many(Data, Size);
        NulOStream os;
        for(simdjson::dom::element root: docs ) {
            os<<root;
        }
    } catch (...) {
    }
    return 0;
}
