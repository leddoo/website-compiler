
#include "util.hpp"
#include "parser.hpp"
#include "analyzer.hpp"
#include "codegen.hpp"
#include "context.hpp"

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

    setup_context();

    if(argument_count < 2) {
        printf("Usage: %s sources", arguments[0]);
        return 0;
    }

    for(int i = 1; i < argument_count; i += 1) {
        auto path = arguments[i];

        auto buffer = create_array<U8>(context.arena);
        if(!read_entire_file(path, buffer)) {
            printf("Error reading file %s\n", path);
        }

        if(!parse(buffer)) {
            exit(1);
        }
    }

    if(!analyze()) {
        exit(1);
    }

    codegen();

    printf("Done.\n");
}

