#include "analyzer.hpp"
#include "context.hpp"

#include "stdio.h"

_inline bool is_definition(const Expression &expr) {
    auto result = has(expr.arguments, context.strings.defines);
    return result;
}

_inline bool is_generic(const Expression &expr) {
    auto result = has(expr.arguments, context.strings.parameters);
    return result;
}

_inline bool is_concrete(const Expression &expr) {
    auto result = !is_generic(expr) && !has(expr.arguments, context.strings.inherits);
    return result;
}

_inline bool is_identifier(String string) {
    if(string.size < 1) {
        return false;
    }

    auto first = string.values[0];
    if(!is_alpha(first) && first != '_') {
        return false;
    }

    for(Usize i = 1; i < string.size; i += 1) {
        auto at = string.values[i];
        if(!is_alpha(at) && !is_numeric(at) && at != '_') {
            return false;
        }
    }

    return true;
}


struct Validate_Context {
    Map<Interned_String, int> *id_table;
    Array<Interned_String>    *label_fors;
    Interned_String           id_prefix;
    bool in_form;
};

static bool validate(Symbol &symbol);
static Expression *instantiate(const Expression &expr);


bool analyze() {

    // NOTE(llw): Fill symbol table.
    for(Usize i = 0; i < context.expressions.count; i += 1) {
        auto &expr = context.expressions[i];

        auto defines = get_pointer(expr.arguments, context.strings.defines);
        if(defines == NULL) {
            printf("Not a definition.\n");
            return false;
        }
        if(defines->type != ARG_STRING) {
            printf("Definition names must be strings.\n");
            return false;
        }
        if(!is_identifier(context.string_table[defines->value])) {
            printf("Definition names must be identifiers.\n");
            return false;
        }

        auto symbol = Symbol {};
        symbol.expression = &expr;
        if(!insert_maybe(context.symbols, defines->value, symbol)) {
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

    // NOTE(llw): Instantiate non-generic symbols.
    for(Usize i = 0; i < context.symbols.count; i += 1) {

        auto &symbol = context.symbols.entries[i].value;
        if(is_generic(*symbol.expression)) {
            continue;
        }

        auto instance = instantiate(*symbol.expression);
        if(instance == NULL) {
            return false;
        }

        assert(is_concrete(*instance));

        push(context.exports, instance);
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

    auto validate_arg_list_p = [&](
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

        if(!is_list_of(*arg, type, NULL)) {
            printf("Error: Non-%s argument in list '%s'.\n",
                argument_type_strings[type],
                context.string_table[name].values
            );
            return false;
        }

        return true;
    };

    auto validate_arg_list = [&](Interned_String name, Argument_Type type, bool required) {
        auto arg = get_pointer(args, name);
        return validate_arg_list_p(name, type, required, arg);
    };



    auto defines    = get_pointer(args, context.strings.defines);
    auto parameters = get_pointer(args, context.strings.parameters);
    auto inherits   = get_pointer(args, context.strings.inherits);
    auto body       = get_pointer(args, context.strings.body);
    auto id         = get_pointer(args, context.strings.id);
    auto styles     = get_pointer(args, context.strings.styles);

    use_arg(context.strings.defines);
    use_arg(context.strings.parameters);
    use_arg(context.strings.inherits);
    use_arg(context.strings.body);
    use_arg(context.strings.id);

    auto concrete = parameters == NULL && inherits == NULL;


    // NOTE(llw): Type specific validation.

    // NOTE(llw): page.
    if(expr.type == context.strings.page) {
        if(id != NULL) {
            printf("Error: Pages cannot have ids.\n");
            return false;
        }

        if(    !validate_arg_type(context.strings.title,        ARG_STRING, false)
            || !validate_arg_type(context.strings.icon,         ARG_STRING, false)
            || !validate_arg_list(context.strings.style_sheets, ARG_STRING, false)
            || !validate_arg_list(context.strings.scripts,      ARG_STRING, false)
        ) {
            return false;
        }
    }
    // NOTE(llw): div.
    else if(expr.type == context.strings.div) {

        if(    defines  == NULL
            && inherits == NULL
            && body     == NULL
            && id       == NULL
            && styles   == NULL
        ) {
            printf("Error: Empty div.\n");
            return false;
        }

    }
    // NOTE(llw): form.
    else if(expr.type == context.strings.form) {

        if(expr.type == context.strings.form) {
            if(vc.in_form) {
                printf("Error: Forms cannot be nested.\n");
                return false;
            }
            vc.in_form = true;
        }

        if(    defines  == NULL
            && inherits == NULL
            && body     == NULL
            && id       == NULL
            && styles   == NULL
        ) {
            printf("Error: Empty form.\n");
            return false;
        }

    }
    // NOTE(llw): label.
    else if(expr.type == context.strings.label) {
        auto _for = get_pointer(args, context.strings._for);
        if(!validate_arg_type_p(context.strings._for, ARG_STRING, false, _for)) {
            return false;
        }

        if(_for != NULL && vc.id_prefix != 0) {
            auto referenced = make_full_id(vc.id_prefix, _for->value);
            push(*vc.label_fors, referenced);
        }
    }
    // NOTE(llw): input.
    else if(expr.type == context.strings.input) {
        if(id == NULL) {
            printf("Error: input requires an id.\n");
            return false;
        }

        auto type    = get_pointer(args, context.strings.type);
        auto min     = get_pointer(args, context.strings.min);
        auto max     = get_pointer(args, context.strings.max);
        auto initial = get_pointer(args, context.strings.initial);

        if(!validate_arg_type_p(context.strings.type, ARG_STRING, true, type)) {
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
            printf("Invalid input type.\n");
            return false;
        }
    }
    // NOTE(llw): text.
    else if(expr.type == context.strings.text) {
        if(id != NULL) {
            printf("Error: 'text' expressions cannot have ids.\n");
            return false;
        }

        if(!validate_arg_type(context.strings.value, ARG_STRING, true)) {
            return false;
        }
    }
    // NOTE(llw): simple types.
    else if(has(context.simple_types, expr.type)) {
        // ok.
    }
    // NOTE(llw): Unknown.
    else {
        auto type = context.string_table[expr.type];
        printf("Unrecognized expression type: '%s'\n", type.values);

        return false;
    }


    // NOTE(llw): Definitions.
    {
        if(expr.parent != NULL && is_definition(expr)) {
            printf("Error: Definitions cannot be nested.\n");
            return false;
        }
    }


    // NOTE(llw): Validate id.
    auto full_id = Interned_String {};
    if(id != NULL) {

        if(    id->type != ARG_STRING
            || id->value == context.strings.empty_string
        ) {
            printf("Error: ids must be non-empty strings.\n");
            return false;
        }

        auto is_global = false;
        auto ident = get_id_identifier(id->value, &is_global);
        if(!is_identifier(ident)) {
            printf("Error: ids must be identifiers.\n");
            return false;
        }

        // NOTE(llw): Check id uniqueness.
        if(vc.id_prefix != 0) {
            full_id = make_full_id(vc.id_prefix, ident, is_global);

            if(!insert_maybe(*vc.id_table, full_id, 0)) {
                printf("Error: Duplicate id.\n");
                return false;
            }
        }

    }

    // NOTE(llw): Validate parameters.
    if(parameters != NULL) {

        if(defines == NULL) {
            printf("Error: Parameter lists only allowed on definitions.\n");
            return false;
        }

        if(parameters->type != ARG_LIST) {
            printf("Error: Parameters must be a list.\n");
            return false;
        }

        TEMP_SCOPE(context.temporary);
        auto names = create_map<Interned_String, int>(context.temporary);

        // NOTE(llw): Unique atoms.
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

    }

    // NOTE(llw): Validate inheritance.
    if(inherits != NULL) {

        if(inherits->type != ARG_STRING) {
            printf("Error: 'inherits' must be a string.\n");
            return false;
        }

        // NOTE(llw): Existence.
        auto symbol = get_pointer(context.symbols, inherits->value);
        if(symbol == NULL) {
            printf("Error: Referenced symbol does not exist.\n");
            return false;
        }

        // NOTE(llw): Type.
        if(symbol->expression->type != expr.type) {
            printf("Error: Referenced symbol is of a different type.\n");
            return false;
        }

        // NOTE(llw): Recurse.
        if(!validate(*symbol)) {
            return false;
        }

        // NOTE(llw): Check parameters provided.
        auto parameters = get_pointer(symbol->expression->arguments, context.strings.parameters);
        if(parameters != NULL) {
            const auto &list = parameters->list;
            for(Usize i = 0; i < list.count; i += 1) {
                auto name = list[i].value;
                use_arg(name);

                if(!has(args, name)) {
                    printf("Parameter not provided.\n");
                    return false;
                }
            }
        }

    }

    // NOTE(llw): Validate body.
    if(concrete && body != NULL) {

        if(body->type != ARG_BLOCK) {
            printf("Error: Body must be a block.\n");
            return false;
        }

        if(full_id) {
            vc.id_prefix = full_id;
        }

        const auto &block = body->block;
        for(Usize i = 0; i < block.count; i += 1) {
            if(!validate(block[i], vc)) {
                return false;
            }
        }

    }


    // NOTE(llw): Styles.
    if(!validate_arg_list(context.strings.styles, ARG_STRING, false)) {
        return false;
    }


    // NOTE(llw): Unused arguments.
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

    const auto &expr = *symbol.expression;

    auto vc = Validate_Context {};
    auto id_table = create_map<Interned_String, int>(context.arena);
    auto label_fors = create_array<Interned_String>(context.arena);

    if(is_concrete(*symbol.expression)) {
        if(expr.type == context.strings.page) {
            vc.id_prefix = context.strings.page;
        }
        else {
            vc.id_prefix = context.strings.empty_string;
        }
        vc.id_table = &id_table;
        vc.label_fors = &label_fors;
    }

    symbol.state = SYMS_PROCESSING;
    auto success = validate(expr, vc);
    symbol.state = SYMS_DONE;

    if(!success) {
        return false;
    }

    for(Usize i = 0; i < label_fors.count; i += 1) {
        auto id = label_fors[i];
        if(!has(id_table, id)) {
            printf("Error: id referenced by label does not exist.\n");
            return false;
        }
    }

    return true;
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

/* NOTE(llw):
    - Both reference and definition are modified!
    - Removes parameter values from reference and inserts them into definition.
    - Recurses on definition.body expressions and definition itself.
    - "On way up", merging takes place:
        - For style_sheets, scripts, styles lists: Concatenate.
        - Others: Outermost writer wins.
*/
static bool instantiate(
    Expression *reference,
    Expression &definition
) {
    TEMP_SCOPE(context.temporary);

    auto instantiate_and_merge = [](Expression &reference, Interned_String inherits) {
        auto symbol = context.symbols[inherits];
        auto instance = duplicate(*symbol.expression, context.arena);
        if(!instantiate(&reference, instance)) {
            return false;
        }

        auto &inst_args = instance.arguments;
        auto &def_args  = reference.arguments;

        // NOTE(llw): Remove defines/inherits.
        remove(inst_args, context.strings.defines);
        remove(def_args, context.strings.inherits);

        // NOTE(llw): Merge.
        for(Usize i = 0; i < inst_args.count; i += 1) {
            auto name = inst_args.entries[i].key;
            auto value = inst_args.entries[i].value;

            if(    name == context.strings.style_sheets
                || name == context.strings.scripts
                || name == context.strings.styles
            ) {
                auto &list = value.list;
                auto def_values = get_pointer(def_args, name);
                if(def_values) {
                    push(list, def_values->list);
                    def_values->list = list;
                }
                else {
                    insert(def_args, name, value);
                }
            }
            else {
                insert_maybe(def_args, name, value);
            }
        }

        return true;
    };

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

            auto inherits = get_pointer(expr.arguments, context.strings.inherits);
            if(inherits != NULL) {
                if(!instantiate_and_merge(expr, inherits->value)) {
                    return false;
                }
            }
        }
    }

    // NOTE(llw): Recurse.
    auto inherits = get_pointer(args, context.strings.inherits);
    if(inherits != NULL) {
        if(!instantiate_and_merge(definition, inherits->value)) {
            return false;
        }
    }

    return true;
}

static Expression *instantiate(const Expression &expr) {
    auto result = allocate<Expression>(context.arena);
    *result = duplicate(expr, context.arena);

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

