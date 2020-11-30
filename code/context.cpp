#include "context.hpp"

Context context;

void setup_context() {
    context = {};

    context.arena        = create_arena();
    context.temporary    = create_arena();
    context.string_table = create_string_table(context.arena);
    context.expressions  = { &context.arena };
    for(Usize i = 0; i < SYMBOL_TYPE_COUNT; i += 1) {
        context.symbols[i] = create_map<Interned_String, Symbol>(context.arena);
    }
    context.pages = { &context.arena };

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
    strings.global_id = intern(table, STRING("global_id"));

    strings.name = intern(table, STRING("name"));
    strings.body = intern(table, STRING("body"));

    strings.type  = intern(table, STRING("type"));
    strings.value = intern(table, STRING("value"));

    strings.desktop = intern(table, STRING("desktop"));
    strings.mobile  = intern(table, STRING("mobile"));

    strings.page = intern(table, STRING("page"));
    strings.title        = intern(table, STRING("title"));
    strings.icon         = intern(table, STRING("icon"));
    strings.style_sheets = intern(table, STRING("style_sheets"));
    strings.scripts      = intern(table, STRING("scripts"));

    strings.text   = intern(table, STRING("text"));
    strings.spacer = intern(table, STRING("spacer"));
    strings.div    = intern(table, STRING("div"));

    strings.h1   = intern(table, STRING("h1"));
    strings.p    = intern(table, STRING("p"));
    strings.span = intern(table, STRING("span"));

    strings.parameters = intern(table, STRING("parameters"));
    strings.inherits   = intern(table, STRING("inherits"));


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

    context.valid_text_types.allocator = &context.arena;
    insert(context.valid_text_types, strings.h1, 0);
    insert(context.valid_text_types, strings.p, 0);
    insert(context.valid_text_types, strings.span, 0);
}

