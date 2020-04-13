#include "simdjson.h"
#include <cstddef>
#include <cstdint>
#include <string>

#include "NullBuffer.h"

// why is this not in <algorithm>?
template<class Iterator, class Element>
Iterator find_last(Iterator first, Iterator last, const Element& e) {
    Iterator ret=last;
    while(first!=ret) {
        --ret;
        if(*ret==e) {
            return ret;
        }
    }
    return last;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {

    // split the input in two, the first part will be used as json and
    // the second part as index to use for the json pointer

    const uint8_t sep='\t';
    const uint8_t* end=Data+Size;
    const uint8_t* mid = find_last(Data,end,sep);
    const size_t Size1=std::distance(Data,mid);
    if(Size1<Size) {
        ++mid;
    }
    const size_t Size2=std::distance(mid,end);

    // inspired by doc/basics.md#json-pointer
    try {
        simdjson::dom::parser pj;
        simdjson::dom::element root=pj.parse(Data, Size1);

        auto key=std::string_view((const char*)mid,Size2);
        NulOStream os;
        //std::ostream& os(std::cout);
        os<< root[key]<<'\n';
    } catch (...) {
    }
    return 0;
}
