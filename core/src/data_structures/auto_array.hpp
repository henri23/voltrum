#pragma once

#include "defines.hpp"

#include <initializer_list>

#include "core/asserts.hpp"
#include "memory/memory.hpp"

// The autoarray is a dynamic array implementation that manages memory elements
// in a contigous memory layout

template <typename T> struct Auto_Array {
    u32 length;
    u32 capacity;
    T* data;

    // Provide standard typedefs but we don't use them ourselves.
    typedef T value_type;
    typedef value_type* iterator;
    typedef const value_type* const_iterator;

    // Constructors, destructor
    FORCE_INLINE Auto_Array() {
        length = capacity = 0;
        data = nullptr;
    }

    FORCE_INLINE Auto_Array(std::initializer_list<T> init) {
        length = capacity = 0;
        data = nullptr;
        reserve((u32)init.size());

        for (const T& item : init) {
            push_back(item);
        }
    }

    FORCE_INLINE Auto_Array<T>& operator=(const Auto_Array<T>& src) {
        clear();
        resize(src.length);
        if (data && src.data)
            memory_copy(data, src.data, (u32)length * sizeof(T));
        return *this;
    }

    FORCE_INLINE Auto_Array<T>& operator=(std::initializer_list<T> init) {
        clear();
        reserve((u32)init.size());
        for (const T& item : init) {
            push_back(item);
        }
        return *this;
    }

    FORCE_INLINE ~Auto_Array() {
        if (data)
            memory_deallocate(data,
                (u32)capacity * sizeof(T),
                Memory_Tag::DARRAY);

        length = 0;
        data = nullptr;
        capacity = 0;
    }

    FORCE_INLINE void clear() {
        length = 0;
    } // Important: Clear does not do any deallocation, just resets the pointer

    FORCE_INLINE b8 empty() const { return length == 0; }
    FORCE_INLINE u32 size() const { return length; }
    FORCE_INLINE u32 size_in_bytes() const { return length * (u32)sizeof(T); }
    FORCE_INLINE u32 max_size() const { return 0x7FFFFFFF / (u32)sizeof(T); }
    FORCE_INLINE u32 cap() const { return capacity; }

    FORCE_INLINE T& operator[](u32 i) {
        RUNTIME_ASSERT(i < length);
        return data[i];
    }

    FORCE_INLINE const T& operator[](u32 i) const {
        RUNTIME_ASSERT(i < length);
        return data[i];
    }

    FORCE_INLINE T* begin() { return data; }
    FORCE_INLINE const T* begin() const { return data; }
    FORCE_INLINE T* end() { return data + length; }
    FORCE_INLINE const T* end() const { return data + length; }

    FORCE_INLINE T& front() {
        RUNTIME_ASSERT(length > 0);
        return data[0];
    }

    FORCE_INLINE const T& front() const {
        RUNTIME_ASSERT(length > 0);
        return data[0];
    }

    FORCE_INLINE T& back() {
        RUNTIME_ASSERT(length > 0);
        return data[length - 1];
    }

    FORCE_INLINE const T& back() const {
        RUNTIME_ASSERT(length > 0);
        return data[length - 1];
    }

    FORCE_INLINE void swap(Auto_Array<T>& rhs) {
        u32 rhs_size = rhs.length;
        rhs.length = length;
        length = rhs_size;
        u32 rhs_cap = rhs.capacity;
        rhs.capacity = capacity;
        capacity = rhs_cap;
        T* rhs_data = rhs.data;
        rhs.data = data;
        data = rhs_data;
    }

    FORCE_INLINE u32 _grow_capacity(u32 sz) const {
        u32 new_capacity = capacity ? (capacity + capacity / 2) : 8;
        return new_capacity > sz ? new_capacity : sz;
    }

    FORCE_INLINE void resize(u32 new_size) {
        if (new_size > capacity)
            reserve(_grow_capacity(new_size));

        length = new_size;
    }

    FORCE_INLINE void resize(u32 new_size, const T& v) {
        if (new_size > capacity)
            reserve(_grow_capacity(new_size));

        if (new_size > length)
            for (u32 n = length; n < new_size; n++)
                memory_copy(&data[n], &v, sizeof(v));

        length = new_size;
    }

    FORCE_INLINE void shrink(u32 new_size) {
        RUNTIME_ASSERT(new_size <= length);
        length = new_size;
    } // Resize a vector to a smaller size, guaranteed not to need a realloc

    FORCE_INLINE void reserve(u32 new_capacity) {
        if (new_capacity <= capacity)
            return;

        T* new_data = (T*)memory_allocate((u32)new_capacity * sizeof(T),
            Memory_Tag::DARRAY);

        if (data) {
            memory_copy(new_data, data, (u32)length * sizeof(T));

            memory_deallocate(data,
                (u32)capacity * sizeof(T),
                Memory_Tag::DARRAY);
        }
        data = new_data;
        capacity = new_capacity;
    }

    FORCE_INLINE void reserve_discard(u32 new_capacity) {
        if (new_capacity <= capacity)
            return;

        if (data)
            memory_deallocate(data,
                (u32)capacity * sizeof(T),
                Memory_Tag::DARRAY);

        data = (T*)memory_allocate((u32)new_capacity * sizeof(T),
            Memory_Tag::DARRAY);

        capacity = new_capacity;
    }

    // NB: It is illegal to call push_back/push_front/insert with a reference
    // pointing inside the Auto_Array data itself! e.g. v.push_back(v[10]) is
    // forbidden.
    FORCE_INLINE void push_back(const T& v) {
        if (length == capacity)
            reserve(_grow_capacity(length + 1));

        memory_copy(&data[length], &v, sizeof(v));

        length++;
    }

    FORCE_INLINE void pop_back() {
        RUNTIME_ASSERT(length > 0);
        length--;
    }

    FORCE_INLINE void push_front(const T& v) {
        if (length == 0)
            push_back(v);
        else
            insert(data, v);
    }

    FORCE_INLINE T* erase(const T* it) {
        RUNTIME_ASSERT(it >= data && it < data + length);

        const u32 off = (u32)(it - data);

        memory_move(data + off, data + off + 1, (length - off - 1) * sizeof(T));

        length--;
        return data + off;
    }

    FORCE_INLINE T* erase(const T* it, const T* it_last) {
        RUNTIME_ASSERT(it >= data && it < data + length && it_last >= it &&
                       it_last <= data + length);

        const u32 count = (u32)(it_last - it);
        const u32 off = (u32)(it - data);

        memory_move(data + off,
            data + off + count,
            (length - off - count) * sizeof(T));

        length -= count;
        return data + off;
    }

    FORCE_INLINE T* erase_unsorted(const T* it) {
        RUNTIME_ASSERT(it >= data && it < data + length);

        const u32 off = (u32)(it - data);

        if (it < data + length - 1)
            memory_copy(data + off, data + length - 1, sizeof(T));

        length--;
        return data + off;
    }

    FORCE_INLINE T* insert(const T* it, const T& v) {
        RUNTIME_ASSERT(it >= data && it <= data + length);

        const u32 off = (u32)(it - data);

        if (length == capacity)
            reserve(_grow_capacity(length + 1));

        if (off < length)
            memory_move(data + off + 1, data + off, (length - off) * sizeof(T));

        memory_copy(&data[off], &v, sizeof(v));

        length++;
        return data + off;
    }

    FORCE_INLINE b8 contains(const T& v) const {
        const T* ptr = data;
        const T* data_end = data + length;

        while (ptr < data_end)
            if (*ptr++ == v)
                return true;

        return false;
    }

    FORCE_INLINE T* find(const T& v) {
        T* ptr = data;
        const T* data_end = data + length;

        while (ptr < data_end)
            if (*ptr == v)
                break;
            else
                ++ptr;

        return ptr;
    }

    FORCE_INLINE const T* find(const T& v) const {
        const T* ptr = data;
        const T* data_end = data + length;

        while (ptr < data_end)
            if (*ptr == v)
                break;
            else
                ++ptr;

        return ptr;
    }

    FORCE_INLINE u32 find_index(const T& v) const {
        const T* data_end = data + length;
        const T* it = find(v);

        if (it == data_end)
            return (u32)-1;

        const u32 off = (u32)(it - data);

        return off;
    }

    FORCE_INLINE b8 find_erase(const T& v) {
        const T* it = find(v);

        if (it < data + length) {
            erase(it);
            return true;
        }

        return false;
    }

    FORCE_INLINE b8 find_erase_unsorted(const T& v) {
        const T* it = find(v);

        if (it < data + length) {
            erase_unsorted(it);
            return true;
        }

        return false;
    }

    FORCE_INLINE u32 index_from_ptr(const T* it) const {
        RUNTIME_ASSERT(it >= data && it < data + length);
        const u32 off = (u32)(it - data);
        return off;
    }
};
