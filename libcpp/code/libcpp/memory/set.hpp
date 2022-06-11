#pragma once

#include <libcpp/base.hpp>
#include <libcpp/memory/hash.hpp>

namespace libcpp {

    template <typename Key>
    struct Set_Entry {
        using Key_Type = Key;
        Key key;
    };

    template <typename Key, typename Hasher = Default_Hasher<Key>>
    using Set = _hash::Hash_Container<Set_Entry<Key>, Hasher>;

    // lifecycle.
    template <typename Key, typename Hasher = Default_Hasher<Key>>
    Set<Key, Hasher> create_set(
        Allocator &allocator = default_allocator,
        Usize initial_capacity = 0
    );
    // c++ adl.

    // capacity manipulation.
    // c++ adl.

    // querying.
    template <typename Key, typename Hasher>
    bool has(const Set<Key, Hasher> &set, Key key);

    // addition.
    template <typename Key, typename Hasher>
    bool insert_maybe(Set<Key, Hasher> &set, Key key);
    template <typename Key, typename Hasher>
    void insert(Set<Key, Hasher> &set, Key key);
    // c++ adl.

    // removal.
    template <typename Key, typename Hasher>
    bool remove_maybe(Set<Key, Hasher> &set, Key key);
    template <typename Key, typename Hasher>
    void remove(Set<Key, Hasher> &set, Key key);

    // util.
    // c++ adl.

}

#include "set.inl"

