#pragma once

#include "util.hpp"
#include "parser.hpp"
#include "analyzer.hpp"

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
            page,
            text, spacer,
            h1, p, span,
            parameters, inherits,
            style_sheets, scripts;
    } strings;

    Map<Interned_String, int> valid_text_types;


    // Parser
    U32 next_expression_id;
    Array<Expression> expressions;

    // Analyzer
    Map<Interned_String, Symbol> symbols;

} context;

void setup_context();

