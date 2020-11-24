#pragma once

#include "util.hpp"

#include <libcpp/memory/arena.hpp>

struct Parse_Context;
struct Expression;


enum Argument_Type {
    ARG_ATOM,
    ARG_STRING,
    ARG_NUMBER,
    ARG_LIST,
};

struct Argument {
    union {
        Array<Expression> expressions;
        Interned_String value;
    };
    Argument_Type type;
};


struct Expression {
    U32 id;
    U32 parent;
    Interned_String type;
    Map<Interned_String, Argument> arguments;
};

bool parse(const Array<U8> &buffer);

