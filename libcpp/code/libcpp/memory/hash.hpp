#pragma once

#include <libcpp/base.hpp>
#include <libcpp/memory/allocator.hpp>
#include <libcpp/memory/string.hpp>

namespace libcpp {

    U64 murmur_hash_64(void *key, Usize size, U64 seed = 0x0dc61362440d29b5ULL);

    template <typename T>
    _inline U64 hash(const T &value) {
        auto result = murmur_hash_64((void *)&value, sizeof(value));
        return result;
    }

    template <>
    _inline U64 hash(const String &key) {
        auto result = murmur_hash_64(key.values, key.size);
        return result;
    }

    template <typename Key>
    struct Default_Hasher {
        static U64 hash(const Key &key) {
            return ::libcpp::hash(key);
        }
    };

}

namespace libcpp { namespace _hash {

    template <typename T, typename Hasher>
    struct Hash_Container {

        struct Slot {
            typename T::Key_Type key;
            Usize entry_index;
            U8 state;
        };

        mutable Allocator *allocator;
        Slot *slots;
        T *entries;
        Usize count;
        Usize capacity;
    };


    // lifecycle.
    template <typename T, typename Hasher>
    void destroy(Hash_Container<T, Hasher> &container);

    // capacity manipulation.
    template <typename T, typename Hasher>
    void set_capacity(Hash_Container<T, Hasher> &container, Usize new_capacity);
    template <typename T, typename Hasher>
    void reserve(Hash_Container<T, Hasher> &container, Usize count);
    template <typename T, typename Hasher>
    void grow(Hash_Container<T, Hasher> &container, Usize count);
    template <typename T, typename Hasher>
    void grow_by(Hash_Container<T, Hasher> &container, Usize delta);

    // querying.
    struct Hash_Search_Result {
        Usize insertion_slot;
        Usize found_slot;
        Usize found_entry;
    };
    template <typename T, typename Hasher, typename K>
    Hash_Search_Result search(const Hash_Container<T, Hasher> &container, const K &key);

    // addition.
    template <typename T, typename Hasher>
    bool _insert(Hash_Container<T, Hasher> &container, const T &entry);
    template <typename T, typename Hasher>
    Usize insert_maybe(Hash_Container<T, Hasher> &container, const Hash_Container<T, Hasher> &other);
    template <typename T, typename Hasher>
    void insert(Hash_Container<T, Hasher> &container, const Hash_Container<T, Hasher> &other);

    // removal.
    template <typename T, typename Hasher, typename K>
    bool _remove(Hash_Container<T, Hasher> &container, const K &key);

    // util.
    template <typename T, typename Hasher>
    Hash_Container<T, Hasher> duplicate(
        const Hash_Container<T, Hasher> &container,
        Allocator &allocator
    );
    template <typename T, typename Hasher>
    Hash_Container<T, Hasher> duplicate(
        const Hash_Container<T, Hasher> &container
    );
    template <typename T, typename Hasher>
    void clear(Hash_Container<T, Hasher> &container);
    template <typename T, typename Hasher>
    void reset(Hash_Container<T, Hasher> &container);

}}

#include "hash.inl"

