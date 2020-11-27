#pragma once

#include <libcpp/memory/array.hpp>
#include <libcpp/memory/map.hpp>
#include <libcpp/memory/string.hpp>
#include <libcpp/memory/allocator.hpp>

using namespace libcpp;

//
// RANGE files.
//

bool read_entire_file(
    const char *path,
    Array<U8> &buffer,
    bool null_terminate = false
);

bool write_entire_file(
    const char *path,
    const Array<U8> &buffer
);



// Reader.

template <typename T>
struct Reader {
    const T *current, *end;
};

bool read_quoted_string(Reader<U8> &reader, String &result);

void skip_whitespace(Reader<U8> &reader);
void skip_whitespace(
    Reader<U8> &reader,
    Unsigned &current_line, const U8 *&line_begin
);

_inline Reader<U8> make_reader(const Array<U8> &array) {
    auto result = Reader<U8> {
        array.values,
        array.values + array.count,
    };
    return result;
}

_inline Reader<U8> make_reader(String string) {
    auto result = Reader<U8> {
        string.values,
        string.values + string.size,
    };
    return result;
}




//
// RANGE string table.
//

using Interned_String = U32;

struct String_Table {
    Map<String, Interned_String> table;
    Map<Interned_String, String> reverse;
    Interned_String previous_id;

    String operator[](Interned_String key) const {
        return reverse[key];
    }
};

String_Table create_string_table(Allocator &allocator = default_allocator);

Interned_String intern(String_Table &table, String string);



//
// RANGE other.
//

_inline bool is_alpha(U8 c) {
    return c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z';
}

_inline bool is_numeric(U8 c) {
    return c >= '0' && c <= '9';
}

_inline String str(const U8 *begin, const U8 *end) {
    auto result = String {
        (U8 *)begin,
        (Usize)(end - begin)
    };
    return result;
}

_inline String str(Array<U8> array) {
    auto result = String {
        array.values,
        array.count,
    };
    return result;
}

_inline void push_bytes(Array<U8> &array, const U8 *src, Usize size) {
    auto old_count = array.count;
    set_count(array, array.count + size);
    auto dest = array.values + old_count;
    copy_values(dest, src, size);
}

_inline void push(Array<U8> &array, String string) {
    push_bytes(array, string.values, string.size);
}

