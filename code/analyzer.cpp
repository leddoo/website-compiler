#include "analyzer.hpp"
#include "context.hpp"

#include "stdio.h"

static bool validate(Symbol &symbol);
static bool validate(const Expression &expr);
static bool instantiate_page(const Expression &page, Expression &result);


bool analyze() {

    // NOTE(llw): Fill symbol table.
    for(Usize i = 0; i < context.expressions.count; i += 1) {
        auto &expr = context.expressions[i];

        auto symbol = Symbol {};

        if(expr.type == context.strings.page) {
            symbol.type = SYM_PAGE;
            symbol.expression = &expr;

            if(has(expr.arguments, context.strings.parameters)) {
                symbol.type = SYM_GENERIC_PAGE;
            }
        }
        else {
            printf("Not a definition.\n");
            return false;
        }

        auto name = get_pointer(expr.arguments, context.strings.name);
        if(name == NULL) {
            printf("Definition without name.\n");
            return false;
        }

        if(!insert_maybe(context.symbols, name->value, symbol)) {
            printf("Multiple definitions.\n");
            return false;
        }
    }

    // NOTE(llw): Validate.
    for(Usize i = 0; i < context.symbols.count; i += 1) {
        if(!validate(context.symbols.entries[i].value)) {
            return false;
        }
    }

    // NOTE(llw): Instantiate non-generic, inheriting pages.
    for(Usize i = 0; i < context.symbols.count; i += 1) {
        auto &symbol = context.symbols.entries[i].value;

        const auto &page = *symbol.expression;
        auto inherits = has(page.arguments, context.strings.inherits);
        if(symbol.type == SYM_PAGE && inherits) {

            auto &expr = *allocate<Expression>(context.arena);
            if(!instantiate_page(page, expr)) {
                return false;
            }

            auto name = page.arguments[context.strings.name];
            insert(expr.arguments, context.strings.name, name);

            if(!validate(expr)) {
                return false;
            }

            symbol.expression = &expr;
        }

    }

    return true;
}



static bool is_list_of(
    const Argument &arg, Argument_Type type,
    Usize *first_non_type
) {
    auto non_type = (Usize)-1;
    auto valid = false;

    if(arg.type == ARG_LIST) {
        valid = true;

        const auto &list = arg.list;
        for(Usize i = 0; i < list.count; i += 1) {
            auto item = list[i];

            if(item.type != type) {
                non_type = i;
                valid = false;
                break;
            }
        }
    }

    if(first_non_type) {
        *first_non_type = non_type;
    }
    return valid;
}

