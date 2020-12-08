#include "deploy.hpp"
#include "context.hpp"

#include "stdio.h"

bool deploy() {

    // NOTE(llw): Write output files.
    for(Usize i = 0; i < context.outputs.count; i += 1) {
        auto source = context.outputs[i];
        auto path = (char *)context.string_table[source.file_path].values;

        if(!write_entire_file(path, source.content)) {
            printf("Error: Could not write file '%s'\n", path);
            return false;
        }
    }

    return true;
}

