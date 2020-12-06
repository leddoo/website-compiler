#include "codegen.hpp"
#include "context.hpp"


static void write_file(Interned_String name, String extension, const Array<U8> &buffer) {
    TEMP_SCOPE(context.temporary);

    auto path = create_array<U8>(context.temporary);
    push(path, STRING("build/"));
    push(path, name);
    push(path, extension);
    push(path, (U8)0);

    write_entire_file((char *)path.values, buffer);
}

static Array<U8> generate_html(const Expression &page);

static void generate_instantiation_js(
    const Expression &expr,
    Array<U8> &buffer, Usize indent,
    bool parent_is_tree_node
);

void codegen() {

    auto instantiate_js = create_array<U8>(context.arena);
    reserve(instantiate_js, MEBI(1));

    for(Usize i = 0; i < context.exports.count; i += 1) {
        const auto &expr = *context.exports[i];
        auto defines = expr.arguments[context.strings.defines].value;

        if(expr.type == context.strings.page) {
            auto html = generate_html(expr);
            write_file(defines, STRING(".html"), html);
        }
        else {
            push(instantiate_js, STRING("function make_"));
            push(instantiate_js, defines);
            push(instantiate_js, STRING("(parent) {\n"));

            // TEMP(llw): This really is just a sanity check. You can only call
            // make_* on tree nodes, which always have an id, unless there's a
            // bug in the compiler.
            push(instantiate_js, STRING("    console.assert(parent.dom.id != \"\");\n\n"));

            push(instantiate_js, STRING("    let dom = parent.dom;\n"));
            push(instantiate_js, STRING("    let me  = parent;\n"));

            push(instantiate_js, STRING("\n    {\n"));
            generate_instantiation_js(expr, instantiate_js, 2, true);
            push(instantiate_js, STRING("    }\n"));

            push(instantiate_js, STRING("}\n\n"));
        }
    }

    write_file(
        intern(context.string_table, STRING("instantiate")),
        STRING(".js"),
        instantiate_js
    );
}



