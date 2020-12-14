#include "context.hpp"

#include <stdio.h>

Context context;

const char *argument_type_strings[ARG_LIST + 1] = {
    "atom",
    "string",
    "number",
    "block",
    "list",
};

void setup_context() {
    context = {};

    context.arena        = create_arena();
    context.temporary    = create_arena();
    context.string_table = create_string_table(context.arena);
    context.expressions  = { &context.arena };
    context.symbols      = create_map<Interned_String, Symbol>(context.arena);
    context.exports      = { &context.arena };

    context.sources       = { &context.arena };
    context.include_paths = { &context.arena };
    context.outputs       = { &context.arena };
    context.referenced_files.allocator = &context.arena;

    context.strings.empty_string = intern(context.string_table, STRING(""));

    context.strings.dot    = intern(context.string_table, STRING("."));
    context.strings.comma  = intern(context.string_table, STRING(","));
    context.strings.colon  = intern(context.string_table, STRING(":"));
    context.strings.paren_open  = intern(context.string_table, STRING("("));
    context.strings.paren_close = intern(context.string_table, STRING(")"));
    context.strings.curly_open  = intern(context.string_table, STRING("{"));
    context.strings.curly_close = intern(context.string_table, STRING("}"));
    context.strings.square_open  = intern(context.string_table, STRING("["));
    context.strings.square_close = intern(context.string_table, STRING("]"));

    auto &strings = context.strings;
    auto &table = context.string_table;

    strings.id        = intern(table, STRING("id"));

    strings.defines  = intern(table, STRING("defines"));
    strings.inherits = intern(table, STRING("inherits"));

    strings.body = intern(table, STRING("body"));
    strings.type  = intern(table, STRING("type"));
    strings.value = intern(table, STRING("value"));

    strings.classes = intern(table, STRING("classes"));
    strings.styles  = intern(table, STRING("styles"));

    strings.desktop = intern(table, STRING("desktop"));
    strings.mobile  = intern(table, STRING("mobile"));

    strings.page = intern(table, STRING("page"));
    strings.title        = intern(table, STRING("title"));
    strings.icon         = intern(table, STRING("icon"));
    strings.style_sheets = intern(table, STRING("style_sheets"));
    strings.scripts      = intern(table, STRING("scripts"));

    strings.text   = intern(table, STRING("text"));
    strings.div    = intern(table, STRING("div"));
    strings.list   = intern(table, STRING("list"));

    strings.h1   = intern(table, STRING("h1"));
    strings.p    = intern(table, STRING("p"));
    strings.span = intern(table, STRING("span"));

    strings.parameters = intern(table, STRING("parameters"));

    strings.form = intern(table, STRING("form"));

    strings.label = intern(table, STRING("label"));
    strings.input = intern(table, STRING("input"));
    strings.button = intern(table, STRING("button"));

    strings._for = intern(table, STRING("for"));

    strings.min     = intern(table, STRING("min"));
    strings.max     = intern(table, STRING("max"));
    strings.locked  = intern(table, STRING("locked"));
    strings.initial = intern(table, STRING("initial"));

    strings.email  = intern(table, STRING("email"));
    strings.number = intern(table, STRING("number"));
    strings.date   = intern(table, STRING("date"));
    strings.checkbox = intern(table, STRING("checkbox"));

    strings.select  = intern(table, STRING("select"));
    strings.option  = intern(table, STRING("option"));
    strings.options = intern(table, STRING("options"));


    TEMP_SCOPE(context.temporary);

    auto set = create_map<Interned_String, int>(context.temporary);
    auto ids = (Interned_String *)&context.strings;
    for(Usize i = 0;
        i < sizeof(context.strings)/sizeof(Interned_String);
        i += 1
    ) {
        auto id = ids[i];
        assert(id != 0);
        assert(insert_maybe(set, id, 0));
    }

    context.simple_types.allocator = &context.arena;
    insert(context.simple_types, strings.h1, 0);
    insert(context.simple_types, strings.p, 0);
    insert(context.simple_types, strings.span, 0);
    insert(context.simple_types, strings.button, 0);
}


