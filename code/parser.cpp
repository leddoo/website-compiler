#include "parser.hpp"
#include "context.hpp"

#include <cstdio>

//
// RANGE tokenizer.
//

enum Token_Type {
    TOKEN_ATOM,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_EOL,
    TOKEN_OTHER,
};

struct Token {
    Interned_String string;

    Unsigned source_line;
    Unsigned source_column;
    Unsigned source_size;

    Token_Type type;
};


static bool tokenize(
    String_Table &string_table,
    const Array<U8> &buffer,
    Array<Token> &result
) {
    auto reader = make_reader(buffer);

    auto source_line = (Unsigned)1;
    auto line_begin = reader.current;

    while(true) {
        auto old_source_line = source_line;
        skip_whitespace(reader, source_line, line_begin);
        if(reader.current >= reader.end) {
            break;
        }

        for(auto l = old_source_line; l < source_line; l += 1) {
            auto eol = Token {};
            eol.type = TOKEN_EOL;
            push(result, eol);
        }

        auto token_begin = reader.current;

        auto token = Token {};
        token.source_line = source_line;
        token.source_column = (Unsigned)(token_begin - line_begin) + 1;

        auto at = *reader.current;
        reader.current += 1;

        if(is_alpha(at) || at == '_') {
            // atom.
            while(reader.current < reader.end) {
                auto at = *reader.current;
                if(!is_alpha(at) && !is_numeric(at) && at != '_') {
                    break;
                }

                reader.current += 1;
            }

            token.type = TOKEN_ATOM;
        }
        else if(at == '"') {
            // string.
            auto string = String {};

            reader.current -= 1;
            if(!read_quoted_string(reader, string)) {
                printf("Error: String at %lld, %lld without closing '\"'.\n", 
                    token.source_line, token.source_column
                );
                return false;
            }

            token.string = intern(string_table, string);
            token.type = TOKEN_STRING;
        }
        else if(is_numeric(at)) {
            // number.
            while(reader.current < reader.end) {
                auto at = *reader.current;
                if(!is_numeric(at) && at != '_') {
                    break;
                }

                reader.current += 1;
            }

            token.type = TOKEN_NUMBER;
        }
        else {
            token.type = TOKEN_OTHER;
        }

        if(token.string == 0) {
            token.string = intern(string_table, str(token_begin, reader.current));
        }

        token.source_size = (Unsigned)(reader.current - token_begin);
        push(result, token);
    }

    // NOTE(llw): Extra tokens for lookahead.
    for(Usize i = 0; i < 2; i += 1) {
        auto eol = Token {};
        eol.type = TOKEN_EOL;
        push(result, eol);
    }

    return true;
}



//
// RANGE parser.
//

Signed skip_eol(Reader<Token> &reader) {
    while( reader.current < reader.end
        && reader.current->type == TOKEN_EOL
    ) {
        reader.current += 1;
    }

    return reader.end - reader.current;
}

bool is_valid(const Expression &e) {
    return e.type != 0;
}

Expression parse_expression(Reader<Token> &reader);

bool parse_argument(
    Reader<Token> &reader,
    Argument &result,
    U32 parent_expression
) {
    if(reader.current >= reader.end) {
        printf("Unexpected end of file.\n");
        return false;
    }

    // NOTE(llw): Consume current.
    auto at = *reader.current;
    reader.current += 1;

    if(at.type == TOKEN_ATOM) {
        result.type = ARG_ATOM;
        result.value = at.string;
        return true;
    }
    else if(at.type == TOKEN_STRING) {
        result.type = ARG_STRING;
        result.value = at.string;
        return true;
    }
    else if(at.type == TOKEN_NUMBER) {
        result.type = ARG_NUMBER;
        result.value = at.string;
        return true;
    }
    else if(at.string == context.strings.curly_open) {
        result.type = ARG_BLOCK;
        result.block = create_array<Expression>(context.arena);

        while(true) {

            if(skip_eol(reader) < 1) {
                printf("Unexpected end of file.\n");
                return false;
            }

            if(reader.current->string == context.strings.curly_close) {
                reader.current += 1;
                break;
            }

            auto expr = parse_expression(reader);
            if(!is_valid(expr)) {
                return false;
            }

            expr.parent = parent_expression;

            push(result.block, expr);
        }

        return true;
    }
    else if(at.string == context.strings.square_open) {
        result.type = ARG_LIST;
        result.list = create_array<Argument>(context.arena);

        auto was_last = false;
        while(true) {

            if(skip_eol(reader) < 2) {
                printf("Unexpected eof.\n");
                return false;
            }

            auto t0 = reader.current[0];
            auto t1 = reader.current[1];

            if(t0.string == context.strings.square_close) {
                reader.current += 1;
                break;
            }

            if(was_last) {
                printf("Comma missing after list item.\n");
                return false;
            }

            auto value = Argument {};
            if(!parse_argument(reader, value, parent_expression)) {
                return false;
            }

            push(result.list, value);

            if(t1.string == context.strings.comma) {
                reader.current += 1;
            }
            else {
                was_last = true;
            }
        }

        return true;
    }
    else {
        printf("Invalid argument value.\n");
        return false;
    }

}

