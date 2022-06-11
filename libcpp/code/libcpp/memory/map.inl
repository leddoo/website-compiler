#pragma once

namespace libcpp {

    //
    // RANGE lifecycle.
    //

    template <typename Key, typename Value, typename Hasher>
    Map<Key, Value, Hasher> create_map(Allocator &allocator, Usize initial_capacity) {
        auto result = Map<Key, Value, Hasher> {};
        result.allocator = &allocator;
        reserve(result, initial_capacity);
        return result;
    }


    //
    // RANGE querying.
    //

    template <typename Key, typename Value, typename Hasher>
    _inline bool has(const Map<Key, Value, Hasher> &map, Key key) {
        auto result = _hash::search(map, key).found_slot != (Usize)-1;
        return result;
    }

    template <typename Key, typename Value, typename Hasher>
    Value *get_pointer(Map<Key, Value, Hasher> &map, Key key) {
        auto result = (Value *)NULL;

        auto entry = _hash::search(map, key).found_entry;
        if(entry != (Usize)-1) {
            result = &map.entries[entry].value;
        }

        return result;
    }

    template <typename Key, typename Value, typename Hasher>
    _inline Value &get(Map<Key, Value, Hasher> &map, Key key) {
        auto result = get_pointer(map, key);
        assert(result != NULL);
        return *result;
    }

    template <typename Key, typename Value, typename Hasher>
    _inline const Value *get_pointer(const Map<Key, Value, Hasher> &map, Key key) {
        return get_pointer(*(Map<Key, Value, Hasher> *)&map, key);
    }

    template <typename Key, typename Value, typename Hasher>
    _inline const Value &get(const Map<Key, Value, Hasher> &map, Key key) {
        return get(*(Map<Key, Value, Hasher> *)&map, key);
    }


    //
    // RANGE addition.
    //

    template <typename Key, typename Value, typename Hasher>
    bool insert_maybe(Map<Key, Value, Hasher> &map, Key key, const Value &value) {
        auto entry = Map_Entry<Key, Value>{ value, key };
        auto result = _hash::_insert(map, entry);
        return result;
    }

    template <typename Key, typename Value, typename Hasher>
    void insert(Map<Key, Value, Hasher> &map, Key key, const Value &value) {
        auto inserted = insert_maybe(map, key, value);
        assert(inserted);
    }

    template <typename Key, typename Value, typename Hasher>
    void insert_or_set(Map<Key, Value, Hasher> &map, Key key, const Value &value) {
        if(!insert_maybe(map, key, value)) {
            map[key] = value;
        }
    }


    //
    // RANGE removal.
    //

    template <typename Key, typename Value, typename Hasher>
    bool remove_maybe(Map<Key, Value, Hasher> &map, Key key) {
        auto result = _hash::_remove(map, key);
        return result;
    }

    template <typename Key, typename Value, typename Hasher>
    void remove(Map<Key, Value, Hasher> &map, Key key) {
        auto removed = remove_maybe(map, key);
        assert(removed);
    }


    //
    // RANGE util.
    //

    template <typename Key, typename Value, typename Hasher>
    _inline Map<Key, Value, Hasher> duplicate(
        const Map<Key, Value, Hasher> &map,
        Allocator &allocator
    ) {
        auto result = _hash::duplicate(map, allocator);
        return result;
    }

    template <typename Key, typename Value, typename Hasher>
    _inline Map<Key, Value, Hasher> duplicate(
        const Map<Key, Value, Hasher> &map
    ) {
        auto result = Map<Key, Value, Hasher>(_hash::duplicate(map));
        return result;
    }
}