static bool validate(const Expression &expr) {

    TEMP_SCOPE(context.temporary);

    const auto &args = expr.arguments;

    auto used_args = create_map<Interned_String, int>(context.temporary);
    auto use_arg = [&](Interned_String arg) {
        insert_maybe(used_args, arg, 0);
    };

    // NOTE(llw): Check id arguments.
    auto has_lid = has(args, context.strings.id);
    auto has_gid = has(args, context.strings.global_id);
    auto has_id = has_lid | has_gid;
    if(has_lid && has_gid) {
        printf("Cannot have both local and global id.\n");
        return false;
    }
    use_arg(context.strings.id);
    use_arg(context.strings.global_id);


    if(expr.type == context.strings.page) {
        use_arg(context.strings.name);
        use_arg(context.strings.body);
        use_arg(context.strings.style_sheets);
        use_arg(context.strings.scripts);

        if(expr.parent) {
            printf("Pages cannot be nested.\n");
            return false;
        }

        auto parameters = get_pointer(args, context.strings.parameters);
        auto inherits = get_pointer(args, context.strings.inherits);

        auto is_generic = parameters != NULL;
        auto is_final   = !is_generic && !inherits;


        // TODO(llw): Warn about duplicates, extract into procedure that prints
        // the errors (takes name of list).

        // NOTE(llw): Check parameters.
        if(is_generic) {
            use_arg(context.strings.parameters);

            if(!is_list_of(*parameters, ARG_ATOM, NULL)) {
                printf("Error: 'parameters' must be a list of atoms.\n");
                return false;
            }
        }

        // NOTE(llw): Check inheritance.
        if(inherits) {
            use_arg(context.strings.inherits);

            if(inherits->type != ARG_STRING) {
                printf("'inherits' requires a string.\n");
                return false;
            }

            // NOTE(llw): Check super existence and type.
            auto symbol = get_pointer(context.symbols, inherits->value);
            if(symbol == NULL) {
                printf("Inherited page does not exist.\n");
                return false;
            }
            if(symbol->type != SYM_GENERIC_PAGE) {
                printf("Inherited page is not a generic page.\n");
                return false;
            }

            // NOTE(llw): Recurse.
            if(!validate(*symbol)) {
                return false;
            }

            // NOTE(llw): Check parameters provided exactly once.
            const auto &super = *symbol->expression;
            const auto &params = super.arguments[context.strings.parameters].list;

            TEMP_SCOPE(context.temporary);
            auto provided = create_map<Interned_String, int>(context.temporary);

            for(Usize i = 0; i < params.count; i += 1) {
                auto name = params[i].value;

                // TODO(llw): This does not actually check if a parameter is
                // provided multiple times (that would be a parse error by the
                // way). Therefore this is redundant once duplicates are
                // detected in generics above.
                if(!insert_maybe(provided, name, 0)) {
                    printf("Duplicate parameter.\n");
                    return false;
                }

                if(!has(args, name)) {
                    printf("Parameter not provided.\n");
                    return false;
                }

                use_arg(name);
            }
        }

        // NOTE(llw): Check body.
        if(is_final) {
            auto body = get_pointer(args, context.strings.body);
            if(body == NULL) {
                printf("Non-inheriting pages require a body.\n");
                return false;
            }

            if(body->type != ARG_BLOCK) {
                printf("Page body must be a block.\n");
                return false;
            }

            const auto &children = body->block;
            for(Usize i = 0; i < children.count; i += 1) {
                if(!validate(children[i])) {
                    return false;
                }
            }
        }

        // NOTE(llw): Check style sheets and scripts.
        auto style_sheets = get_pointer(args, context.strings.style_sheets);
        if(style_sheets != NULL) {
            if(!is_list_of(*style_sheets, ARG_STRING, NULL)) {
                printf("Error: 'style_sheets' must be a list of strings.\n");
                return false;
            }
        }

        auto scripts = get_pointer(args, context.strings.style_sheets);
        if(scripts != NULL) {
            if(!is_list_of(*scripts, ARG_STRING, NULL)) {
                printf("Error: 'scripts' must be a list of strings.\n");
                return false;
            }
        }

    }
    else if(expr.type == context.strings.text) {
        auto type  = get_pointer(args, context.strings.type);
        auto value = get_pointer(args, context.strings.value);
        if(    !is_string(type)
            || !is_string(value)
        ) {
            printf("Invalid text expression.\n");
            return false;
        }

        if(!has(context.valid_text_types, type->value)) {
            printf("Invalid text type.\n");
            return false;
        }

        use_arg(context.strings.type);
        use_arg(context.strings.value);
    }
    else if(expr.type == context.strings.spacer) {
        if(has_id) {
            printf("Spacers can't have ids.\n");
            return false;
        }

        auto value   = get_pointer(args, context.strings.value);
        auto desktop = get_pointer(args, context.strings.desktop);
        auto mobile  = get_pointer(args, context.strings.mobile);

        if(is_number(value)) {
            use_arg(context.strings.value);
        }
        else if(is_number(desktop) && is_number(mobile)) {
            use_arg(context.strings.desktop);
            use_arg(context.strings.mobile);
        }
        else {
            printf("Invalid spacer expression.\n");
            return false;
        }
    }
    else {
        auto type = context.string_table[expr.type];
        printf("Unrecognized expression type: '%.*s'\n",
            (int)type.size, type.values
        );

        return false;
    }

    for(Usize i = 0; i < args.count; i += 1) {
        auto name = args.entries[i].key;

        if(!has(used_args, name)) {
            auto string = context.string_table[name];
            printf("Warning: Unused argument '%.*s'\n",
                (int)string.size, string.values
            );
        }
    }

    return true;
}

static bool validate(Symbol &symbol) {
    if(symbol.state == SYMS_DONE) {
        return true;
    }
    else if(symbol.state == SYMS_PROCESSING) {
        printf("Circular dependency.\n");
        return false;
    }

    symbol.state = SYMS_PROCESSING;
    auto success = validate(*symbol.expression);
    symbol.state = SYMS_DONE;

    return success;
}


