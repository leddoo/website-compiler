#pragma once

#include "util.hpp"
#include "parser.hpp"


bool analyze();


enum Symbol_State {
    SYMS_DISCOVERED,
    SYMS_PROCESSING,
    SYMS_DONE,
};

struct Symbol {
    Expression *expression;
    Symbol_State state;
};

