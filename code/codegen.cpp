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

void codegen() {

    // NOTE(llw): Generate html for pages.
    for(Usize i = 0; i < context.exports[SYMBOL_PAGE].count; i += 1) {
        const auto &page = *context.exports[SYMBOL_PAGE][i];

        auto name = page.arguments[context.strings.name].value;
        auto html = generate_html(page);
        write_file(name, STRING(".html"), html);
    }
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

