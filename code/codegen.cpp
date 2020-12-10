#include "codegen.hpp"
#include "context.hpp"


static void do_indent(Array<U8> &buffer, Usize indent) {
    for(Usize i = 0; i < indent; i += 1) {
        push(buffer, STRING("    "));
    }
}


static void add_output_file(Interned_String name, String extension, const Array<U8> &buffer) {
    TEMP_SCOPE(context.temporary);

    auto path = create_array<U8>(context.temporary);
    push(path, context.output_prefix);
    push(path, name);
    push(path, extension);

    auto source = Source {};
    source.file_path = intern(context.string_table, str(path));
    source.content = buffer;
    push(context.outputs, source);
}

static Array<U8> generate_html(const Expression &page);

static void generate_instantiation_js(
    const Expression &expr,
    Array<U8> &buffer, Usize indent
);

void codegen() {

    auto instantiate_js = create_array<U8>(context.arena);
    reserve(instantiate_js, MEBI(1));

    for(Usize i = 0; i < context.exports.count; i += 1) {
        const auto &expr = *context.exports[i];
        auto defines = expr.arguments[context.strings.defines].value;

        if(expr.type == context.strings.page) {
            auto html = generate_html(expr);
            add_output_file(defines, STRING(".html"), html);
        }
        else {
            push(instantiate_js, STRING("function tn_make_"));
            push(instantiate_js, defines);
            push(instantiate_js, STRING("(parent) {\n"));

            do_indent(instantiate_js, 1);
            push(instantiate_js, STRING("console.assert(parent.tn_dom.id != \"\");\n\n"));

            do_indent(instantiate_js, 1);
            push(instantiate_js, STRING("let dom = parent.tn_dom;\n"));

            do_indent(instantiate_js, 1);
            push(instantiate_js, STRING("let me  = parent;\n"));

            generate_instantiation_js(expr, instantiate_js, 1);

            push(instantiate_js, STRING("}\n\n"));
        }
    }

    add_output_file(
        intern(context.string_table, STRING("instantiate")),
        STRING(".js"),
        instantiate_js
    );
}



static void push_list(Array<U8> &buffer, const Array<Argument> &list, String separator) {
    push(buffer, STRING("\""));
    for(Usize i = 0; i < list.count; i += 1) {
        push(buffer, list[i].value);

        if(i < list.count - 1) {
            push(buffer, separator);
        }
    }
    push(buffer, STRING("\""));
}

