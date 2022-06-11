#pragma once

#include <libcpp/util/math.hpp>

namespace libcpp { namespace _hash {

    //
    // RANGE internal.
    //

    // NOTE(llw): Quadratic probing with triangular numbers, as they produce a
    //  permutation on indices (modulo 2^n).
    //  https://fgiesen.wordpress.com/2015/02/22/triangular-numbers-mod-2n/
    _inline Usize probe(Usize primary, Usize index) {
        auto result = primary + (index + index*index)/2;
        return result;
    }

    constexpr F32 hash_load_factor_grow = 0.8f;

    enum {
        HASH_ENTRY_STATE_EMPTY    = 0,
        HASH_ENTRY_STATE_OCCUPIED = 1,
        HASH_ENTRY_STATE_DELETED  = 2,
    };

    template <typename T, typename Hasher>
    _inline F32 load_factor(Hash_Container<T, Hasher> &container, Usize count) {
        auto result = (F32)count / (F32)container.capacity;
        return result;
    }

    template <typename T, typename Hasher>
    _inline F32 load_factor(Hash_Container<T, Hasher> &container) {
        auto result = load_factor(container, container.count);
        return result;
    }

    template <typename T, typename Hasher>
    _inline bool needs_grow(Hash_Container<T, Hasher> &container) {
        auto result =
               container.capacity == 0
            || load_factor(container) > hash_load_factor_grow;
        return result;
    }


    //
    // RANGE lifecycle.
    //

    template <typename T, typename Hasher>
    void destroy(Hash_Container<T, Hasher> &container) {
        if(container.allocator != NULL && container.capacity > 0) {
            assert(container.slots != NULL && container.entries != NULL);
            free(container.slots, *container.allocator);
            free(container.entries, *container.allocator);
        }
        container = {};
    }


    //
    // RANGE capacity manipulation.
    //

    template <typename T, typename Hasher>
    void set_capacity(Hash_Container<T, Hasher> &container, Usize new_capacity) {
        assert(container.allocator != NULL);
        assert(is_power_of_two(new_capacity));

        if(new_capacity > 0 && new_capacity != container.capacity) {
            auto old_slots = container.slots;
            auto old_entries = container.entries;
            auto old_capacity = container.capacity;
            auto old_count = container.count;

            container.slots   = allocate_array<Hash_Container<T, Hasher>::Slot>(
                new_capacity, *container.allocator
            );
            container.entries = allocate_array<T>(new_capacity, *container.allocator);
            container.count = 0;
            container.capacity = new_capacity;

            for(Usize i = 0;
                i < old_count && !needs_grow(container);
                i += 1
            ) {
                _insert(container, old_entries[i]);
            }

            if(new_capacity > old_capacity) {
                assert(container.count == old_count);
            }

            if(old_capacity > 0) {
                assert(container.slots != NULL && container.entries != NULL);
                free(old_entries, *container.allocator);
                free(old_slots, *container.allocator);
            }
        }
    }

    template <typename T, typename Hasher>
    void reserve(Hash_Container<T, Hasher> &container, Usize count) {
        auto required = (Usize)((F32)(count+1)/hash_load_factor_grow);

        if(count > 0 && container.capacity < required) {
            grow(container, required);
        }
    }

    template <typename T, typename Hasher>
    _inline void grow(Hash_Container<T, Hasher> &container, Usize count) {
        set_capacity(container, next_power_of_two(count));
    }

    template <typename T, typename Hasher>
    _inline void grow_by(Hash_Container<T, Hasher> &container, Usize delta) {
        grow(container, container.capacity + delta);
    }


    //
    // RANGE querying.
    //

