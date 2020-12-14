#pragma once

#include "util.hpp"
#include "parser.hpp"


bool analyze();


struct Symbol {
    Expression *expression;
    bool instantiating;
};