static bool insert_parameters(
    Argument &arg,
    const Map<Interned_String, Argument> parameters
) {
    switch(arg.type) {
        case ARG_ATOM: {
            auto value = get_pointer(parameters, arg.value);
            if(value) {
                arg = *value;
            }
            return true;
        } break;

        case ARG_STRING:
        case ARG_NUMBER: {
            return true;
        } break;

        case ARG_LIST: {
            auto &list = arg.list;
            for(Usize i = 0; i < list.count; i += 1) {
                if(!insert_parameters(list[i], parameters)) {
                    return false;
                }
            }

            return true;
        } break;

        case ARG_BLOCK: {
            auto &block = arg.block;
            for(Usize i = 0; i < block.count; i += 1) {
                auto &expr = block[i];
                auto &args = expr.arguments;

                auto replace = get_pointer(parameters, expr.type);
                if(replace) {

                    if(replace->type == ARG_ATOM) {
                        expr.type = replace->value;
                    }
                    else if(replace->type == ARG_BLOCK) {
                        if(args.count > 0) {
                            printf("Cannot insert expressions. Original expression has arguments.\n");
                            return false;
                        }

                        // NOTE(llw): Like push_at but removes block[i].
                        auto amount = replace->block.count;
                        make_space_at(block, i, amount - 1);
                        copy_values(block.values + i, replace->block.values, amount);

                        continue;
                    }
                    else {
                        printf("Cannot replace expression. Not an atom or block.\n");
                        return false;
                    }

                }

                for(Usize j = 0; j < args.count; j += 1) {
                    if(!insert_parameters(args.entries[j].value, parameters)) {
                        return false;
                    }
                }
            }

            return true;
        } break;

        default: {
            assert(false);
            return false;
        } break;
    }
}


static bool instantiate_page(
    const Expression &derived,
    Expression &self, // modified
    Expression &result
) {
    TEMP_SCOPE(context.temporary);

    auto &own_args = self.arguments;

    // NOTE(llw): Collect parameters.
    auto parameters = create_map<Interned_String, Argument>(context.temporary);
    const auto &name_list = own_args[context.strings.parameters].list;
    for(Usize i = 0; i < name_list.count; i += 1) {
        auto name = name_list[i].value;
        insert(parameters, name, derived.arguments[name]);
    }

    // NOTE(llw): Insert parameters into self.
    for(Usize i = 0; i < own_args.count; i += 1) {
        auto name = own_args.entries[i].key;
        auto &arg = own_args.entries[i].value;
        if(name != context.strings.parameters) {
            if(!insert_parameters(arg, parameters)) {
                return false;
            }
        }
    }

    // NOTE(llw): Recurse.
    auto inherits = get_pointer(own_args, context.strings.inherits);
    if(inherits) {

        auto super = context.symbols[inherits->value];
        auto new_self = duplicate(*super.expression, context.temporary);
        if(!instantiate_page(self, new_self, result)) {
            return false;
        }
    }


    // NOTE(llw): Build result.
    auto &res_args = result.arguments;

    auto body = get_pointer(own_args, context.strings.body);
    if(body) {
        res_args[context.strings.body] = duplicate(*body, context.arena);
    }

    auto style_sheets = get_pointer(own_args, context.strings.style_sheets);
    if(style_sheets) {
        push(res_args[context.strings.style_sheets].list, style_sheets->list);
    }

    auto scripts = get_pointer(own_args, context.strings.scripts);
    if(scripts) {
        push(res_args[context.strings.scripts].list, scripts->list);
    }


    return true;
}

static bool instantiate_page(
    const Expression &page,
    Expression &result
) {
    // NOTE(llw): Initialize result.
    context.next_expression_id += 1;
    result = {};
    result.id = context.next_expression_id;
    result.type = context.strings.page;
    result.arguments = create_map<Interned_String, Argument>(context.arena);
    insert(
        result.arguments,
        context.strings.body,
        create_argument(ARG_BLOCK, context.arena)
    );
    insert(
        result.arguments,
        context.strings.style_sheets,
        create_argument(ARG_LIST, context.arena)
    );
    insert(
        result.arguments,
        context.strings.scripts,
        create_argument(ARG_LIST, context.arena)
    );

    TEMP_SCOPE(context.temporary);

    auto super_name = page.arguments[context.strings.inherits].value;
    auto super = *context.symbols[super_name].expression;
    super = duplicate(super, context.temporary);

    auto success = instantiate_page(page, super, result);
    return success;
}

