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

static void generate_html(
    Array<U8> &buffer,
    const Expression &expr,
    Interned_String parent = 0,
    Usize indent = 0
);


void codegen() {

    // NOTE(llw): Generate html for pages.
    for(Usize i = 0; i < context.pages.count; i += 1) {
        const auto &page = *context.pages[i];
        auto name = page.arguments[context.strings.name].value;
        auto buffer = create_array<U8>(context.arena);

        generate_html(buffer, page);

        write_file(name, STRING(".html"), buffer);
    }
}


const String page_html_1 = STRING(
    "<!DOCTYPE html>\n"
    "<html lang = \"de\">\n"
    "\n"
    "<head>\n"
    "    <meta charset = \"UTF-8\">\n"
    "    <meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0\">\n"
    "    <meta http-equiv = \"X-UA-Compatible\" content = \"ie=edge\">\n"
);

const String page_html_2 = STRING(
    "</head>\n"
    "\n"
    "<body>\n"
);

const String page_html_3 = STRING(
    "</body>\n"
    "\n"
);


static void generate_html(
    Array<U8> &buffer,
    const Expression &expr,
    Interned_String parent,
    Usize indent
) {
    TEMP_SCOPE(context.temporary);

    auto do_indent = [&](Usize offset = 0) {
        for(Usize i = 0; i < indent + offset; i += 1) {
            push(buffer, STRING("    "));
        }
    };


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
    }

    auto id_string = create_array<U8>(context.temporary);
    if(full_id != 0) {
        push(id_string, STRING(" id = \""));
        push(id_string, full_id);
        push(id_string, STRING("\""));
    }


    do_indent();

    if(expr.type == context.strings.page) {

        push(buffer, page_html_1);
        indent += 1;

        auto title = get_pointer(args, context.strings.title);
        if(title != NULL) {
            do_indent();
            push(buffer, STRING("<title>"));
            push(buffer, title->value);
            push(buffer, STRING("</title>\n"));
        }

        auto icon = get_pointer(args, context.strings.icon);
        if(icon != NULL) {
            do_indent();
            push(buffer, STRING("<link rel = \"icon\" href = \""));
            push(buffer, icon->value);
            push(buffer, STRING("\">\n"));
        }

        auto style_sheets = get_pointer(args, context.strings.style_sheets);
        if(style_sheets != NULL) {
            const auto &list = style_sheets->list;
            for(Usize i = 0; i < list.count; i += 1) {
                do_indent();
                push(buffer, STRING("<link rel = \"stylesheet\" href = \""));
                push(buffer, list[i].value);
                push(buffer, STRING("\">\n"));
            }
        }

        auto scripts = get_pointer(args, context.strings.scripts);
        if(scripts != NULL) {
            const auto &list = scripts->list;
            for(Usize i = 0; i < list.count; i += 1) {
                do_indent();
                push(buffer, STRING("<script src = \""));
                push(buffer, list[i].value);
                push(buffer, STRING("\"></script>\n"));
            }
        }

        push(buffer, page_html_2);

        do_indent();
        push(buffer, STRING("<div id = \"page\">\n"));

        const auto &children = args[context.strings.body].block;
        for(Usize i = 0; i < children.count; i += 1) {
            generate_html(buffer, children[i], context.strings.page, indent + 1);
        }

        do_indent();
        push(buffer, STRING("</div>\n"));

        push(buffer, page_html_3);
    }
    else if(expr.type == context.strings.div) {
        push(buffer, STRING("<div"));
        push(buffer, id_string);
        push(buffer, STRING(">\n"));

        auto body = get_pointer(args, context.strings.body);
        if(body != NULL) {
            const auto &children = body->block;
            for(Usize i = 0; i < children.count; i += 1) {
                generate_html(buffer, children[i], full_id, indent + 1);
            }
        }

        do_indent();
        push(buffer, STRING("</div>\n"));
    }
    else if(expr.type == context.strings.text) {
        auto type  = args[context.strings.type].value;
        auto value = args[context.strings.value].value;

        push(buffer, STRING("<"));
        push(buffer, type);
        push(buffer, id_string);
        push(buffer, STRING(">\n"));

        do_indent(1);
        push(buffer, value);
        push(buffer, STRING("\n"));

        do_indent();
        push(buffer, STRING("</"));
        push(buffer, type);
        push(buffer, STRING(">\n"));
    }
    else if(expr.type == context.strings.spacer) {
        if(has(args, context.strings.value)) {
            auto value = args[context.strings.value].value;

            push(buffer, STRING("<div style = \"spacer_"));
            push(buffer, value);
            push(buffer, STRING("\"></div>\n"));
        }
        else {
            auto desktop = args[context.strings.desktop].value;
            auto mobile  = args[context.strings.mobile].value;

            push(buffer, STRING("<div style = \"spacer_"));
            push(buffer, desktop);
            push(buffer, STRING(" desktop\"></div>\n"));

            do_indent();
            push(buffer, STRING("<div style = \"spacer_"));
            push(buffer, mobile);
            push(buffer, STRING(" mobile\"></div>\n"));
        }
    }
    else {
        assert(false);
    }
}

