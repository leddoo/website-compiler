#include "analyzer.hpp"
#include "context.hpp"

#include "stdio.h"


_inline bool is_definition(const Expression &expr) {
    return has(expr.arguments, context.strings.name);
}

_inline bool is_generic(const Expression &expr) {
    return has(expr.arguments, context.strings.parameters);
}

_inline bool is_page(const Expression &expr) {
    return expr.type == context.strings.page;
}

_inline bool is_concrete_page(const Expression &expr) {
    return is_page(expr)
        && !is_generic(expr)
        && !has(expr.arguments, context.strings.inherits);
}


struct Validate_Context {
    Map<Interned_String, int> *id_table;
    Interned_String id_prefix;
    bool in_form;
};

static bool validate(Symbol &symbol);
static Expression *instantiate_page(const Expression &page);


bool analyze() {

    // NOTE(llw): Fill symbol table.
    for(Usize i = 0; i < context.expressions.count; i += 1) {
        auto &expr = context.expressions[i];

        auto name = get_pointer(expr.arguments, context.strings.name);
        if(name == NULL) {
            printf("Definitions require names.\n");
            return false;
        }
        if(name->type != ARG_STRING) {
            printf("Names must be strings.\n");
            return false;
        }

        auto type = Symbol_Type {};
        if(expr.type == context.strings.page) {
            type = SYMBOL_PAGE;
        }
        else if(expr.type == context.strings.div) {
            type = SYMBOL_DIV;
        }
        else if(expr.type == context.strings.form) {
            type = SYMBOL_FORM;
        }
        else {
            printf("Unrecognized definition type.\n");
            return false;
        }

        auto symbol = Symbol {};
        symbol.expression = &expr;
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

        if(!is_generic(*symbol.expression)) {
            auto instance = instantiate_page(*symbol.expression);
            if(instance == NULL) {
                return false;
            }

            assert(is_concrete_page(*instance));

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


static bool validate(const Expression &expr, Validate_Context vc) {

    TEMP_SCOPE(context.temporary);

    const auto &args = expr.arguments;

    auto used_args = create_map<Interned_String, int>(context.temporary);
    auto use_arg = [&](Interned_String arg) {
        insert_maybe(used_args, arg, 0);
    };


    auto validate_arg_type_p = [&](
        Interned_String name, Argument_Type type,
        bool required,
        const Argument *arg
    ) {
        use_arg(name);

        if(arg == NULL) {
            if(required) {
                printf("Error: Missing required argument '%s'.\n",
                    context.string_table[name].values
                );
                return false;
            }
            return true;
        }

        if(arg->type != type) {
            printf("Error: '%s' must be a %s\n",
                context.string_table[name].values,
                argument_type_strings[type]
            );
            return false;
        }

        return true;
    };

    auto validate_arg_type = [&](Interned_String name, Argument_Type type, bool required) {
        auto arg = get_pointer(args, name);
        return validate_arg_type_p(name, type, required, arg);
    };

    auto validate_arg_list = [&](Interned_String name, Argument_Type type, bool required) {
        use_arg(name);

        auto arg = get_pointer(args, name);
        if(arg == NULL) {
            if(required) {
                printf("Error: Missing required argument '%s'.\n",
                    context.string_table[name].values
                );
                return false;
            }
            return true;
        }

        if(!is_list_of(*arg, type, NULL)) {
            printf("Error: Non-%s argument in list '%s'.\n",
                argument_type_strings[type],
                context.string_table[name].values
            );
            return false;
        }

        return true;
    };

    auto validate_parameters = [&]() {
        use_arg(context.strings.parameters);

        auto parameters = get_pointer(args, context.strings.parameters);
        if(parameters == NULL) {
            return true;
        }

        if(parameters->type != ARG_LIST) {
            printf("Error: Parameters must be a list.\n");
            return false;
        }

        TEMP_SCOPE(context.temporary);
        auto names = create_map<Interned_String, int>(context.temporary);

        const auto &list = parameters->list;
        for(Usize i = 0; i < list.count; i += 1) {
            const auto &name = list[i];

            if(name.type != ARG_ATOM) {
                printf("Error: Parameter list must only contain atoms.\n");
                return false;
            }

            if(!insert_maybe(names, name.value, 0)) {
                printf("Error: Parameter declared multiple times.\n");
                return false;
            }
        }

        return true;
    };

    auto validate_reference = [&](Interned_String name, Symbol_Type type) {
        // NOTE(llw): Existence.
        auto symbol = get_pointer(context.symbols[type], name);
        if(symbol == NULL) {
            printf("Referenced symbol does not exist.\n");
            return false;
        }

        // NOTE(llw): Recurse.
        if(!validate(*symbol)) {
            return false;
        }

        // NOTE(llw): Check parameters provided.
        auto parameters = get_pointer(symbol->expression->arguments, context.strings.parameters);
        if(parameters) {
            const auto &params = parameters->list;
            for(Usize i = 0; i < params.count; i += 1) {
                auto param_name = params[i].value;
                if(!has(args, param_name)) {
                    printf("Parameter not provided.\n");
                    return false;
                }
                use_arg(param_name);
            }
        }

        return true;
    };

    auto validate_body = [](
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
    if(    lid != NULL && lid->value == context.strings.empty_string
        || gid != NULL && gid->value == context.strings.empty_string
    ) {
        printf("Ids must not be empty strings.\n");
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



    if(expr.parent != NULL && is_definition(expr)) {
        printf("Error: Definitions cannot be nested.\n");
        return false;
    }


    // SUB page.
    if(expr.type == context.strings.page) {
        use_arg(context.strings.name);
        use_arg(context.strings.body);

        if(expr.parent != NULL) {
            printf("Error: Pages cannot be nested.\n");
            return false;
        }

        if(has_id) {
            printf("Error: Pages cannot have ids.\n");
            return false;
        }

        // NOTE(llw): Check generic.
        if(!validate_parameters()) {
            return false;
        }

        // NOTE(llw): Check inheritance.
        auto inherits = get_pointer(args, context.strings.inherits);
        if(inherits) {
            use_arg(context.strings.inherits);

            if(inherits->type != ARG_STRING) {
                printf("'inherits' requires a string.\n");
                return false;
            }

            if(!validate_reference(inherits->value, SYMBOL_PAGE)) {
                return false;
            }
        }

        // NOTE(llw): Check body.
        if(is_concrete_page(expr)) {
            auto body = get_pointer(args, context.strings.body);
            if(body == NULL) {
                printf("Concrete pages require a body.\n");
                return false;
            }

            if(!validate_body(*body, vc, 0)) {
                return false;
            }
        }

        if(    !validate_arg_type(context.strings.title, ARG_STRING, false)
            || !validate_arg_type(context.strings.icon, ARG_STRING, false)
            || !validate_arg_list(context.strings.style_sheets, ARG_STRING, false)
            || !validate_arg_list(context.strings.scripts, ARG_STRING, false)
        ) {
            return false;
        }
    }
    // SUB div/form.
    else if(expr.type == context.strings.div
        || expr.type == context.strings.form
    ) {
        use_arg(context.strings.body);

        if(expr.type == context.strings.form) {
            if(vc.in_form) {
                printf("Error: Forms cannot be nested.\n");
                return false;
            }
            vc.in_form = true;
        }

        auto type = get_pointer(args, context.strings.type);

        auto name = get_pointer(args, context.strings.name);
        auto body = get_pointer(args, context.strings.body);

        // NOTE(llw): Definition.
        if(name != NULL) {
            use_arg(context.strings.name);
            use_arg(context.strings.parameters);

            if(!validate_parameters()) {
                return false;
            }
        }
        // NOTE(llw): Reference.
        else if(type != NULL) {
            use_arg(context.strings.type);

            if(!is_string(type)) {
                printf("Error: 'type' must be a string.\n");
                return false;
            }

            auto symbol_type = SYMBOL_DIV;
            if(expr.type == context.strings.form) {
                symbol_type = SYMBOL_FORM;
            }
            else {
                assert(expr.type == context.strings.div);
            }

            if(!validate_reference(type->value, symbol_type)) {
                return false;
            }
        }
        // NOTE(llw): Instance.
        else if(body != NULL) {
            if(!validate_body(*body, vc, full_id)) {
                return false;
            }
        }
        else if(full_id == 0) {
            printf("Error: Empty %s.\n", context.string_table[expr.type].values);
            return false;
        }
    }
    // SUB form field.
    else if(expr.type == context.strings.form_field) {
        if(!vc.in_form) {
            printf("Error: form_field needs to be inside a form.\n");
            return false;
        }

        if(!full_id) {
            printf("Error: form_field requires an id.\n");
            return false;
        }

        auto type    = get_pointer(args, context.strings.type);
        auto min     = get_pointer(args, context.strings.min);
        auto max     = get_pointer(args, context.strings.max);
        auto locked  = get_pointer(args, context.strings.locked);
        auto initial = get_pointer(args, context.strings.initial);

        if(    !validate_arg_type_p(context.strings.type, ARG_STRING, true, type)
            || !validate_arg_type  (context.strings.title, ARG_STRING, true)
            || !validate_arg_type_p(context.strings.locked, ARG_NUMBER, false, locked)
        ) {
            return false;
        }

        // NOTE(llw): Type specific validation.
        if(    type->value == context.strings.text
            || type->value == context.strings.email
        ) {
            if(    !validate_arg_type_p(context.strings.min, ARG_NUMBER, false, min)
                || !validate_arg_type_p(context.strings.max, ARG_NUMBER, false, max)
                || !validate_arg_type_p(context.strings.initial, ARG_STRING, false, initial)
            ) {
                return false;
            }
        }
        else {
            printf("Invalid form field type.\n");
            return false;
        }
    }
    // SUB text.
    else if(expr.type == context.strings.text) {
        auto type = get_pointer(args, context.strings.type);

        if(    !validate_arg_type_p(context.strings.type, ARG_STRING, true, type)
            || !validate_arg_type  (context.strings.value, ARG_STRING, true)
        ) {
            return false;
        }

        if(!has(context.valid_text_types, type->value)) {
            printf("Invalid text type.\n");
            return false;
        }
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
            printf("Error: Unused argument '%s'\n", string.values);
            return false;
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
            if(type != 0 && (
                   expr.type == context.strings.div
                || expr.type == context.strings.form
            )) {
                auto symbol_type = SYMBOL_DIV;
                if(expr.type == context.strings.form) {
                    symbol_type = SYMBOL_FORM;
                }
                else {
                    assert(expr.type == context.strings.div);
                }

                auto symbol = context.symbols[symbol_type][type->value];
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

