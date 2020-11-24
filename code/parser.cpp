#include "parser.hpp"

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

Expression parse_expression(Reader<Token> &reader, Parse_Context &context) {
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

    // NOTE(llw): Parse arguments.
    auto arguments = create_array<Argument>(context.arena);
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
        }

        if(done) {
            // NOTE(llw): Consume ')'.
            if(is_multi_line) {
                reader.current += 1;
            }

            auto result = Expression {};
            result.type = type;
            result.arguments = arguments;
            return result;
        }

        // NOTE(llw): Parse "name : value".
        if(reader.current + 3 > reader.end) {
            printf("Not enough tokens.\n");
            return {};
        }

        // NOTE(llw): Consume all 3 tokens.
        auto t0 = reader.current[0];
        auto t1 = reader.current[1];
        auto t2 = reader.current[2];
        reader.current += 3;

        if(t0.type != TOKEN_ATOM) {
            printf("Argument must begin with name.\n");
            return {};
        }

        if(t1.string != context.strings.colon) {
            printf("Argument name must be followed by a colon.\n");
            return {};
        }

        auto arg = Argument {};
        arg.name  = t0.string;
        arg.value = t2.string;

        if(t2.type == TOKEN_ATOM) {
            arg.type = ARG_ATOM;
        }
        else if(t2.type == TOKEN_STRING) {
            arg.type = ARG_STRING;
        }
        else if(t2.type == TOKEN_NUMBER) {
            arg.type = ARG_NUMBER;
        }
        else if(t2.string == context.strings.curly_open) {
            arg.type = ARG_EXPR_LIST;
            arg.expressions = create_array<Expression>(context.arena);

            auto success = false;
            while(reader.current < reader.end) {

                if(skip_eol(reader) < 1) {
                    reached_eof = true;
                    break;
                }

                if(reader.current->string == context.strings.curly_close) {
                    reader.current += 1;
                    success = true;
                    break;
                }

                auto expr = parse_expression(reader, context);
                if(!is_valid(expr)) {
                    break;
                }

                push(arg.expressions, expr);
            }

            if(!success) {
                break;
            }

        }
        else {
            printf("Invalid argument value.\n");
            return {};
        }

        push(arguments, arg);

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


void print_expression(const Expression &expr, String_Table &string_table, Unsigned indent = 0) {
    auto do_indent = [&]() {
        for(Usize i = 0; i < indent; i += 1) {
            printf("    ");
        }
    };

    do_indent();

    auto type = string_table[expr.type];
    printf("Expression type: %.*s\n",
        (int)type.size, type.values
    );

    indent += 1;

    auto &arguments = expr.arguments;
    for(Usize i = 0; i < arguments.count; i += 1) {
        do_indent();

        auto arg = arguments[i];

        auto name = string_table[arg.name];
        printf("name: %.*s, type: ", (int)name.size, name.values);

        if(arg.type == ARG_EXPR_LIST) {
            printf("expression_list\n");

            auto &expressions = arg.expressions;
            for(Usize i = 0; i < expressions.count; i += 1) {
                print_expression(expressions[i], string_table, indent + 1);
            }
        }
        else {
            switch(arg.type) {
                case ARG_ATOM: { printf("atom"); } break;
                case ARG_STRING: { printf("string"); } break;
                case ARG_NUMBER: { printf("number"); } break;
                case ARG_EXPR_LIST: assert(false);
            }

            auto value = string_table[arg.value];
            printf(", value: %.*s\n", (int)value.size, value.values);
        }
    }
}

bool parse(Parse_Context &context, const Array<U8> &buffer) {
    auto tokens = create_array<Token>(context.temporary);
    if(!tokenize(context.string_table, buffer, tokens)) {
        return false;
    }

    #if 0
    for(Usize i = 0 ; i < tokens.count; i += 1) {
        auto t = tokens[i];
        auto s = context.string_table[t.string];
        printf("%d %.*s\n", t.type, (int)s.size, s.values);
    }
    #endif

    auto token_reader = Reader<Token> {
        tokens.values,
        tokens.values + tokens.count
    };
    while(token_reader.current < token_reader.end) {

        auto expr = parse_expression(token_reader, context);
        if(!is_valid(expr)) {
            return false;
        }

        #if 0
        print_expression(expr, context.string_table);
        #endif

        push(context.expressions, expr);
    }

    return true;
}



Parse_Context create_parse_context(Arena &arena) {
    auto result = Parse_Context {};
    result.arena = arena;
    result.temporary = create_arena();
    result.string_table = create_string_table(arena);
    result.expressions = { &arena };

    result.strings.dot    = intern(result.string_table, STRING("."));
    result.strings.comma  = intern(result.string_table, STRING(","));
    result.strings.colon  = intern(result.string_table, STRING(":"));
    result.strings.paren_open  = intern(result.string_table, STRING("("));
    result.strings.paren_close = intern(result.string_table, STRING(")"));
    result.strings.curly_open  = intern(result.string_table, STRING("{"));
    result.strings.curly_close = intern(result.string_table, STRING("}"));

    return result;
}

