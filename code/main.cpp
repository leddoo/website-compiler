
#include "util.hpp"
#include "parser.hpp"
#include "analyzer.hpp"
#include "codegen.hpp"
#include "context.hpp"
#include "deploy.hpp"

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

    if(!parse_arguments(argument_count, arguments)) {
        return 1;
    }

    if(!read_sources()) {
        return 1;
    }

    for(Usize i = 0; i < context.sources.count; i += 1) {
        if(!parse(context.sources[i].content)) {
            return 1;
        }
    }

    if(!analyze()) {
        return 1;
    }

    codegen();

    if(!deploy()) {
        return 1;
    }

    printf("Done.\n");
    return 0;
}

