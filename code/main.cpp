
#include "util.hpp"
#include "parser.hpp"

#include <libcpp/memory/arena.hpp>
#include <libcpp/memory/array.hpp>
#include <libcpp/memory/map.hpp>
#include <libcpp/util/assert.hpp>
#include <libcpp/util/defer.hpp>
#include <libcpp/util/math.hpp>

using namespace libcpp;

#include "cstdio"
#include "cstdlib"


int main(int argument_count, const char **arguments) {

    auto arena = create_arena();

    if(argument_count < 2) {
        printf("Usage: %s sources", arguments[0]);
        return 0;
    }

    auto context = create_parse_context(arena);

    for(int i = 1; i < argument_count; i += 1) {
        auto path = arguments[i];

        auto buffer = create_array<U8>(arena);
        if(!read_entire_file(path, buffer)) {
            printf("Error reading file %s\n", path);
        }

        if(!parse(context, buffer)) {
            exit(1);
        }
    }

    printf("Done.\n");
}



