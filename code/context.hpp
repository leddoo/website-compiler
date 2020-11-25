#pragma once

#include "util.hpp"
#include "parser.hpp"
#include <libcpp/memory/arena.hpp>
using namespace libcpp;

extern struct Context {

    Arena temporary;
    Arena arena;

    String_Table string_table;
    struct {
        Interned_String
            dot, comma, colon,
            paren_open, paren_close,
            curly_open, curly_close,
            square_open, square_close,
            id, global_id,
            name, body,
            type, value,
            desktop, mobile,
            def_page,
            text, spacer,
            spacer_same, spacer_desktop_mobile,
            h1, p, span;
    } strings;

    Map<Interned_String, int> valid_text_types;


    // Parser
    U32 next_expression_id;
    Array<Expression> expressions;

} context;

void setup_context();