static void do_indent(Array<U8> &buffer, Usize indent) {
    for(Usize i = 0; i < indent; i += 1) {
        push(buffer, STRING("    "));
    }
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
    if(id != NULL) {
        auto is_global = false;
        auto ident = get_id_identifier(id->value, &is_global);

        full_id = make_full_id(parent, ident, is_global);
        parent = full_id;
    }

    auto id_string = create_array<U8>(context.temporary);
    if(full_id != 0) {
        push(id_string, STRING(" id=\""));
        push(id_string, full_id);
        push(id_string, STRING("\""));

        do_indent(init_js, init_js_indent);
        push(init_js, STRING("window."));
        push(init_js, full_id);
        push(init_js, STRING(" = { dom: document.getElementById(\""));
        push(init_js, full_id);
        push(init_js, STRING("\") };\n"));
    }

    auto styles_string = create_array<U8>(context.temporary);
    auto styles = get_pointer(args, context.strings.styles);
    if(styles != NULL) {
        push(styles_string, STRING(" class=\""));

        const auto &list = styles->list;
        for(Usize i = 0; i < list.count; i += 1) {
            push(styles_string, list[i].value);

            if(i < list.count - 1) {
                push(styles_string, STRING(" "));
            }
        }

        push(styles_string, STRING("\""));
    }


    auto begin_simple_element = [&](Interned_String type) {
        do_indent(html, html_indent);
        push(html, STRING("<"));
        push(html, type);
        push(html, id_string);
        push(html, styles_string);
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
    else if(expr.type == context.strings.form_field) {
        auto title = args[context.strings.title].value;
        auto type  = args[context.strings.type].value;

        auto initial = get_pointer(args, context.strings.initial);

        do_indent(html, html_indent);
        push(html, STRING("<label for=\""));
        push(html, full_id);
        push(html, STRING("\">"));
        push(html, title);
        push(html, STRING("</label>\n"));

        do_indent(html, html_indent);
        push(html, STRING("<input"));
        push(html, id_string);
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

    push(init_js, STRING(
        "        window.page = { dom: document.getElementById(\"page\") };\n"
    ));

    const auto &children = page.arguments[context.strings.body].block;
    for(Usize i = 0; i < children.count; i += 1) {
        generate_html(
            children[i],
            context.strings.page,
            html, 2,
            init_js, 2
        );
    }

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
    Array<U8> &buffer, Usize indent,
    bool parent_is_tree_node
) {
    const auto &args = expr.arguments;

    auto id = get_pointer(args, context.strings.id);
    auto identifier = String {};
    auto is_global_id = false;

    if(id != NULL) {
        identifier = get_id_identifier(id->value, &is_global_id);
    }


    auto begin_element = [&]() {
        push(buffer, STRING("\n"));
        do_indent(buffer, indent);
        push(buffer, STRING("{\n"));
        indent += 1;
    };

    auto end_element = [&]() {
        indent -= 1;
        do_indent(buffer, indent);
        push(buffer, STRING("}\n"));
    };

    auto write_parent_variables = [&]() {
        do_indent(buffer, indent);
        push(buffer, STRING("let my_dom_parent = dom;\n"));

        do_indent(buffer, indent);
        if(is_global_id) {
            push(buffer, STRING("let my_tree_parent = window.page;\n"));
        }
        else if(parent_is_tree_node) {
            push(buffer, STRING("let my_tree_parent = me;\n"));
        }
        else {
            push(buffer, STRING("// same tree parent.\n"));
        }

        if(id != 0) {
            push(buffer, STRING("\n"));

            do_indent(buffer, indent);
            push(buffer, STRING("console.assert(my_tree_parent[\""));
            push(buffer, identifier);
            push(buffer, STRING("\"] === undefined);\n"));

            do_indent(buffer, indent);
            push(buffer, STRING("let full_id = my_tree_parent.dom.id + \"."));
            push(buffer, identifier);
            push(buffer, STRING("\";\n"));
        }
    };

    auto write_create_dom = [&](String type, bool takes_id, bool takes_styles) {
        push(buffer, STRING("\n"));

        do_indent(buffer, indent);
        push(buffer, STRING("let dom = document.createElement(\""));
        push(buffer, type);
        push(buffer, STRING("\");\n"));

        do_indent(buffer, indent);
        push(buffer, STRING("my_dom_parent.append(dom);\n"));

        if(takes_id && id != 0) {
            do_indent(buffer, indent);
            push(buffer, STRING("dom.id = full_id;\n"));
        }

        auto styles = get_pointer(args, context.strings.styles);
        if(takes_styles && styles != NULL) {
            const auto &list = styles->list;
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

        if(id != 0) {
            do_indent(buffer, indent);
            push(buffer, STRING("let me = {};\n"));

            do_indent(buffer, indent);
            push(buffer, STRING("my_tree_parent[\""));
            push(buffer, identifier);
            push(buffer, STRING("\"] = me;\n"));

            do_indent(buffer, indent);
            push(buffer, STRING("me.dom = dom;\n"));
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
                begin_element();
                generate_instantiation_js(
                    children[i],
                    buffer, indent,
                    id != 0
                );
                end_element();
            }
        }
    };

    auto write_simple_element = [&](String type) {
        write_parent_variables();
        write_create_dom(type, true, true);
        write_create_tree_node();
        write_body();
    };


    if(expr.type == context.strings.div) {
        write_simple_element(STRING("div"));
    }
    else if(expr.type == context.strings.form) {
        write_simple_element(STRING("form"));
    }
    else if(expr.type == context.strings.form_field) {
        auto title = args[context.strings.title].value;
        auto type  = args[context.strings.type].value;

        auto initial = get_pointer(args, context.strings.initial);

        write_parent_variables();

        begin_element();
        {
            write_create_dom(STRING("label"), false, false);

            do_indent(buffer, indent);
            push(buffer, STRING("dom.htmlFor = full_id\n"));

            do_indent(buffer, indent);
            push(buffer, STRING("dom.innerHTML = \""));
            push(buffer, title);
            push(buffer, STRING("\"\n"));
        }
        end_element();

        begin_element();
        {
            write_create_dom(STRING("input"), true, true);

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
        }
        end_element();
    }
    else if(expr.type == context.strings.text) {
        auto value = args[context.strings.value].value;

        do_indent(buffer, indent);
        push(buffer, STRING("let text = document.createTextNode(\""));
        push(buffer, value);
        push(buffer, STRING("\");\n"));

        do_indent(buffer, indent);
        push(buffer, STRING("dom.append(text);\n"));
    }
    else if(has(context.simple_types, expr.type)) {
        write_simple_element(context.string_table[expr.type]);
    }
    else {
        assert(false);
    }

}

