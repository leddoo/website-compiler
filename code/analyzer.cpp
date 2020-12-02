#include "analyzer.hpp"
#include "context.hpp"

#include "stdio.h"

static bool is_generic_page(const Expression &expr) {
    return expr.type == context.strings.page
        && has(expr.arguments, context.strings.parameters);
}

struct Validate_Context {
    Map<Interned_String, int> *id_table;
    Interned_String id_prefix;
};

static bool validate(Symbol &symbol);
static bool validate(const Expression &expr, Validate_Context vc);
static Expression *instantiate_page(const Expression &page);


bool analyze() {

    // NOTE(llw): Fill symbol table.
    for(Usize i = 0; i < context.expressions.count; i += 1) {
        auto &expr = context.expressions[i];

        auto symbol = Symbol {};
        symbol.expression = &expr;

        auto type = Symbol_Type {};

        if(expr.type == context.strings.page) {
            type = SYMBOL_PAGE;
        }
        else if(expr.type == context.strings.div) {
            type = SYMBOL_DIV;
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

        if(!insert_maybe(context.symbols[type], name->value, symbol)) {
            printf("Multiple definitions.\n");
            return false;
        }
    }

    // NOTE(llw): Validate.
    for(Usize type = 0; type < SYMBOL_TYPE_COUNT; type += 1) {
        auto &symbols = context.symbols[type];
        for(Usize i = 0; i < symbols.count; i += 1) {
            if(!validate(symbols.entries[i].value)) {
                return false;
            }
        }
    }

    // NOTE(llw): Instantiate pages.
    for(Usize i = 0; i < context.symbols[SYMBOL_PAGE].count; i += 1) {
        auto &symbol = context.symbols[SYMBOL_PAGE].entries[i].value;

        if(!is_generic_page(*symbol.expression)) {
            auto instance = instantiate_page(*symbol.expression);
            if(instance == NULL) {
                return false;
            }

            push(context.pages, instance);
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

static bool is_concrete_page(const Expression &expr) {
    return expr.type == context.strings.page
        && !has(expr.arguments, context.strings.parameters)
        && !has(expr.arguments, context.strings.inherits);
}

static bool validate_body(
    const Argument &arg,
    Validate_Context vc, Interned_String full_id
) {
    if(arg.type != ARG_BLOCK) {
        printf("Error: Body must be a block.\n");
        return false;
    }

    if(full_id) {
        vc.id_prefix = full_id;
    }

    const auto &block = arg.block;
    for(Usize i = 0; i < block.count; i += 1) {
        if(!validate(block[i], vc)) {
            return false;
        }
    }

    return true;
}

static bool validate(const Expression &expr, Validate_Context vc) {

    TEMP_SCOPE(context.temporary);

    const auto &args = expr.arguments;

    auto used_args = create_map<Interned_String, int>(context.temporary);
    auto use_arg = [&](Interned_String arg) {
        insert_maybe(used_args, arg, 0);
    };

    // NOTE(llw): Check id existence.
    use_arg(context.strings.id);
    use_arg(context.strings.global_id);
    auto lid = get_pointer(args, context.strings.id);
    auto gid = get_pointer(args, context.strings.global_id);
    auto has_id = lid != NULL || gid != NULL;
    if(lid != NULL && gid != NULL) {
        printf("Cannot have both local and global id.\n");
        return false;
    }

    // NOTE(llw): Check id type.
    if(    lid != NULL && lid->type != ARG_STRING
        || gid != NULL && gid->type != ARG_STRING
    ) {
        printf("Ids must be strings.\n");
        return false;
    }

    // NOTE(llw): Check id uniqueness.
    auto full_id = Interned_String {};
    if(vc.id_table != NULL && has_id) {
        if(lid != NULL) {
            full_id = make_full_id_from_lid(vc.id_prefix, lid->value);
        }
        else {
            full_id = make_full_id_from_gid(gid->value);
        }

        if(!insert_maybe(*vc.id_table, full_id, 0)) {
            printf("Error: Duplicate id.\n");
            return false;
        }
    }


    // SUB page.
    if(expr.type == context.strings.page) {
        use_arg(context.strings.name);
        use_arg(context.strings.body);
        use_arg(context.strings.title);
        use_arg(context.strings.icon);
        use_arg(context.strings.style_sheets);
        use_arg(context.strings.scripts);

        if(expr.parent) {
            printf("Pages cannot be nested.\n");
            return false;
        }

        if(has_id) {
            printf("Error: Pages cannot have an id.\n");
            return false;
        }

        auto parameters = get_pointer(args, context.strings.parameters);
        auto inherits   = get_pointer(args, context.strings.inherits);

        auto is_generic  = parameters != NULL;
        auto is_concrete = !is_generic && !inherits;


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
            auto symbol = get_pointer(context.symbols[SYMBOL_PAGE], inherits->value);
            if(symbol == NULL) {
                printf("Inherited page does not exist.\n");
                return false;
            }
            if(!is_generic_page(*symbol->expression)) {
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
        if(is_concrete) {
            auto body = get_pointer(args, context.strings.body);
            if(body == NULL) {
                printf("Non-inheriting pages require a body.\n");
                return false;
            }

            if(!validate_body(*body, vc, 0)) {
                return false;
            }
        }

        // NOTE(llw): Check title, icon, style sheets, scripts.

        auto title = get_pointer(args, context.strings.title);
        if(title != NULL) {
            if(title->type != ARG_STRING) {
                printf("Error: 'title' must be a string.\n");
                return false;
            }
        }

        auto icon = get_pointer(args, context.strings.icon);
        if(icon != NULL) {
            if(icon->type != ARG_STRING) {
                printf("Error: 'icon' must be a string.\n");
                return false;
            }
        }

        auto style_sheets = get_pointer(args, context.strings.style_sheets);
        if(style_sheets != NULL) {
            if(!is_list_of(*style_sheets, ARG_STRING, NULL)) {
                printf("Error: 'style_sheets' must be a list of strings.\n");
                return false;
            }
        }

        auto scripts = get_pointer(args, context.strings.scripts);
        if(scripts != NULL) {
            if(!is_list_of(*scripts, ARG_STRING, NULL)) {
                printf("Error: 'scripts' must be a list of strings.\n");
                return false;
            }
        }

    }
    // SUB div.
    else if(expr.type == context.strings.div) {
        auto type = get_pointer(args, context.strings.type);

        auto name = get_pointer(args, context.strings.name);
        auto body = get_pointer(args, context.strings.body);

        auto parameters = get_pointer(args, context.strings.parameters);

        if(expr.parent && name != NULL) {
            printf("Div definitions cannot be nested.\n");
            return false;
        }

        // NOTE(llw): Reference.
        if(type != NULL) {
            use_arg(context.strings.type);

            if(!is_string(type)) {
                printf("Error: Div type must be a string.\n");
                return false;
            }

            // NOTE(llw): Existence.
            auto symbol = get_pointer(context.symbols[SYMBOL_DIV], type->value);
            if(symbol == NULL) {
                printf("Div does not exist.\n");
                return false;
            }

            const auto &def = *symbol->expression;
            auto parameters = get_pointer(def.arguments, context.strings.parameters);
            if(parameters) {
                const auto &list = parameters->list;
                for(Usize i = 0; i < list.count; i += 1) {
                    auto name = list[i].value;
                    if(!has(args, name)) {
                        printf("Parameter not provided.\n");
                        return false;
                    }

                    use_arg(name);
                }
            }
        }
        // NOTE(llw): Definition.
        else if(name != NULL) {
            use_arg(context.strings.name);
            use_arg(context.strings.body);
            use_arg(context.strings.parameters);

            if(parameters) {
                if(!is_list_of(*parameters, ARG_ATOM, NULL)) {
                    printf("Error: 'parameters' must be a list of atoms.\n");
                    return false;
                }
            }
        }
        // NOTE(llw): Instance.
        else if(body != NULL) {
            use_arg(context.strings.body);

            if(parameters != NULL) {
                printf("Div instances cannot have parameters.\n");
                return false;
            }

            if(!validate_body(*body, vc, full_id)) {
                return false;
            }
        }
        else if(full_id == 0) {
            printf("Warning: Empty div.\n");
        }
    }
    // SUB text.
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
    // SUB spacer.
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
    // SUB default.
    else {
        auto type = context.string_table[expr.type];
        printf("Unrecognized expression type: '%s'\n", type.values);

        return false;
    }

    for(Usize i = 0; i < args.count; i += 1) {
        auto name = args.entries[i].key;

        if(!has(used_args, name)) {
            auto string = context.string_table[name];
            printf("Warning: Unused argument '%s'\n", string.values);
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

    auto vc = Validate_Context {};

    auto id_table = create_map<Interned_String, int>(context.arena);
    if(is_concrete_page(*symbol.expression)) {
        vc.id_table = &id_table;
        vc.id_prefix = context.strings.page;
    }

    symbol.state = SYMS_PROCESSING;
    auto success = validate(*symbol.expression, vc);
    symbol.state = SYMS_DONE;

    return success;
}


static bool insert_arguments(
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
                if(!insert_arguments(list[i], parameters)) {
                    return false;
                }
            }

            return true;
        } break;

        case ARG_BLOCK: {
            auto &block = arg.block;
            for(Usize i = 0; i < block.count; ) {
                auto &expr = block[i];
                auto &args = expr.arguments;

                // NOTE(llw): Expression is to be replaced completely.
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

                        auto amount = replace->block.count;
                        if(amount > 0) {
                            // NOTE(llw): Like push_at but removes block[i].
                            make_space_at(block, i, amount - 1);
                            copy_values(block.values + i, replace->block.values, amount);
                        }
                        else {
                            remove_at(block, i);
                        }

                        // NOTE(llw): Skip inserted expressions.
                        i += amount;
                        continue;
                    }
                    else {
                        printf("Cannot replace expression. Not an atom or block.\n");
                        return false;
                    }

                }

                // NOTE(llw): Replace expression arguments.
                for(Usize j = 0; j < args.count; j += 1) {
                    if(!insert_arguments(args.entries[j].value, parameters)) {
                        return false;
                    }
                }

                i += 1;
            }

            return true;
        } break;

        default: {
            assert(false);
            return false;
        } break;
    }
}

// - Both reference and definition are modified!
// - Remove parameter values, inherits/type from reference and insert into
//   definition. Remove name and parameters from definition.
// - Instantiate pages and references in definition.body (inherit arguments,
//   outermost writer wins).
static bool instantiate(
    Expression *reference,
    Expression &definition
) {
    TEMP_SCOPE(context.temporary);

    auto &args = definition.arguments;

    // NOTE(llw): Insert arguments.
    auto parameters = get_pointer(args, context.strings.parameters);
    if(parameters) {
        assert(reference != NULL);

        // NOTE(llw): Collect arguments from reference.
        auto arguments = create_map<Interned_String, Argument>(context.temporary);
        for(Usize i = 0; i < parameters->list.count; i += 1) {
            auto name = parameters->list[i].value;
            insert(arguments, name, reference->arguments[name]);

            // NOTE(llw): Remove argument from reference.
            remove(reference->arguments, name);
        }

        // NOTE(llw): Remove parameter list from definition.
        remove(args, context.strings.parameters);

        // NOTE(llw): Insert arguments into definition.
        for(Usize i = 0; i < args.count; i += 1) {
            auto &arg = args.entries[i].value;
            if(!insert_arguments(arg, arguments)) {
                return false;
            }
        }
    }

    // NOTE(llw): Instantiate references in body.
    auto body = get_pointer(args, context.strings.body);
    if(body) {
        auto &block = body->block;
        for(Usize expr_index = 0; expr_index < block.count; expr_index += 1) {
            auto &expr = block[expr_index];

            auto type = get_pointer(expr.arguments, context.strings.type);
            if(    type != 0
                && expr.type == context.strings.div
            ) {
                auto symbol = context.symbols[SYMBOL_DIV][type->value];
                auto instance = duplicate(*symbol.expression, context.arena);
                if(!instantiate(&expr, instance)) {
                    return false;
                }

                auto &inst_args = instance.arguments;
                auto &def_args  = expr.arguments;

                // NOTE(llw): Remove name/type.
                remove(inst_args, context.strings.name);
                remove(def_args, context.strings.type);

                // NOTE(llw): Merge.
                for(Usize i = 0; i < inst_args.count; i += 1) {
                    auto name = inst_args.entries[i].key;
                    auto value = inst_args.entries[i].value;
                    insert_maybe(def_args, name, value);
                }
            }
        }
    }

    // NOTE(llw): Recurse on pages.
    auto inherits = get_pointer(args, context.strings.inherits);
    if(    inherits != NULL
        && definition.type == context.strings.page
    ) {
        const auto &symbol = context.symbols[SYMBOL_PAGE][inherits->value];

        // NOTE(llw): Instantiate.
        auto instance = duplicate(*symbol.expression, context.arena);
        if(!instantiate(&definition, instance)) {
            return false;
        }

        auto &inst_args = instance.arguments;
        auto &def_args = definition.arguments;

        // NOTE(llw): Remove name/inherits.
        remove(inst_args, context.strings.name);
        remove(def_args, context.strings.inherits);

        // NOTE(llw): Merge.
        for(Usize i = 0; i < inst_args.count; i += 1) {
            auto name = inst_args.entries[i].key;
            auto &value = inst_args.entries[i].value;

            if(    name != context.strings.style_sheets
                && name != context.strings.scripts
            ) {
                insert_maybe(def_args, name, value);
            }
            else {
                auto &list = value.list;
                auto def_values = get_pointer(def_args, name);
                if(def_values) {
                    push(list, def_values->list);
                    def_values->list = list;
                }
            }
        }
    }

    return true;
}

static Expression *instantiate_page(const Expression &page) {
    auto result = allocate<Expression>(context.arena);
    *result = duplicate(page, context.arena);

    if(!instantiate(NULL, *result)) {
        return NULL;
    }

    auto symbol = Symbol {};
    symbol.expression = result;
    if(!validate(symbol)) {
        return NULL;
    }

    return result;
}

