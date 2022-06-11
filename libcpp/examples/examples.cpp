#include <libcpp/util/assert.hpp>
#include <libcpp/util/defer.hpp>
#include <libcpp/util/math.hpp>

#include <libcpp/memory/arena.hpp>
#include <libcpp/memory/array.hpp>
#include <libcpp/memory/map.hpp>
#include <libcpp/memory/set.hpp>
#include <libcpp/memory/heap.hpp>


using namespace libcpp;

#include <cstdio>

void test_assert(String condition, String source_location) {
    printf("Assertion failed (%s) at \"%s\".\n", condition.values, source_location.values);
}

void util_assert() {
    printf("\n--- util/assert ---\n");
    push_assert(test_assert);
    assert(false);
    pop_assert();
}

void util_defer() {
    printf("\n--- util/defer ---\n");
    defer { printf("1\n"); };
    defer { printf("2\n"); };
}

void util_math() {
    printf("\n--- util/math ---\n");
    printf("clamp(1, 2, 3) = %d\n", clamp(1, 2, 3));
    printf("clamp(2, 2, 3) = %d\n", clamp(2, 2, 3));
    printf("clamp(4, 2, 3) = %d\n", clamp(4, 2, 3));
    printf("\n");

    printf("abs(-1) = %d\n", abs(-1));
    printf("abs(22) = %d\n", abs(22));
    printf("\n");

    printf("leading_zeros(0) = %Id\n", leading_zeros(0));
    printf("leading_zeros(1) = %Id\n", leading_zeros(1));
    printf("leading_zeros(1 << 62) = %Id\n", leading_zeros(1ull << 62));
    printf("leading_zeros(-1) = %Id\n", leading_zeros((U64)-1));
    printf("leading_zeros(1 << 31) = %Id\n", leading_zeros(1ull << 31));
    printf("\n");

    printf("is_power_of_two(0) = %d\n", is_power_of_two(0));
    printf("is_power_of_two(1) = %d\n", is_power_of_two(1));
    printf("is_power_of_two(2) = %d\n", is_power_of_two(2));
    printf("is_power_of_two(3) = %d\n", is_power_of_two(3));
    printf("is_power_of_two(4) = %d\n", is_power_of_two(4));
    printf("\n");

    printf("next_power_of_two(0) = %d\n", next_power_of_two(0));
    printf("next_power_of_two(1) = %d\n", next_power_of_two(1));
    printf("next_power_of_two(2) = %d\n", next_power_of_two(2));
    printf("next_power_of_two(3) = %d\n", next_power_of_two(3));
    printf("next_power_of_two(4) = %d\n", next_power_of_two(4));
}

void memory_arena() {
    printf("\n--- memory/arena ---\n");

    auto arena = create_arena();
    defer {
        printf("destroying arena, used: %zd\n", arena.used);
        destroy(arena);
    };

    printf("created arena, used: %zd\n", arena.used);

    allocate<int>(arena);
    printf("allocated int, used: %zd\n", arena.used);

    {
        defer { printf("left temp scope, used: %zd\n", arena.used); };
        TEMP_SCOPE(arena);
        printf("entering temp scope, used: %zd\n", arena.used);

        auto foo = allocate<int>(arena);
        *foo = 42;
        printf("allocated int (%d), used: %zd\n", *foo, arena.used);

        auto size = 2*arena.block_size;
        allocate_uninitialized(size, 8, arena);
        printf("allocated a lot (%zd), used: %zd\n", size, arena.used);
    }

    auto foo = allocate_uninitialized<int>(arena);
    printf("allocated int uninit, value: %d, used: %zd\n", *foo, arena.used);
}

void print_array(const char *name, const Array<int> &array) {
    printf("%s = [", name);
    for(Usize i = 0; i < array.count; i += 1) {
        printf(" %d", array[i]);
    }
    printf(" ]");
}

void memory_array() {
    printf("\n--- memory/array ---\n");

    auto array = create_array<int>();
    defer { destroy(array); };

    auto other = create_array<int>();
    defer { destroy(other); };

    const auto print_arrays = [&]() {
        print_array("array", array);
        printf(" ");
        print_array("other", other);
        printf("\n");
    };

    print_arrays();
    printf("eq(array, other) = %d\n", eq(array, other));
    printf("\n");

    printf("push(Array, T)\n");
    push(array, 1);
    push(other, 2);
    print_arrays();
    printf("eq(array, other) = %d\n", eq(array, other));
    printf("\n");

    printf("push(Array, Array)\n");
    push(array, other); print_arrays();
    push(array, array); print_arrays();
    printf("\n");

    printf("push_at\n");
    push_front(array, 0); print_arrays();
    push_at(array, 3, 3); print_arrays();
    printf("\n");

    printf("push_unique\n");
    for(Usize i = 0; i < 2; i += 1) {
        auto inserted = push_unique(other, 42);
        printf("inserted: %d ", inserted);
        print_arrays();
    }
    printf("\n");

    clear(other);
    push(other, array);

    printf("removal\n");
    remove_swap(array, 3);      print_arrays();
    pop(array);                 print_arrays();
    remove_at(array, 3);        print_arrays();
    remove_leading(array, 3);   print_arrays();
    remove_at(other, 0);        print_arrays();
    remove_at(other, 4);        print_arrays();
    remove_swap(other, 3);      print_arrays();
    printf("\n");

    printf("util\n");
    auto third = duplicate(other);
    print_array("third", third); printf("\n");

    auto sub = sub_array(third, 1, 2);
    print_array("sub", sub); printf("\n");

    printf("remaining_capacity(other): %zd\n", remaining_capacity(other));
    printf("data_size(other): %zd\n", data_size(other));
    reset(other);
    printf("remaining capacity after reset: %zd\n", remaining_capacity(other));
    printf("\n");

    printf("count manipulation\n");
    push(array, 1); print_arrays();
    set_count(array, 4, 42); print_arrays();
    add_count(array, 1, 123); print_arrays();
    set_min_count(array, 10, 10); print_arrays();
    printf("\n");
}