Expression parse_expression(Reader<Token> &reader) {
    if(skip_eol(reader) < 1) {
        printf("Unexpected end of file.\n");
        return {};
    }

    auto t0 = *reader.current;

    auto is_multi_line = false;
    auto type = t0.string;

    // NOTE(llw): Check if is multi line expression - starts with '('.
    if(t0.string == context.strings.paren_open) {
        // NOTE(llw): Consume '('.
        reader.current += 1;
        is_multi_line = true;

        if(reader.current >= reader.end) {
            printf("Unexpected end of file after '('\n");
            return {};
        }

        auto t1 = *reader.current;
        if(t1.type != TOKEN_ATOM) {
            printf("Non-atom token after '('\n");
            return {};
        }

        type = t1.string;
    }

    // NOTE(llw): Consume type.
    reader.current += 1;


    auto reached_eof = false;
    auto was_last = false;

    context.next_expression_id += 1;
    auto own_id = context.next_expression_id;

    // NOTE(llw): Parse arguments.
    auto arguments = create_map<Interned_String, Argument>(context.arena);
    while(reader.current < reader.end) {

        if(is_multi_line && skip_eol(reader) < 1) {
            reached_eof = true;
            break;
        }

        auto at = *reader.current;

        auto done = was_last;
        if(is_multi_line) {
            done |= at.string == context.strings.paren_close;
        }
        else {
            done |= at.type == TOKEN_EOL;
            done |= at.string == context.strings.curly_close;
        }

        if(done) {
            // NOTE(llw): Consume ')'.
            if(is_multi_line) {
                reader.current += 1;
            }

            auto result = Expression {};
            result.id = own_id;
            result.type = type;
            result.arguments = arguments;
            return result;
        }

        // NOTE(llw): Parse "name : value".
        if(reader.current + 3 > reader.end) {
            printf("Not enough tokens.\n");
            return {};
        }

        auto t0 = reader.current[0];
        auto t1 = reader.current[1];
        auto t2 = reader.current[2];

        if(t0.type != TOKEN_ATOM) {
            printf("Argument must begin with name.\n");
            return {};
        }

        if(t1.string != context.strings.colon) {
            printf("Argument name must be followed by a colon.\n");
            return {};
        }

        // NOTE(llw): Consume name and colon.
        reader.current += 2;

        auto arg_name = t0.string;
        auto arg = Argument {};
        if(!parse_argument(reader, arg, own_id)) {
            return {};
        }

        if(has(arguments, arg_name)) {
            printf("Argument provided multiple times.\n");
            return {};
        }
        insert(arguments, arg_name, arg);

        // NOTE(llw): Try to consume ','.
        if(    reader.current < reader.end
            && reader.current->string == context.strings.comma
        ) {
            reader.current += 1;
        }
        else {
            was_last = true;
        }

    }

    printf("Error: Reached end of file in expression starting at %lld, %lld.\n",
        t0.source_line, t0.source_column
    );
    return {};
}