static void generate_html(
    const Expression &expr,
    Interned_String parent,
    Array<U8> &html, Usize html_indent,
    Array<U8> &init_js, Usize init_js_indent
) {
    TEMP_SCOPE(context.temporary);

    const auto &args = expr.arguments;

    auto id = get_pointer(args, context.strings.id);
    auto full_id = Interned_String {};

    auto id_string = create_array<U8>(context.temporary);
    if(id != NULL) {
        auto is_global = false;
        auto identifier = get_id_identifier(id->value, &is_global);
        full_id = make_full_id(parent, identifier, is_global);
        parent = full_id;

        push(id_string, STRING(" id=\""));
        push(id_string, full_id);
        push(id_string, STRING("\""));

        // NOTE(llw): id setup code 1/2.
        do_indent(init_js, init_js_indent);
        push(init_js, STRING("\n"));

        if(is_global) {
            do_indent(init_js, init_js_indent);
            push(init_js, STRING("var my_tree_parent = window.page;\n"));
        }
        else {
            do_indent(init_js, init_js_indent);
            push(init_js, STRING("var my_tree_parent = me;\n"));
        }

        do_indent(init_js, init_js_indent);
        push(init_js, STRING("{\n"));
        init_js_indent += 1;

        do_indent(init_js, init_js_indent);
        push(init_js, STRING("let me = new Tree_Node(my_tree_parent, document.getElementById(\""));
        push(init_js, full_id);
        push(init_js, STRING("\"), \""));
        push(init_js, identifier);
        push(init_js, STRING("\");\n"));
    }

    auto css_string = create_array<U8>(context.temporary);

    auto classes = get_pointer(args, context.strings.classes);
    if(classes != NULL) {
        push(css_string, STRING(" class="));
        push_list(css_string, classes->list, STRING(" "));
    }

    auto styles = get_pointer(args, context.strings.styles);
    if(styles != NULL) {
        push(css_string, STRING(" style="));
        push_list(css_string, styles->list, STRING("; "));
    }


    auto begin_simple_element = [&](Interned_String type) {
        do_indent(html, html_indent);
        push(html, STRING("<"));
        push(html, type);
        push(html, id_string);
        push(html, css_string);
        push(html, STRING(">\n"));
    };

    auto end_element = [&](Interned_String type) {
        do_indent(html, html_indent);
        push(html, STRING("</"));
        push(html, type);
        push(html, STRING(">\n"));
    };

    auto write_body = [&]() {
        auto body = get_pointer(args, context.strings.body);
        if(body == NULL) {
            return;
        }

        const auto &children = body->block;
        for(Usize i = 0; i < children.count; i += 1) {
            generate_html(
                children[i], parent,
                html, html_indent + 1,
                init_js, init_js_indent
            );
        }
    };

    auto write_simple_element = [&](Interned_String type) {
        begin_simple_element(type);
        write_body();
        end_element(type);
    };


    if(expr.type == context.strings.div) {
        write_simple_element(context.strings.div);
    }
    else if(expr.type == context.strings.form) {
        write_simple_element(context.strings.form);
    }
    else if(expr.type == context.strings.list) {
        write_simple_element(context.strings.div);
    }
    else if(expr.type == context.strings.select) {
        begin_simple_element(context.strings.select);

        const auto &options = args[context.strings.options].block;
        for(Usize i = 0; i < options.count; i += 1) {
            generate_html(
                options[i], parent,
                html, html_indent + 1,
                init_js, init_js_indent
            );
        }

        end_element(context.strings.select);
    }
    else if(expr.type == context.strings.option) {
        auto value = get_pointer(args, context.strings.value);

        do_indent(html, html_indent);
        push(html, STRING("<option"));
        push(html, id_string);
        push(html, css_string);
        if(value != NULL) {
            push(html, STRING(" value=\""));
            push(html, value->value);
            push(html, STRING("\""));
        }
        push(html, STRING(">\n"));

        auto text = args[context.strings.text];
        do_indent(html, html_indent + 1);
        push(html, text.value);
        push(html, STRING("\n"));

        end_element(context.strings.option);
    }
    else if(expr.type == context.strings.label) {
        auto _for = get_pointer(args, context.strings._for);

        do_indent(html, html_indent);
        push(html, STRING("<label"));
        push(html, id_string);
        push(html, css_string);
        if(_for != NULL) {
            push(html, STRING(" for=\""));
            push(html, make_full_id(parent, _for->value));
            push(html, STRING("\""));
        }
        push(html, STRING(">\n"));

        write_body();
        end_element(context.strings.label);
    }
    else if(expr.type == context.strings.input) {
        auto type  = args[context.strings.type].value;
        auto initial = get_pointer(args, context.strings.initial);

        do_indent(html, html_indent);
        push(html, STRING("<input"));
        push(html, id_string);
        push(html, css_string);
        push(html, STRING(" type=\""));
        push(html, type);
        push(html, STRING("\""));
        if(initial) {
            push(html, STRING(" value=\""));
            push(html, initial->value);
            push(html, STRING("\""));
        }
        push(html, STRING(">\n"));
    }
    else if(expr.type == context.strings.text) {
        auto value = args[context.strings.value].value;

        do_indent(html, html_indent);
        push(html, value);
        push(html, STRING("\n"));
    }
    else if(has(context.simple_types, expr.type)) {
        write_simple_element(expr.type);
    }
    else {
        assert(false);
    }


    if(expr.type == context.strings.list) {
        auto type_string = args[context.strings.type].value;
        auto min_string  = intern(context.string_table, STRING("-Infinity"));
        auto max_string  = intern(context.string_table, STRING("+Infinity"));

        auto min = get_pointer(args, context.strings.min);
        if(min != NULL) { min_string = min->value; }

        auto max = get_pointer(args, context.strings.max);
        if(max != NULL) { max_string = max->value; }

        do_indent(init_js, init_js_indent);
        push(init_js, STRING("me.tn_listify("));
        push(init_js, STRING("tn_make_"));
        push(init_js, type_string);
        push(init_js, STRING(", "));
        push(init_js, min_string);
        push(init_js, STRING(", "));
        push(init_js, max_string);
        push(init_js, STRING(");\n"));
    }

    if(full_id != 0) {
        // NOTE(llw): id setup code 2/2.
        init_js_indent -= 1;
        do_indent(init_js, init_js_indent);
        push(init_js, STRING("}\n"));
    }

}

