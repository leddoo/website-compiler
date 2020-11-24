#include "util.hpp"

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
    result.table.allocator = &allocator;
    result.reverse.allocator = &allocator;
    return result;
}

Interned_String intern(String_Table &table, String string) {
    auto pointer = get_pointer(table.table, string);

    if(pointer == NULL) {
        table.previous_id += 1;
        auto id = table.previous_id;
        insert(table.table, string, id);
        insert(table.reverse, id, string);
        return id;
    }

    return *pointer;
}

