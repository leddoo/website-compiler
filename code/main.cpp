
#include "util.hpp"
#include "parser.hpp"
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


void generate_html(const Expression &expr, Interned_String parent, Usize indent = 0) {
    auto do_indent = [&](Usize offset = 0) {
        for(Usize i = 0; i < indent + offset; i += 1) {
            printf("    ");
        }
    };

    const auto &args = expr.arguments;

    auto id = Interned_String {};
    {
        auto lid = get_pointer(args, context.strings.id);
        auto gid = get_pointer(args, context.strings.global_id);

        if(parent != 0 && lid != NULL) {
            TEMP_SCOPE(context.temporary);

            auto cat = create_array<U8>(context.temporary);
            push(cat, context.string_table[parent]);
            push(cat, (U8)'.');
            push(cat, context.string_table[lid->value]);

            id = intern(context.string_table, str(cat));
        }
        else if(gid != NULL) {
            id = gid->value;
        }
    }

    auto print_id = [&]() {
        if(id != 0) {
            auto s = context.string_table[id];
            printf(" id = \"%.*s\"", (int)s.size, s.values);
        }
    };


    do_indent();

    if(expr.type == context.strings.page) {
        auto id = args[context.strings.name].value;

        printf("<div");
        print_id();
        printf(">\n");

        const auto &children = args[context.strings.body].block;
        for(Usize i = 0; i < children.count; i += 1) {
            generate_html(children[i], id, indent + 1);
        }

        printf("</div>\n");
    }
    else if(expr.type == context.strings.text) {
        auto type  = context.string_table[args[context.strings.type].value];
        auto value = context.string_table[args[context.strings.value].value];

        printf("<%.*s", (int)type.size, type.values);
        print_id();
        printf(">\n");

        do_indent(1);
        printf("%.*s\n", (int)value.size, value.values);

        do_indent();
        printf("</%.*s>\n", (int)type.size, type.values);
    }
    else if(expr.type == context.strings.spacer) {
        if(has(args, context.strings.value)) {
            auto value = context.string_table[args[context.strings.value].value];

            printf("<div style = \"spacer_%.*s\"></div>\n",
                (int)value.size, value.values
            );
        }
        else {
            auto desktop = context.string_table[args[context.strings.desktop].value];
            auto mobile  = context.string_table[args[context.strings.mobile].value];

            printf("<div style = \"spacer_%.*s desktop\"></div>\n",
                (int)desktop.size, desktop.values
            );

            do_indent();
            printf("<div style = \"spacer_%.*s mobile\"></div>\n",
                (int)mobile.size, mobile.values
            );
        }
    }
    else {
        assert(false);
    }
}

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

    // code gen.


    #if 0

    // NOTE(llw): Generate html for pages.
    for(Usize expr_index = 0;
        expr_index < context.expressions.count;
        expr_index += 1
    ) {
        auto expr = context.expressions[expr_index];
        if(expr.type != context.strings.def_page) {
            continue;
        }

        generate_html(expr, 0);
    }


    // NOTE(llw): Generate js.
    #endif

    printf("Done.\n");
}