String get_id_identifier(Interned_String id, Id_Type *id_type) {
    auto ident = context.string_table[id];

    Id_Type type;
    Usize offset;
    if(ident.values[0] == '#') {
        type = ID_GLOBAL;
        offset = 1;
    }
    else if(ident.values[0] == '$') {
        type = ID_HTML;
        offset = 1;
    }
    else {
        type = ID_LOCAL;
        offset = 0;
    }

    ident.values += offset;
    ident.size   -= offset;

    if(id_type != NULL) {
        *id_type = type;
    }

    return ident;
}

Interned_String make_full_id(Interned_String prefix, String id, Id_Type id_type) {
    auto result = Interned_String {};

    if(prefix != 0) {
        TEMP_SCOPE(context.temporary);
        auto buffer = create_array<U8>(context.temporary);

        if(id_type == ID_LOCAL) {
            push(buffer, prefix);
            push(buffer, STRING("-"));
        }
        else if(id_type == ID_GLOBAL) {
            push(buffer, STRING("page-"));
        }
        else {
            assert(id_type == ID_HTML);
        }

        push(buffer, id);

        result = intern(context.string_table, str(buffer));
    }

    return result;
}

Interned_String make_full_id(Interned_String prefix, Interned_String id) {
    Id_Type type;
    auto ident = get_id_identifier(id, &type);

    auto result = make_full_id(prefix, ident, type);
    return result;
}


Interned_String intern_path(const char *string) {
    auto size = strlen(string);
    assert(size > 0);

    auto last = string[size - 1];

    if(last != '\\' && last != '/') {
        TEMP_SCOPE(context.temporary);

        auto buffer = create_array<U8>(context.temporary);
        push(buffer, String { (U8 *)string, (Usize)size });
        push(buffer, STRING("/"));
        return intern(context.string_table, str(buffer));
    }
    else {
        return intern(context.string_table, string);
    }
}

bool parse_arguments(int argument_count, const char **arguments) {
    if(argument_count < 2) {
        printf("Usage: %s [options] sources", arguments[0]);
        return false;
    }

    for(int i = 1; i < argument_count; i += 1) {
        auto string = arguments[i];
        auto size   = strlen(string);
        assert(size > 0);

        if(string[0] == '-') {
            if(size < 2) {
                printf("Invalid argument '%s'\n", string);
                return false;
            }

            if(string[1] == 'i') {
                i += 1;
                if(i >= argument_count) {
                    printf("'-i' requires an argument.\n");
                    return false;
                }

                auto path = arguments[i];
                push(context.include_paths, intern_path(path));
            }
            else if(string[1] == 'o') {
                i += 1;
                if(i >= argument_count) {
                    printf("'-o' requires an argument.\n");
                    return false;
                }

                auto path = arguments[i];
                if(context.output_prefix != 0) {
                    printf("Error: Output prefix ('-o') provided multiple times.\n");
                    return false;
                }

                context.output_prefix = intern_path(path);
            }
            else {
                printf("Invalid argument '%s'\n", string);
                return false;
            }
        }
        else {
            auto source = Source {};
            source.file_path = intern(context.string_table, string);
            push(context.sources, source);
        }
    }

    if(context.include_paths.count == 0) {
        push(context.include_paths, intern_path("."));
    }

    if(context.output_prefix == 0) {
        context.output_prefix = intern_path("wsc-output");
    }

    return true;
}

bool read_sources() {
    for(Usize i = 0; i < context.sources.count; i += 1) {
        auto &source = context.sources[i];

        auto name = context.string_table[source.file_path];
        auto path = find_first_file(context.include_paths, name);
        if(path == 0) {
            printf("Error: Could not find file %s.\n", name.values);
            return false;
        }

        auto path_string = context.string_table[path].values;
        auto buffer = create_array<U8>(context.arena);
        if(!read_entire_file((char *)path_string, buffer)) {
            printf("Error: reading file %s\n", path_string);
            return false;
        }

        source.file_path = path;
        source.content = buffer;
    }

    return true;
}

