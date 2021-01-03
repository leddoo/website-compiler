#pragma once

#include "util.hpp"
#include "parser.hpp"
#include "analyzer.hpp"

#include <libcpp/memory/arena.hpp>
using namespace libcpp;

struct Source {
    Interned_String file_path;
    Array<U8> content;
};

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
            id,
            defines, inherits,
            body, type, value,
            classes, styles,
            desktop, mobile,
            page, title, icon, style_sheets, scripts,
            text, div, list,
            h1, p, span,
            parameters,
            form,
            label, input, button,
            _for,
            min, max, locked, initial,
            email, number, date, checkbox,
            select, option, options,
            anchor, href,
            required, min_length, max_length;
    } strings;

    Map<Interned_String, int> simple_types;


    // Parser
    U32 next_expression_id;
    Array<Expression> expressions;

    // Analyzer
    Map<Interned_String, Symbol> symbols;

    Array<Expression *> exports;


    Array<Source> sources;
    Array<Interned_String> include_paths;
    Interned_String output_prefix;
    Array<Source> outputs;
    Map<Interned_String, int> referenced_files;

} context;

void setup_context();

_inline void push(Array<U8> &array, Interned_String id) {
    push(array, context.string_table[id]);
}


enum Id_Type {
    ID_LOCAL,
    ID_GLOBAL,
    ID_HTML,
};

String get_id_identifier(Interned_String id, Id_Type *id_type);

Interned_String make_full_id(Interned_String prefix, String id, Id_Type id_type);

Interned_String make_full_id(Interned_String prefix, Interned_String id);


bool parse_arguments(int argument_count, const char **arguments);
bool read_sources();

