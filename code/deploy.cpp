#include "deploy.hpp"
#include "context.hpp"

#include "stdio.h"

bool deploy() {

    // NOTE(llw): Copy referenced files.
    for(Usize i = 0; i < context.referenced_files.count; i += 1) {
        auto name = context.referenced_files.entries[i].key;
        auto name_string = context.string_table[name];

        auto path = find_first_file(context.include_paths, name_string);
        if(path == 0) {
            printf("Error: Could not find referenced file '%s'.\n", name_string.values);
            return false;
        }

        auto path_string = (const char *)context.string_table[path].values;

        TEMP_SCOPE(context.temporary);
        auto buffer = create_array<U8>(context.temporary);
        if(!read_entire_file(path_string, buffer)) {
            printf("Error: Could not read file '%s'.\n", path_string);
            return false;
        }

        auto out_path = create_array<U8>(context.temporary);
        push(out_path, context.output_prefix);
        push(out_path, name);
        push(out_path, (U8)0);

        auto out_string = (const char *)out_path.values;
        if(!write_entire_file(out_string, buffer)) {
            printf("Error: Could not write file '%s'.\n", out_string);
            return false;
        }
    }

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

