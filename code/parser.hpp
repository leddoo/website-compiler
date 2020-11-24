#pragma once

#include "util.hpp"

#include <libcpp/memory/arena.hpp>

struct Parse_Context;
struct Expression;


enum Argument_Type {
    ARG_ATOM,
    ARG_STRING,
    ARG_NUMBER,
    ARG_EXPR_LIST,
};

struct Argument {
    union {
        Array<Expression> expressions;
        Interned_String value;
    };

    Interned_String name;
    Argument_Type type;
};


struct Expression {
    Interned_String type;
    Array<Argument> arguments;
};

bool parse(Parse_Context &context, const Array<U8> &buffer);


struct Parse_Context {
    Arena arena;
    Arena temporary;

    String_Table string_table;
    struct {
        Interned_String
            dot, comma, colon,
            paren_open, paren_close,
            curly_open, curly_close;
    } strings;

    Array<Expression> expressions;
};

Parse_Context create_parse_context(Arena &arena);