    template <typename T, typename Hasher, typename K>
    Hash_Search_Result search(const Hash_Container<T, Hasher> &container, const K &key) {
        auto result = Hash_Search_Result { (Usize)-1, (Usize)-1, (Usize)-1 };

        // NOTE(llw): Don't hash on first call to insert.
        if(container.capacity == 0) {
            return result;
        }

        auto primary = Hasher::hash(key);

        for(Usize i = 0; i < container.capacity; i += 1) {
            auto index = probe(primary, i) % container.capacity;
            auto slot = container.slots[index];

            if(slot.state == HASH_ENTRY_STATE_OCCUPIED) {
                if(eq(slot.key, key)) {
                    result.found_slot = index;
                    result.found_entry = slot.entry_index;
                    break;
                }
            }
            else { // NOTE(llw): Not occupied.
                if(result.insertion_slot == (Usize)-1) {
                    result.insertion_slot = index;
                }

                if(slot.state == HASH_ENTRY_STATE_EMPTY) {
                    break;
                }
            }
        }

        return result;
    }


    //
    // RANGE addition.
    //

    template <typename T, typename Hasher>
    bool _insert(Hash_Container<T, Hasher> &container, const T &entry) {
        auto search_result = search(container, entry.key);

        if(search_result.found_slot == (Usize)-1) {
            auto insertion_slot = search_result.insertion_slot;

            if(needs_grow(container)) {
                set_capacity(container, max((Usize)16, container.capacity*2));
                insertion_slot = search(container, entry.key).insertion_slot;
                assert(insertion_slot != (Usize)-1);
            }

            auto &slot = container.slots[insertion_slot];
            slot.key = entry.key;
            slot.entry_index = container.count;
            slot.state = HASH_ENTRY_STATE_OCCUPIED;

            container.entries[container.count] = entry;
            container.count += 1;

            return true;
        }
        else {
            return false;
        }
    }

    template <typename T, typename Hasher>
    Usize insert_maybe(Hash_Container<T, Hasher> &container, const Hash_Container<T, Hasher> &other) {
        reserve(container, container.count + other.count);

        auto inserted = (Usize)0;
        for(Usize i = 0; i < other.count; i += 1) {
            auto &entry = other.entries[i];
            if(_insert(container, entry)) {
                inserted += 1;
            }
        }

        return inserted;
    }

    template <typename T, typename Hasher>
    _inline void insert(
        Hash_Container<T, Hasher> &container,
        const Hash_Container<T, Hasher> &other
    ) {
        auto inserted = insert_maybe(container, other);
        assert(inserted == other.count);
    }


    //
    // RANGE removal.
    //

    template <typename T, typename Hasher, typename K>
    bool _remove(Hash_Container<T, Hasher> &container, const K &key) {
        auto search_result = search(container, key);

        if(search_result.found_slot != (Usize)-1) {
            auto slot = search_result.found_slot;
            auto entry = search_result.found_entry;

            container.slots[slot].state = HASH_ENTRY_STATE_DELETED;

            container.count -= 1;
            container.entries[entry] = container.entries[container.count];

            // NOTE(llw): Update entry index for slot of swapped entry.
            if(entry < container.count) {
                search_result = search(container, container.entries[entry].key);
                assert(search_result.found_slot != (Usize)-1);
                container.slots[search_result.found_slot].entry_index = entry;
            }

            return true;
        }
        else {
            return false;
        }
    }


    //
    // RANGE util.
    //

    template <typename T, typename Hasher>
    Hash_Container<T, Hasher> duplicate(
        const Hash_Container<T, Hasher> &container,
        Allocator &allocator
    ) {
        auto result = Hash_Container<T, Hasher> {};
        result.allocator = &allocator;
        reserve(result, container.count);

        // NOTE(llw): Inserting is more expensive than copying but gets rid of
        // deleted slots!
        for(Usize i = 0; i < container.count; i += 1) {
            auto inserted = _insert(result, container.entries[i]);
            assert(inserted);
        }

        return result;
    }

    template <typename T, typename Hasher>
    _inline Hash_Container<T, Hasher> duplicate(
        const Hash_Container<T, Hasher> &container
    ) {
        auto result = duplicate(container, *container.allocator);
        return result;
    }

    template <typename T, typename Hasher>
    _inline void clear(Hash_Container<T, Hasher> &container) {
        set_bytes(container.slots, 0, container.capacity);
        container.count = 0;
    }

    template <typename T, typename Hasher>
    _inline void reset(Hash_Container<T, Hasher> &container) {
        auto allocator = container.allocator;
        destroy(container);
        container.allocator = allocator;
    }

}}

