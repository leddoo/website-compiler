#include "context.hpp"

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
    strings.global_id = intern(table, STRING("global_id"));

    strings.defines  = intern(table, STRING("defines"));
    strings.inherits = intern(table, STRING("inherits"));

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

    strings.form        = intern(table, STRING("form"));
    strings.form_field  = intern(table, STRING("form_field"));
    strings.form_submit = intern(table, STRING("form_submit"));

    strings.form_list      = intern(table, STRING("form_list"));
    strings.form_list_item = intern(table, STRING("form_list_item"));

    strings.min     = intern(table, STRING("min"));
    strings.max     = intern(table, STRING("max"));
    strings.locked  = intern(table, STRING("locked"));
    strings.initial = intern(table, STRING("initial"));

    strings.email = intern(table, STRING("email"));


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


Interned_String make_full_id_from_lid(Interned_String prefix, Interned_String lid) {
    TEMP_SCOPE(context.temporary);
    auto buffer = create_array<U8>(context.temporary);
    push(buffer, prefix);
    push(buffer, (U8)'.');
    push(buffer, lid);
    return intern(context.string_table, str(buffer));
}

Interned_String make_full_id_from_gid(Interned_String gid) {
    TEMP_SCOPE(context.temporary);
    auto buffer = create_array<U8>(context.temporary);
    push(buffer, STRING("page."));
    push(buffer, gid);
    return intern(context.string_table, str(buffer));
}