void print(const Expression &expr, Unsigned indent) {
    auto &string_table = context.string_table;

    auto do_indent = [&](int offset = 0) {
        for(Usize i = 0; i < indent + offset; i += 1) {
            printf("    ");
        }
    };

    do_indent();

    auto type = string_table[expr.type];
    printf("Expression type: %.*s\n",
        (int)type.size, type.values
    );

    auto print_simple_arg = [&](Argument_Type type, Interned_String value) {
        printf("type: ");
        switch(type) {
            case ARG_ATOM: { printf("atom"); } break;
            case ARG_STRING: { printf("string"); } break;
            case ARG_NUMBER: { printf("number"); } break;
            case ARG_BLOCK: case ARG_LIST: default: assert(false);
        }

        auto string = string_table[value];
        printf(", value: %.*s\n", (int)string.size, string.values);
    };

    // NOTE(llw): Print arguments.
    auto &arguments = expr.arguments;
    for(Usize i = 0; i < arguments.count; i += 1) {
        do_indent(1);

        auto arg_name = arguments.entries[i].key;
        auto arg      = arguments.entries[i].value;

        auto name = string_table[arg_name];
        printf("name: %.*s, ", (int)name.size, name.values);

        if(arg.type == ARG_BLOCK) {
            printf("type: block\n");

            for(Usize i = 0; i < arg.block.count; i += 1) {
                print(arg.block[i], indent + 2);
            }
        }
        else if(arg.type == ARG_LIST) {
            printf("type: list\n");

            for(Usize i = 0; i < arg.list.count; i += 1) {
                do_indent(2);
                print_simple_arg(arg.list[i].type, arg.list[i].value);
            }
        }
        else {
            print_simple_arg(arg.type, arg.value);
        }
    }
}

bool parse(const Array<U8> &buffer) {
    auto tokens = create_array<Token>(context.temporary);
    if(!tokenize(context.string_table, buffer, tokens)) {
        return false;
    }

    #if 0
    for(Usize i = 0 ; i < tokens.count; i += 1) {
        auto t = tokens[i];
        if(t.string != 0) {
            auto s = context.string_table[t.string];
            printf("%d %.*s\n", t.type, (int)s.size, s.values);
        }
        else {
            printf("0 \\n\n");
        }
    }
    #endif

    auto token_reader = Reader<Token> {
        tokens.values,
        tokens.values + tokens.count
    };
    while(true) {

        if(skip_eol(token_reader) < 1) {
            break;
        }

        auto expr = parse_expression(token_reader);
        if(!is_valid(expr)) {
            return false;
        }

        #if 0
        print(expr);
        #endif

        push(context.expressions, expr);
    }

    return true;
}


Argument create_argument(Argument_Type type, Allocator &allocator) {
    auto result = Argument {};
    result.type = type;

    switch(result.type) {
        case ARG_ATOM:
        case ARG_STRING:
        case ARG_NUMBER: {} break;

        case ARG_BLOCK: {
            result.block = { &allocator };
        } break;

        case ARG_LIST: {
            result.list = { &allocator };
        } break;
    }

    return result;
}

Argument duplicate(const Argument &argument, Allocator &allocator) {
    auto result = Argument {};
    result.type = argument.type;

    switch(argument.type) {
        case ARG_ATOM:
        case ARG_STRING:
        case ARG_NUMBER: {
            result.value = argument.value;
        } break;

        case ARG_BLOCK: {
            result.block = duplicate(argument.block, allocator);
            for(Usize i = 0; i < result.block.count; i += 1) {
                result.block[i] = duplicate(result.block[i], allocator);
            }
        } break;

        case ARG_LIST: {
            result.list = duplicate(argument.list, allocator);
            for(Usize i = 0; i < result.list.count; i += 1) {
                result.list[i] = duplicate(result.list[i], allocator);
            }
        } break;
    }

    return result;
}

Expression duplicate(const Expression &expression, Allocator &allocator) {
    auto result = Expression {};
    result = expression;
    result.arguments = duplicate(expression.arguments, allocator);

    for(Usize i = 0; i < result.arguments.count; i += 1) {
        auto &v = result.arguments.entries[i].value;
        v = duplicate(v, allocator);
    }

    return result;
}