static Array<U8> generate_html(const Expression &page) {
    assert(page.type == context.strings.page);

    auto html = create_array<U8>(context.arena);
    reserve(html, MEBI(1));

    auto init_js = create_array<U8>(context.arena);
    reserve(init_js, MEBI(1));

    push(html, STRING(
        "<!DOCTYPE html>\n"
        "<html lang=\"de\">\n"
        "\n"
        "<head>\n"
        "    <meta charset=\"UTF-8\">\n"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "    <meta http-equiv=\"X-UA-Compatible\" content=\"ie=edge\">\n"
    ));

    auto title = get_pointer(page.arguments, context.strings.title);
    if(title != NULL) {
        do_indent(html, 1);
        push(html, STRING("<title>"));
        push(html, title->value);
        push(html, STRING("</title>\n"));
    }

    auto icon = get_pointer(page.arguments, context.strings.icon);
    if(icon != NULL) {
        do_indent(html, 1);
        push(html, STRING("<link rel=\"icon\" href=\""));
        push(html, icon->value);
        push(html, STRING("\">\n"));
    }

    auto style_sheets = get_pointer(page.arguments, context.strings.style_sheets);
    if(style_sheets != NULL) {
        const auto &list = style_sheets->list;
        for(Usize i = 0; i < list.count; i += 1) {
            do_indent(html, 1);
            push(html, STRING("<link rel=\"stylesheet\" href=\""));
            push(html, list[i].value);
            push(html, STRING("\">\n"));
        }
    }

    auto scripts = get_pointer(page.arguments, context.strings.scripts);
    if(scripts != NULL) {
        const auto &list = scripts->list;
        for(Usize i = 0; i < list.count; i += 1) {
            do_indent(html, 1);
            push(html, STRING("<script src=\""));
            push(html, list[i].value);
            push(html, STRING("\"></script>\n"));
        }
    }

    push(html, STRING(
        "</head>\n"
        "\n"
        "<body>\n"
        "    <div id=\"page\">\n"
    ));

    do_indent(init_js, 2);
    push(init_js, STRING("(function() {\n"));

    do_indent(init_js, 3);
    push(init_js, STRING(
        "let me = new Tree_Node(null, document.getElementById(\"page\"), \"page\");\n"
    ));
    do_indent(init_js, 3);
    push(init_js, STRING("window.page = me;\n"));

    const auto &children = page.arguments[context.strings.body].block;
    for(Usize i = 0; i < children.count; i += 1) {
        generate_html(
            children[i],
            context.strings.page,
            html, 2,
            init_js, 3
        );
    }

    do_indent(init_js, 2);
    push(init_js, STRING("})();\n"));

    push(html, STRING(
        "    </div>\n"
        "    <script>\n"
    ));
    push(html, init_js);
    push(html, STRING(
        "    </script>\n"
        "</body>\n"
        "\n"
        "</html>\n"
        "\n"
    ));

    return html;
}


