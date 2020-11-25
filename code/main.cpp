
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


bool check_ids(const Array<Expression> &expressions) {
    TEMP_SCOPE(context.temporary);

    auto set = create_map<Interned_String, int>(context.temporary);

    for(Usize i = 0; i < expressions.count; i += 1) {
        auto expr = expressions[i];

        auto id = get_pointer(expr.arguments, context.strings.id);
        if(id) {
            if(id->type != ARG_STRING) {
                printf("Id is not a string.\n");
                return false;
            }

            if(!insert_maybe(set, id->value, 0)) {
                printf("Id used multiple times.\n");
                return false;
            }
        }
    }

    return true;
}

_inline bool is_arg_type(Argument *arg, Argument_Type t) {
    return arg != NULL && arg->type == t;
}
_inline bool is_atom  (Argument *arg) { return is_arg_type(arg, ARG_ATOM); }
_inline bool is_string(Argument *arg) { return is_arg_type(arg, ARG_STRING); }
_inline bool is_number(Argument *arg) { return is_arg_type(arg, ARG_NUMBER); }
_inline bool is_block (Argument *arg) { return is_arg_type(arg, ARG_BLOCK); }
_inline bool is_list  (Argument *arg) { return is_arg_type(arg, ARG_LIST); }

bool analyze(Expression &expr) {
    auto &args = expr.arguments;

    auto has_lid = has(args, context.strings.id);
    auto has_gid = has(args, context.strings.global_id);
    auto has_id = has_lid | has_gid;
    if(has_lid && has_gid) {
        printf("Cannot have both local and global id.\n");
        return false;
    }

    if(expr.type == context.strings.def_page) {
        if(expr.parent != 0) {
            printf("Page definitions cannot be nested.\n");
            return false;
        }

        auto name = get_pointer(args, context.strings.name);
        auto body = get_pointer(args, context.strings.body);
        if(    !is_string(name)
            || !is_block(body)
        ) {
            printf("Invalid def_page expression.\n");
            return false;
        }

        auto &children = body->block;

        if(!check_ids(children)) {
            return false;
        }

        for(Usize i = 0; i < children.count; i += 1) {
            if(!analyze(children[i])) {
                return false;
            }
        }

        return true;
    }
    else if(expr.type == context.strings.text) {
        auto type  = get_pointer(args, context.strings.type);
        auto value = get_pointer(args, context.strings.value);
        if(    !is_string(type)
            || !is_string(value)
        ) {
            printf("Invalid text expression.\n");
            return false;
        }

        if(!has(context.valid_text_types, type->value)) {
            printf("Invalid text type.\n");
            return false;
        }

        return true;
    }
    else if(expr.type == context.strings.spacer) {
        if(has_id) {
            printf("Spacers can't have ids.\n");
            return false;
        }

        auto value = get_pointer(args, context.strings.value);
        if(is_number(value)) {
            expr.type = context.strings.spacer_same;
            return true;
        }

        auto desktop = get_pointer(args, context.strings.desktop);
        auto mobile  = get_pointer(args, context.strings.mobile);
        if(    is_number(desktop)
            && is_number(mobile)
        ) {
            expr.type = context.strings.spacer_desktop_mobile;
            return true;
        }

        printf("Invalid spacer expression.\n");
        return false;
    }
    else {
        auto type = context.string_table[expr.type];
        printf("Unrecognized expression type: '%.*s'\n",
            (int)type.size, type.values
        );

        return false;
    }
}

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

    if(expr.type == context.strings.def_page) {
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
    else if(expr.type == context.strings.spacer_same) {
        auto value = context.string_table[args[context.strings.value].value];

        printf("<div");
        printf(" style = \"spacer_%.*s\"",
            (int)value.size, value.values
        );
        printf("></div>\n");
    }
    else if(expr.type == context.strings.spacer_desktop_mobile) {
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

    // NOTE(llw): Analyze.
    for(Usize expr_index = 0;
        expr_index < context.expressions.count;
        expr_index += 1
    ) {
        auto expr = context.expressions[expr_index];
        if(!analyze(expr)) {
            exit(1);
        }
    }

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

    printf("Done.\n");
}

