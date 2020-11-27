#pragma once

#include "util.hpp"

#include <libcpp/memory/arena.hpp>

struct Expression;


enum Argument_Type {
    ARG_ATOM,
    ARG_STRING,
    ARG_NUMBER,
    ARG_BLOCK,
    ARG_LIST,
};

struct Argument {
    union {
        Interned_String   value;
        Array<Expression> block;
        Array<Argument>   list;
    };
    Argument_Type type;
};

Argument duplicate(const Argument &argument, Allocator &allocator);
Argument create_argument(Argument_Type type, Allocator &allocator);

_inline bool is_arg_type(const Argument *arg, Argument_Type t) {
    return arg != NULL && arg->type == t;
}
_inline bool is_atom  (const Argument *arg) { return is_arg_type(arg, ARG_ATOM); }
_inline bool is_string(const Argument *arg) { return is_arg_type(arg, ARG_STRING); }
_inline bool is_number(const Argument *arg) { return is_arg_type(arg, ARG_NUMBER); }
_inline bool is_block (const Argument *arg) { return is_arg_type(arg, ARG_BLOCK); }
_inline bool is_list  (const Argument *arg) { return is_arg_type(arg, ARG_LIST); }


struct Expression {
    U32 id;
    U32 parent;
    Interned_String type;
    Map<Interned_String, Argument> arguments;
};

Expression duplicate(const Expression &expression, Allocator &allocator);

bool parse(const Array<U8> &buffer);

void print(const Expression &expression, Unsigned indent = 0);

namespace libcpp {
    template <>
    _inline bool eq(Argument left, Argument right) {
        if(left.type != right.type) {
            return false;
        }

        switch(left.type) {

            case ARG_ATOM:
            case ARG_STRING:
            case ARG_NUMBER: {
                return left.value == right.value;
            } break;

            case ARG_BLOCK: {
                assert(false);
                return false;
                //return eq(left.block, right.block);
            } break;

            case ARG_LIST: {
                return eq(left.list, right.list);
            } break;

            default: {
                assert(false);
                return false;
            } break;
        }
    }
}

