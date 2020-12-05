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
        auto name = expr.arguments[context.strings.name].value;

        if(expr.type == context.strings.page) {
            auto html = generate_html(expr);
            write_file(name, STRING(".html"), html);
        }
        else {
            push(instantiate_js, STRING("function make_"));
            push(instantiate_js, name);
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

    auto full_id = Interned_String {};
    {
        auto lid = get_pointer(args, context.strings.id);
        auto gid = get_pointer(args, context.strings.global_id);

        if(lid != NULL) {
            full_id = make_full_id_from_lid(parent, lid->value);
        }
        else if(gid != NULL) {
            full_id = make_full_id_from_gid(gid->value);
        }

        if(full_id) {
            parent = full_id;
        }
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


    if(expr.type == context.strings.div) {
        do_indent(html, html_indent);
        push(html, STRING("<div"));
        push(html, id_string);
        push(html, STRING(">\n"));

        auto body = get_pointer(args, context.strings.body);
        if(body != NULL) {
            const auto &children = body->block;
            for(Usize i = 0; i < children.count; i += 1) {
                generate_html(
                    children[i], parent,
                    html, html_indent + 1,
                    init_js, init_js_indent
                );
            }
        }

        do_indent(html, html_indent);
        push(html, STRING("</div>\n"));
    }
    else if(expr.type == context.strings.form) {
        do_indent(html, html_indent);
        push(html, STRING("<form"));
        push(html, id_string);
        push(html, STRING(">\n"));

        auto body = get_pointer(args, context.strings.body);
        if(body != NULL) {
            const auto &children = body->block;
            for(Usize i = 0; i < children.count; i += 1) {
                generate_html(
                    children[i], parent,
                    html, html_indent + 1,
                    init_js, init_js_indent
                );
            }
        }

        do_indent(html, html_indent);
        push(html, STRING("</form>\n"));
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
        auto type  = args[context.strings.type].value;
        auto value = args[context.strings.value].value;

        do_indent(html, html_indent);
        push(html, STRING("<"));
        push(html, type);
        push(html, id_string);
        push(html, STRING(">\n"));

        do_indent(html, html_indent + 1);
        push(html, value);
        push(html, STRING("\n"));

        do_indent(html, html_indent);
        push(html, STRING("</"));
        push(html, type);
        push(html, STRING(">\n"));
    }
    else if(expr.type == context.strings.spacer) {
        if(has(args, context.strings.value)) {
            auto value = args[context.strings.value].value;

            do_indent(html, html_indent);
            push(html, STRING("<div style=\"spacer_"));
            push(html, value);
            push(html, STRING("\"></div>\n"));
        }
        else {
            auto desktop = args[context.strings.desktop].value;
            auto mobile  = args[context.strings.mobile].value;

            do_indent(html, html_indent);
            push(html, STRING("<div style=\"spacer_"));
            push(html, desktop);
            push(html, STRING(" desktop\"></div>\n"));

            do_indent(html, html_indent);
            push(html, STRING("<div style=\"spacer_"));
            push(html, mobile);
            push(html, STRING(" mobile\"></div>\n"));
        }
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

    auto lid = get_pointer(args, context.strings.id);
    auto gid = get_pointer(args, context.strings.global_id);

    auto id = Interned_String {};
    {
        if(lid != NULL) {
            id = lid->value;
        }
        else if(gid != NULL) {
            id = gid->value;
        }
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
        if(gid != NULL) {
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
            push(buffer, id);
            push(buffer, STRING("\"] === undefined);\n"));

            do_indent(buffer, indent);
            push(buffer, STRING("let full_id = my_tree_parent.dom.id + \"."));
            push(buffer, id);
            push(buffer, STRING("\";\n"));
        }
    };

    auto write_create_dom = [&](String type, bool takes_id) {
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
    };

    auto write_create_tree_node = [&]() {
        push(buffer, STRING("\n"));

        if(id != 0) {
            do_indent(buffer, indent);
            push(buffer, STRING("let me = {};\n"));

            do_indent(buffer, indent);
            push(buffer, STRING("my_tree_parent[\""));
            push(buffer, id);
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
            assert(expr.type == context.strings.div
                || expr.type == context.strings.form
            );

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
        write_create_dom(type, true);
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
            write_create_dom(STRING("label"), false);

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
            write_create_dom(STRING("input"), true);

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
        auto type  = args[context.strings.type].value;
        auto value = args[context.strings.value].value;

        write_parent_variables();
        write_create_dom(context.string_table[type], true);

        do_indent(buffer, indent);
        push(buffer, STRING("dom.innerHTML = \""));
        push(buffer, value);
        push(buffer, STRING("\";\n"));

        write_create_tree_node();
    }
    else if(expr.type == context.strings.spacer) {
        write_parent_variables();

        if(has(args, context.strings.value)) {
            auto value = args[context.strings.value].value;

            begin_element();
            {
                write_create_dom(STRING("div"), true);

                do_indent(buffer, indent);
                push(buffer, STRING("dom.classList.add(\"spacer_"));
                push(buffer, value);
                push(buffer, STRING("\");\n"));
            }
            end_element();
        }
        else {
            auto desktop = args[context.strings.desktop].value;
            auto mobile  = args[context.strings.mobile].value;

            begin_element();
            {
                write_create_dom(STRING("div"), true);

                do_indent(buffer, indent);
                push(buffer, STRING("dom.classList.add(\"desktop\");\n"));

                do_indent(buffer, indent);
                push(buffer, STRING("dom.classList.add(\"spacer_"));
                push(buffer, desktop);
                push(buffer, STRING("\");\n"));
            }
            end_element();

            begin_element();
            {
                write_create_dom(STRING("div"), true);

                do_indent(buffer, indent);
                push(buffer, STRING("dom.classList.add(\"mobile\");\n"));

                do_indent(buffer, indent);
                push(buffer, STRING("dom.classList.add(\"spacer_"));
                push(buffer, mobile);
                push(buffer, STRING("\");\n"));
            }
            end_element();
        }
    }
    else {
        assert(false);
    }

}

