#pragma once

#include <libcpp/base.hpp>
#include <libcpp/memory/hash.hpp>

namespace libcpp {

    template <typename Key, typename Value>
    struct Map_Entry {
        using Key_Type = Key;
        using Value_Type = Value;

        //mutable
        Value value;
        Key key;
    };

    template <typename Key, typename Value, typename Hasher>
    using Map_Type = _hash::Hash_Container<Map_Entry<Key, Value>, Hasher>;

    template <typename Key, typename Value, typename Hasher = Default_Hasher<Key>>
    struct Map : Map_Type<Key, Value, Hasher> {

        Map() = default;

        Map(const Map_Type<Key, Value, Hasher> &map)
            : Map_Type<Key, Value, Hasher>(map) {}

        Value &operator[](const Key &key) {
            return get(*this, key);
        }

        const Value &operator[](const Key &key) const {
            return get(*this, key);
        }
    };


    // lifecycle.
    template <typename Key, typename Value, typename Hasher = Default_Hasher<Key>>
    Map<Key, Value, Hasher> create_map(
        Allocator &allocator = default_allocator,
        Usize initial_capacity = 0
    );
    // c++ adl.

    // capacity manipulation.
    // c++ adl.

    // querying.
    template <typename Key, typename Value, typename Hasher>
    bool has(const Map<Key, Value, Hasher> &map, Key key);
    template <typename Key, typename Value, typename Hasher>
    Value *get_pointer(Map<Key, Value, Hasher> &map, Key key);
    template <typename Key, typename Value, typename Hasher>
    Value &get(Map<Key, Value, Hasher> &map, Key key);
    template <typename Key, typename Value, typename Hasher>
    const Value *get_pointer(const Map<Key, Value, Hasher> &map, Key key);

    // addition.
    template <typename Key, typename Value, typename Hasher>
    bool insert_maybe(Map<Key, Value, Hasher> &map, Key key, const Value &value);
    template <typename Key, typename Value, typename Hasher>
    void insert(Map<Key, Value, Hasher> &map, Key key, const Value &value);
    // c++ adl.
    template <typename Key, typename Value, typename Hasher>
    void insert_or_set(Map<Key, Value, Hasher> &map, Key key, const Value &value);

    // removal.
    template <typename Key, typename Value, typename Hasher>
    bool remove_maybe(Map<Key, Value, Hasher> &map, Key key);
    template <typename Key, typename Value, typename Hasher>
    void remove(Map<Key, Value, Hasher> &map, Key key);

    // util.
    template <typename Key, typename Value, typename Hasher>
    Map<Key, Value, Hasher> duplicate(const Map<Key, Value, Hasher> &map, Allocator &allocator);
    template <typename Key, typename Value, typename Hasher>
    Map<Key, Value, Hasher> duplicate(const Map<Key, Value, Hasher> &map);
    // c++ adl.

}

#include "map.inl"

