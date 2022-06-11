#include <libcpp/memory/hash.hpp>

namespace libcpp {

    // NOTE(llw): Taken from
    // http://bitsquid.blogspot.com/2011/08/code-snippet-murmur-hash-inverse-pre.html.
    U64 murmur_hash_64(void *key, Usize size, U64 seed) {
        U64 m = 0xc6a4a7935bd1e995ULL;
        auto r = (U64)47;

        auto hash = seed ^ (size * m);

        auto data = (U64 *)key;
        auto end = data + (size/8);

        while(data != end)
        {
            auto k = *data++;

            k *= m;
            k ^= k >> r;
            k *= m;

            hash ^= k;
            hash *= m;
        }

        auto remainder = (U8 *)data;

        switch(size & 7)
        {
            case 7: hash ^= (U64)remainder[6] << 48;
            case 6: hash ^= (U64)remainder[5] << 40;
            case 5: hash ^= (U64)remainder[4] << 32;
            case 4: hash ^= (U64)remainder[3] << 24;
            case 3: hash ^= (U64)remainder[2] << 16;
            case 2: hash ^= (U64)remainder[1] << 8;
            case 1: hash ^= (U64)remainder[0];
                    hash *= m;
        };

        hash ^= hash >> r;
        hash *= m;
        hash ^= hash >> r;

        return hash;
    }

}

