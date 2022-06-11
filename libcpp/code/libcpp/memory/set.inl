#pragma once

namespace libcpp {

    template <typename Key, typename Hasher>
    Set<Key, Hasher> create_set(Allocator &allocator, Usize initial_capacity) {
        auto result = Set<Key, Hasher> {};
        result.allocator = &allocator;
        reserve(result, initial_capacity);
        return result;
    }

    template <typename Key, typename Hasher>
    bool has(const Set<Key, Hasher> &set, Key key) {
        auto result = _hash::search(set, key).found_slot != (Usize)-1;
        return result;
    }

    template <typename Key, typename Hasher>
    bool insert_maybe(Set<Key, Hasher> &set, Key key) {
        auto entry = Set_Entry<Key>{ key };
        auto result = _hash::_insert(set, entry);
        return result;
    }

    template <typename Key, typename Hasher>
    void insert(Set<Key, Hasher> &set, Key key) {
        auto inserted = insert_maybe(set, key);
        assert(inserted);
    }

    template <typename Key, typename Hasher>
    bool remove_maybe(Set<Key, Hasher> &set, Key key) {
        auto result = _hash::_remove(set, key);
        return result;
    }

    template <typename Key, typename Hasher>
    void remove(Set<Key, Hasher> &set, Key key) {
        auto removed = remove_maybe(set, key);
        assert(removed);
    }

}

