#pragma once

#include <libcpp/util/math.hpp>

namespace libcpp {

    //
    // RANGE interfaces.
    //

    template <typename T>
    bool eq(const Array<T> &left, const Array<T> &right) {
        auto result = array_eq(
            left.values,  left.count,
            right.values, right.count
        );
        return result;
    }


    //
    // RANGE lifecycle.
    //

    template <typename T>
    Array<T> create_array(Allocator &allocator, Usize initial_capacity) {
        Array<T> result = {};
        result.allocator = &allocator;
        set_capacity(result, initial_capacity);
        return result;
    }

    template <typename T>
    void destroy(Array<T> &array) {
        if(array.values != NULL && array.allocator != NULL) {
            free(array.values, *array.allocator);
        }

        array = {};
    }


    //
    // RANGE capacity manipulation.
    //

    template <typename T>
    void set_capacity(Array<T> &array, Usize new_capacity) {
        assert(array.allocator != NULL);

        T *new_values = NULL;
        if(new_capacity > 0 && new_capacity != array.capacity) {
            new_values = allocate_array_uninitialized<T>(new_capacity, *array.allocator);
            copy_values(new_values, array.values, min(array.count, new_capacity));

            if(array.values != NULL) {
                free(array.values, *array.allocator);
            }
        }

        array.values = new_values;
        array.capacity = new_capacity;
    }

    template <typename T>
    _inline void reserve(Array<T> &array, Usize count) {
        if(array.capacity < count) {
            set_capacity(array, count);
        }
    }

    template <typename T>
    _inline void grow(Array<T> &array, Usize count) {
        reserve(array, next_power_of_two(count));
    }

    template <typename T>
    _inline void grow_by(Array<T> &array, Usize delta) {
        grow(array, array.capacity + delta);
    }


    //
    // RANGE count manipulation.
    //

    template <typename T>
    void set_count(Array<T> &array, Usize count, T new_value) {
        grow(array, count);
        set_values(array.values + array.count, array.values + count, new_value);
        array.count = count;
    }

    template <typename T>
    _inline void set_min_count(Array<T> &array, Usize min_count, T new_value) {
        if(array.count < min_count) {
            set_count(array, min_count, new_value);
        }
    }

    template <typename T>
    _inline void add_count(Array<T> &array, Ssize delta, T new_value) {
        set_count(array, array.count + (Usize)delta, new_value);
    }


    //
    // RANGE querying.
    //

    template <typename T>
    Usize get_first_index(const Array<T> &array, const T &value) {
        for(Usize i = 0; i < array.count; i += 1) {
            if(eq(array[i], value)) {
                return i;
            }
        }
        return array.count;
    }

    template <typename T> bool has(const Array<T> &array, const T &value) {
        auto result = get_first_index(array, value) < array.count;
        return result;
    }


    //
    // RANGE addition.
    //

    template <typename T>
    void push(Array<T> &array, const T &value) {
        if(array.count + 1 > array.capacity) {
            grow(array, max((Usize)16, array.capacity + 1));
        }

        array.values[array.count] = value;
        array.count += 1;
    }

    template <typename T>
    void push(Array<T> &array, const Array<T> &other) {
        auto offset = array.count;
        auto count = other.count;

        set_count(array, array.count + other.count);
        for(Usize i = 0; i < count; i += 1) {
            array[offset + i] = other[i];
        }
    }

    template <typename T>
    void push_at(Array<T> &array, Usize index, const T &value) {
        make_space_at(array, index, 1);
        array.values[index] = value;
    }

    template <typename T>
    void push_at(Array<T> &array, Usize index, const Array<T> &other) {
        auto amount = other.count;
        make_space_at(array, index, amount);
        copy_values(array.values + index, other.values, amount);
    }

    template <typename T>
    _inline void push_front(Array<T> &array, const T &value) {
        push_at(array, 0, value);
    }

    template <typename T>
    _inline void push_front(Array<T> &array, const Array<T> &other) {
        push_at(array, 0, other);
    }

    template <typename T>
    bool push_unique(Array<T> &array, T value) {
        if(!has(array, value)) {
            push(array, value);
            return true;
        }
        return false;
    }

    template <typename T>
    T &push(Array<T> &array) {
        push(array, T {});
        return last(array);
    }


    //
    // RANGE removal.
    //

    template <typename T>
    T pop(Array<T> &array) {
        assert(array.count > 0);
        auto result = array[array.count - 1];
        array.count -= 1;
        return result;
    }

    template <typename T>
    void remove_at(Array<T> &array, Usize index) {
        assert(index < array.count);

        auto value_count_after_index = array.count - (index + 1);

        copy_values_ltr(
            array.values + index,
            array.values + index + 1,
            value_count_after_index
        );

        array.count -= 1;
    }

    template <typename T>
    void remove_swap(Array<T> &array, Usize index) {
        assert(index < array.count);

        array.values[index] = last(array);
        array.count -= 1;
    }

    template <typename T>
    void remove_leading(Array<T> &array, Usize count) {
        assert(count <= array.count);

        auto remaining = array.count - count;

        copy_values_ltr(
            array.values + 0,
            array.values + count,
            remaining
        );

        array.count -= count;
    }


    //
    // RANGE util.

    template <typename T>
    _inline void clear(Array<T> &array) {
        array.count = 0;
    }

    template <typename T>
    _inline void reset(Array<T> &array) {
        auto allocator = array.allocator;
        destroy(array);
        array.allocator = allocator;
    }

    template <typename T>
    Array<T> duplicate(const Array<T> &array, Allocator &allocator) {
        auto result = create_array<T>(allocator, array.count);

        result.count = array.count;
        copy_values(result.values, array.values, result.count);

        return result;
    }

    template <typename T>
    _inline Array<T> duplicate(const Array<T> &array) {
        return duplicate(array, *array.allocator);
    }

    template <typename T>
    _inline T &last(Array<T> &array, Usize offset) {
        return array[array.count - 1 - offset];
    }

    template <typename T>
    Array<T> sub_array(Array<T> &array, Usize begin, Usize count) {
        assert(begin + count <= array.count);

        auto result = Array<T> {};
        result.values = array.values + begin;
        result.count = count;
        return result;
    }

    template <typename T>
    _inline Usize remaining_capacity(Array<T> &array) {
        auto result = array.capacity - array.count;
        return result;
    }

    template <typename T>
    _inline Usize data_size(Array<T> &array) {
        auto result = array.count * sizeof(T);
        return result;
    }

    template <typename T>
    void make_space_at(Array<T> &array, Usize index, Usize amount) {
        assert(index <= array.count);

        auto copy_count = array.count - index;

        grow(array, array.count + amount);
        array.count += amount;

        copy_values_rtl(
            array.values + index + amount,
            array.values + index,
            copy_count
        );
    }

    template <typename T>
    void reverse(Array<T> &array) {
        for(Usize i = 0; i < array.count/2; i += 1) {
            swap(array[i], last(array, i));
        }
    }


}

