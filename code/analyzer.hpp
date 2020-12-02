#pragma once

#include "util.hpp"
#include "parser.hpp"


bool analyze();


enum Symbol_Type {
    SYMBOL_PAGE,
    SYMBOL_DIV,
    SYMBOL_FORM,

    SYMBOL_TYPE_COUNT,
};

enum Symbol_State {
    SYMS_DISCOVERED,
    SYMS_PROCESSING,
    SYMS_DONE,
};

struct Symbol {
    Expression *expression;
    Symbol_State state;
};

