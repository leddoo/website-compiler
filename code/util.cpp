#include "util.hpp"
#include "context.hpp"

#pragma warning(disable:4996) // crt secure
#include "cstdio"

#include <libcpp/util/defer.hpp>


//
// RANGE files.
//

bool read_entire_file(const char *path, Array<U8> &buffer, bool null_terminate) {
    auto f = fopen(path, "rb");
    if(f == NULL) { return false; }
    defer { fclose(f); };

    if(fseek(f, 0, SEEK_END) != 0) { return false; }

    auto size = (Usize)ftell(f);
    if(size == (Usize)-1L) { return false; }
    set_count(buffer, size + null_terminate);

    if(fseek(f, 0, SEEK_SET) != 0) { return false; }

    auto bytes_read = fread(buffer.values, 1, size, f);
    if(bytes_read != size) { return false; }

    if(null_terminate) {
        last(buffer) = 0;
    }

    return true;
}

bool write_entire_file(const char *path, const Array<U8> &buffer) {
    auto f = fopen(path, "wb");
    if(f == NULL) { return false; }
    defer { fclose(f); };

    auto bytes_written = fwrite(buffer.values, 1, buffer.count, f);
    if(bytes_written != buffer.count) { return false; }

    return true;
}



//
// RANGE reader.
//

bool read_quoted_string(Reader<U8> &reader, String &result) {
    if(reader.current >= reader.end) {
        return false;
    }
    auto old_current = reader.current;

    if(*reader.current != '"') {
        return false;
    }
    reader.current += 1;

    auto begin = reader.current;
    while(reader.current < reader.end) {
        auto at = *reader.current;
        if(at == '\\') {
            reader.current += 1;
        }
        else if(at == '"') {
            auto end = reader.current;
            result = str(begin, end);
            reader.current += 1;
            return true;
        }

        reader.current += 1;
    }

    reader.current = old_current;
    return false;
}

void skip_whitespace(Reader<U8> &reader) {
    while(reader.current < reader.end) {
        auto at = *reader.current;
        if(    at != ' '
            && at != '\t'
        ) {
            break;
        }

        reader.current += 1;
    }
}

void skip_whitespace(
    Reader<U8> &reader,
    Unsigned &current_line, const U8 *&line_begin
) {
    while(reader.current < reader.end) {
        auto at = *reader.current;
        if(    at != ' '
            && at != '\t'
            && at != '\r'
            && at != '\n'
        ) {
            break;
        }

        if(at == '\n') {
            current_line += 1;
            line_begin = reader.current + 1;
        }

        reader.current += 1;
    }
}



//
// RANGE string table.
//

String_Table create_string_table(libcpp::Allocator &allocator) {
    auto result = String_Table {};
    result.allocator = &allocator;
    result.table.allocator = &allocator;
    result.reverse.allocator = &allocator;
    return result;
}

Interned_String intern(String_Table &table, String string) {
    auto pointer = get_pointer(table.table, string);

    if(pointer == NULL) {
        table.previous_id += 1;
        auto id = table.previous_id;

        auto s = allocate_array_uninitialized<U8>(string.size + 1, *table.allocator);
        copy_bytes(s, string.values, string.size);
        s[string.size] = 0;

        string.values = s;

        insert(table.table, string, id);
        insert(table.reverse, id, string);
        return id;
    }

    return *pointer;
}

Interned_String intern(String_Table &table, const char *string) {
    auto s = String { (U8 *)string, (Usize)strlen(string) };
    auto result = intern(table, s);
    return result;
}

Interned_String find_first_file(
    const Array<Interned_String> &include_paths,
    String file_name
) {
    for(Usize i = 0; i < include_paths.count; i += 1) {
        TEMP_SCOPE(context.temporary);
        auto buffer = create_array<U8>(context.temporary);

        auto prefix = context.string_table[include_paths[i]];
        push(buffer, prefix);
        push(buffer, file_name);
        push(buffer, (U8)0);

        auto f = fopen((char *)buffer.values, "rb");
        if(f != NULL) {
            fclose(f);

            auto result = intern(context.string_table, str(buffer));
            return result;
        }
    }

    return 0;
}


bool parse_int_maybe(String string, U64 &result) {
    if(string.size < 1) {
        return false;
    }

    result = 0;

    for(Usize i = 0; i < string.size; i += 1) {
        auto at = string.values[i];
        if(!is_numeric(at)) {
            return false;
        }

        auto new_result = 10*result + (at - '0');
        if(new_result < result) {
            return false;
        }

        result = new_result;
    }

    return true;
}

U64 parse_int(String string) {
    U64 result;
    auto ok = parse_int_maybe(string, result);
    assert(ok);
    return result;
}

void serialize_int(U64 value, Array<U8> &buffer) {
    clear(buffer);
    reserve(buffer, 20);

    if(value == 0) {
        push(buffer, (U8)'0');
        return;
    }

    while(value != 0) {
        auto digit = value % 10;
        value      = value / 10;

        push(buffer, (U8)('0' + (char)digit));
    }

    reverse(buffer);
}

