#pragma once

#include "util.hpp"
#include "parser.hpp"


bool analyze();


enum Symbol_Type {
    SYM_PAGE,
    SYM_GENERIC_PAGE,
};

enum Symbol_State {
    SYMS_DISCOVERED,
    SYMS_PROCESSING,
    SYMS_DONE,
};

struct Symbol {
    Expression *expression;

    Symbol_Type type;
    Symbol_State state;
};

