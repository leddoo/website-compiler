#include <Windows.h>
#include <intrin.h>

#include <libcpp/memory/allocator.hpp>
#include <libcpp/memory/string.hpp>
#include <libcpp/util/defer.hpp>

namespace libcpp {

    void _default_assert(String condition, String source_location) {
        auto size = source_location.size + 2 + condition.size + 2;

        auto buffer = allocate_array_uninitialized<U8>(size);
        defer { free(buffer); };

        auto cursor = buffer;
        copy_bytes(cursor, source_location.values, source_location.size);
        cursor += source_location.size;

        *cursor = '\n'; cursor += 1;
        *cursor = '"'; cursor += 1;

        copy_bytes(cursor, condition.values, condition.size);
        cursor += condition.size;

        *cursor = '"'; cursor += 1;
        *cursor = 0;  cursor += 1;

        MessageBox(
            NULL,
            (const char *)buffer,
            "Assertion failed.",
            MB_OK | MB_ICONERROR
        );

        ExitProcess(1);

        //__debugbreak();
    }

}

