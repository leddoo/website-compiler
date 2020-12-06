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
            empty_string,
            dot, comma, colon,
            paren_open, paren_close,
            curly_open, curly_close,
            square_open, square_close,
            id, global_id,
            defines, inherits,
            body, type, value,
            desktop, mobile,
            page, title, icon, style_sheets, scripts,
            text, div,
            h1, p, span,
            parameters,
            form, form_field, form_submit,
            form_list, form_list_item,
            min, max, locked, initial,
            email;
    } strings;

    Map<Interned_String, int> simple_types;


    // Parser
    U32 next_expression_id;
    Array<Expression> expressions;

    // Analyzer
    Map<Interned_String, Symbol> symbols;

    Array<Expression *> exports;

} context;

void setup_context();

_inline void push(Array<U8> &array, Interned_String id) {
    push(array, context.string_table[id]);
}


String get_id_identifier(Interned_String id, bool *is_global);

Interned_String make_full_id(Interned_String prefix, String id, bool is_global);