static void generate_instantiation_js(
    const Expression &expr,
    Array<U8> &buffer, Usize indent
) {
    const auto &args = expr.arguments;

    auto id = get_pointer(args, context.strings.id);
    auto identifier = String {};
    auto is_global_id = false;
    if(id != NULL) {
        identifier = get_id_identifier(id->value, &is_global_id);
    }


    auto write_parent_variables = [&]() {
        push(buffer, STRING("\n"));

        do_indent(buffer, indent);
        push(buffer, STRING("var my_dom_parent = dom;\n"));

        if(is_global_id) {
            do_indent(buffer, indent);
            push(buffer, STRING("var my_tree_parent = window.page;\n"));
        }
        else {
            do_indent(buffer, indent);
            push(buffer, STRING("var my_tree_parent = me;\n"));
        }

    };

    auto begin_element = [&]() {
        do_indent(buffer, indent);
        push(buffer, STRING("{\n"));
        indent += 1;
    };

    auto end_element = [&]() {
        indent -= 1;
        do_indent(buffer, indent);
        push(buffer, STRING("}\n"));
    };

    auto write_create_dom = [&](String type) {
        do_indent(buffer, indent);
        push(buffer, STRING("let dom = document.createElement(\""));
        push(buffer, type);
        push(buffer, STRING("\");\n"));

        do_indent(buffer, indent);
        push(buffer, STRING("my_dom_parent.append(dom);\n"));

        auto styles = get_pointer(args, context.strings.styles);
        if(styles != NULL) {
            do_indent(buffer, indent);
            push(buffer, STRING("dom.style = "));
            push_list(buffer, styles->list, STRING("; "));
            push(buffer, STRING(";\n"));
        }

        auto classes = get_pointer(args, context.strings.classes);
        if(classes != NULL) {
            const auto &list = classes->list;
            for(Usize i = 0; i < list.count; i += 1) {
                do_indent(buffer, indent);
                push(buffer, STRING("dom.classList.add(\""));
                push(buffer, list[i].value);
                push(buffer, STRING("\");\n"));
            }
        }
    };

    auto write_create_tree_node = [&]() {
        push(buffer, STRING("\n"));

        if(id != NULL) {
            do_indent(buffer, indent);
            push(buffer, STRING("let me = new Tree_Node(my_tree_parent, dom, \""));
            push(buffer, identifier);
            push(buffer, STRING("\");\n"));
        }
        else {
            do_indent(buffer, indent);
            push(buffer, STRING("// no tree node.\n"));
        }
    };

    auto write_body = [&]() {
        auto body = get_pointer(args, context.strings.body);
        if(body != NULL) {
            const auto &children = body->block;
            for(Usize i = 0; i < children.count; i += 1) {
                generate_instantiation_js(
                    children[i],
                    buffer, indent
                );
            }
        }
    };

    auto write_simple_element = [&](String type) {
        write_parent_variables();
        begin_element();
        write_create_dom(type);
        write_create_tree_node();
        write_body();
        end_element();
    };


    if(expr.type == context.strings.div) {
        write_simple_element(STRING("div"));
    }
    else if(expr.type == context.strings.form) {
        write_simple_element(STRING("form"));
    }
    else if(expr.type == context.strings.list) {
        auto type_string = args[context.strings.type].value;

        write_parent_variables();
        begin_element();

        write_create_dom(STRING("div"));
        write_create_tree_node();

        // NOTE(llw): listify.
        do_indent(buffer, indent);
        push(buffer, STRING("me.tn_listify("));
        push(buffer, STRING("tn_make_"));
        push(buffer, type_string);
        push(buffer, STRING(", -Infinity, +Infinity);\n"));

        // NOTE(llw): initial.
        auto initial = get_pointer(args, context.strings.initial);
        if(initial) {
            do_indent(buffer, indent);
            push(buffer, STRING("for(let i = 0; i < "));
            push(buffer, initial->value);
            push(buffer, STRING("; i += 1) {\n"));

            do_indent(buffer, indent + 1);
            push(buffer, STRING("me.tn_list_insert_new();\n"));

            do_indent(buffer, indent);
            push(buffer, STRING("}\n"));
        }

        auto min = get_pointer(args, context.strings.min);
        if(min != NULL) {
            do_indent(buffer, indent);
            push(buffer, STRING("me.tn_list_min = "));
            push(buffer, min->value);
            push(buffer, STRING(";\n"));
        }

        auto max = get_pointer(args, context.strings.max);
        if(max != NULL) {
            do_indent(buffer, indent);
            push(buffer, STRING("me.tn_list_max = "));
            push(buffer, max->value);
            push(buffer, STRING(";\n"));
        }

        end_element();
    }
    else if(expr.type == context.strings.select) {
        write_parent_variables();
        begin_element();
        write_create_dom(STRING("select"));
        write_create_tree_node();

        const auto &options = args[context.strings.options].block;
        for(Usize i = 0; i < options.count; i += 1) {
            generate_instantiation_js(
                options[i],
                buffer, indent
            );
        }

        end_element();
    }
    else if(expr.type == context.strings.option) {
        write_parent_variables();
        begin_element();
        write_create_dom(STRING("option"));

        auto value = get_pointer(args, context.strings.value);
        if(value != NULL) {
            do_indent(buffer, indent);
            push(buffer, STRING("dom.value = \""));
            push(buffer, value->value);
            push(buffer, STRING("\";\n"));
        }

        auto text = args[context.strings.text];
        do_indent(buffer, indent);
        push(buffer, STRING("let text = document.createTextNode(\""));
        push(buffer, text.value);
        push(buffer, STRING("\");\n"));
        do_indent(buffer, indent);
        push(buffer, STRING("dom.append(text);\n"));

        end_element();
    }
    else if(expr.type == context.strings.label) {
        auto _for = get_pointer(args, context.strings._for);

        write_parent_variables();
        begin_element();
        write_create_dom(STRING("label"));

        if(_for != NULL) {
            auto global = false;
            auto ident = get_id_identifier(_for->value, &global);

            do_indent(buffer, indent);
            if(global) {
                push(buffer, STRING("dom.htmlFor = \"page-"));
            }
            else {
                push(buffer, STRING("dom.htmlFor = my_tree_parent.tn_dom.id + \"-"));
            }
            push(buffer, ident);
            push(buffer, STRING("\";\n"));
        }

        write_create_tree_node();
        write_body();
        end_element();
    }
    else if(expr.type == context.strings.input) {
        auto type = args[context.strings.type].value;
        auto initial = get_pointer(args, context.strings.initial);

        write_parent_variables();
        begin_element();
        write_create_dom(STRING("input"));

        do_indent(buffer, indent);
        push(buffer, STRING("dom.type = \""));
        push(buffer, type);
        push(buffer, STRING("\";\n"));

        if(initial != NULL) {
            do_indent(buffer, indent);
            push(buffer, STRING("dom.value = \""));
            push(buffer, initial->value);
            push(buffer, STRING("\";\n"));
        }

        write_create_tree_node();
        write_body();
        end_element();
    }
    else if(expr.type == context.strings.text) {
        auto value = args[context.strings.value].value;

        begin_element();

        do_indent(buffer, indent);
        push(buffer, STRING("let text = document.createTextNode(\""));
        push(buffer, value);
        push(buffer, STRING("\");\n"));

        do_indent(buffer, indent);
        push(buffer, STRING("dom.append(text);\n"));

        end_element();
    }
    else if(has(context.simple_types, expr.type)) {
        write_simple_element(context.string_table[expr.type]);
    }
    else {
        assert(false);
    }

}