void print_map(const char *name, const Map<int, int> &map) {
    printf("%s = {", name);
    for(Usize i = 0; i < map.count; i += 1) {
        auto entry = map.entries[i];
        printf(" %d: %d", entry.key, entry.value);
        if(i < map.count - 1) {
            printf(",");
        }
    }
    printf(" }\n");
}

void memory_map() {
    printf("\n--- memory/map ---\n");

    auto map = create_map<int, int>();
    defer { destroy(map); };

    printf("insert\n");
    insert(map, 4, 1);
    insert(map, 6, 8);
    insert(map, 8, 1);
    insert_maybe(map, 6, 0);
    print_map("map", map);
    printf("\n");

    printf("access\n");
    map[4] = 2;
    map[6] += 1;
    print_map("map", map);
    printf("\n");

    printf("remove\n");
    printf("remove_maybe(map, 1): %d\n", remove_maybe(map, 1));
    print_map("map", map);
    printf("remove(map, 4)\n"); remove(map, 4);
    print_map("map", map);
    printf("\n");

    printf("has\n");
    printf("has(map, 4): %d\n", has(map, 4));
    printf("has(map, 6): %d\n", has(map, 6));
    printf("\n");

    printf("util\n");

    printf("duplicate: ");
    auto other = duplicate(map);
    print_map("other", other);

    printf("clear: ");
    clear(other);
    print_map("other", other);

    printf("insert some values: ");
    insert(other, 1, 2);
    insert(other, 3, 4);
    print_map("other", other);

    printf("move to third: ");
    auto third = duplicate(other);
    print_map("third", third);

    printf("reset: ");
    reset(other);
    print_map("other", other);

    printf("insert: ");
    insert(other, third);
    insert(other, map);
    print_map("other", other);

    printf("other.capacity: %zd\n", other.capacity);
    printf("grow by 3\n"); grow_by(other, 3);
    printf("other.capacity: %zd\n", other.capacity);

    printf("\n");
}

void print_set(const char *name, const Set<int> &set) {
    printf("%s = {", name);
    for(Usize i = 0; i < set.count; i += 1) {
        printf(" %d", set.entries[i].key);
        if(i < set.count - 1) {
            printf(",");
        }
    }
    printf(" }\n");
}

void memory_set() {
    printf("\n--- memory/set ---\n");

    auto set = create_set<int>();
    defer { destroy(set); };

    printf("insert\n");
    insert(set, 4);
    insert(set, 6);
    insert(set, 8);
    insert_maybe(set, 6);
    print_set("set", set);
    printf("\n");

    printf("remove\n");
    printf("remove_maybe(set, 1): %d\n", remove_maybe(set, 1));
    print_set("set", set);
    printf("remove(set, 4)\n"); remove(set, 4);
    print_set("set", set);
    printf("\n");

    printf("has\n");
    printf("has(set, 4): %d\n", has(set, 4));
    printf("has(set, 6): %d\n", has(set, 6));
    printf("\n");

    auto other = create_set<int>();
    defer { destroy(other); };

    printf("insert\n");
    insert(other, 1);
    insert(other, 2);
    insert(other, 4);
    insert(other, 8);
    print_set("other", other);
    printf("insert_maybe(set, other): %zd\n", insert_maybe(set, other));
    print_set("set", set);
    printf("\n");
}

void memory_heap() {
    printf("\n--- memory/heap ---\n");

    auto heap = create_heap(default_allocator, 4096, 8);
    defer { destroy(heap); };

    auto do_check = [&]() {
        auto s = Heap_Statistics {};
        auto ok = check(heap, &s);

        printf("Heap statistics:\n"
            "  block_count: %zd\n"
            "  total_overhead: %zd (%.2f%%)\n"
            "  total_used: %zd (%.2f%%)\n"
            "  total_allocated: %zd\n"
            "  used_chunk_count: %zd (%.2f%%)\n"
            "  free_chunk_count: %zd (%.2f%%)\n"
            "  total_chunk_count: %zd\n",
            s.block_count,
            s.total_overhead, (F64)s.total_overhead/(F64)s.total_allocated*100.0,
            s.total_used, (F64)s.total_used/(F64)s.total_allocated*100.0,
            s.total_allocated,
            s.used_chunk_count, (F64)s.used_chunk_count/(F64)s.total_chunk_count*100.0,
            s.free_chunk_count, (F64)s.free_chunk_count/(F64)s.total_chunk_count*100.0,
            s.total_chunk_count
        );
        printf("\n");

        assert(ok);
    };

    printf("After creation:\n");
    do_check();

    printf("(1)Allocate\n");
    auto a1 = allocate<U8>(heap);
    do_check();

    printf("(2)Allocate\n");
    auto a2 = allocate<F64>(heap);
    do_check();

    printf("(3)Allocate\n");
    auto a3 = allocate<Array<int>>(heap);
    do_check();

    printf("(4)Allocate\n");
    auto a4 = allocate<int>(heap);
    do_check();

    printf("(1)Free\n");
    free(a1, heap);
    do_check();

    printf("(2)Free\n");
    free(a2, heap);
    do_check();

    printf("(4)Free\n");
    free(a4, heap);
    do_check();

    printf("(3)Free\n");
    free(a3, heap);
    do_check();

    printf("\n");
}

int main() {
    util_assert();
    util_defer();
    util_math();
    memory_arena();
    memory_array();
    memory_map();
    memory_set();
    memory_heap();
}

