#pragma once

#include <zen/crate/iterator/_contiguous_container.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <new>
#include <type_traits>
#include <utility>

namespace zen {

    template <typename T, std::size_t N>
    struct small_vec : contiguous_container_base<T> {
    public:
        static_assert(N > 0, "inline capacity must be at least 1");

        using base            = contiguous_container_base<T>;
        using size_type       = typename base::size_type;
        using reference       = typename base::reference;
        using const_reference = typename base::const_reference;
        using iterator        = typename base::iterator;
        using const_iterator  = typename base::const_iterator;

    private:
        alignas (T) std::byte _buffer[N * sizeof(T)];
        size_type _capacity { N };

        [[nodiscard]] constexpr T*       _inline_ptr()       noexcept { return reinterpret_cast<T*>(this->_buffer); }
        [[nodiscard]] constexpr const T* _inline_ptr() const noexcept { return reinterpret_cast<const T*>(this->_buffer); }

        [[nodiscard]] constexpr bool _is_heap() const noexcept { return this->_data != this->_inline_ptr(); }
        [[nodiscard]] constexpr bool _would_grow(size_type extra) const noexcept { return this->_size + extra > this->_capacity; }

        [[nodiscard]] constexpr size_type _compute_new_cap(size_type needed) const noexcept
        {
            size_type new_cap {this->_capacity ? this->_capacity : N};
            while (new_cap < needed) new_cap *= 2;
            return new_cap;
        }

        [[nodiscard]] constexpr T* _allocate(size_type n)
        {
            return static_cast<T*>(::operator new(n * sizeof(T), std::align_val_t(alignof(T))));
        }

        void _deallocate_heap() noexcept { if (this->_is_heap()) ::operator delete(this->_data, std::align_val_t(alignof(T))); }

        static void _relocate(T* dst, T* src, size_type count)
        {
            if constexpr (std::is_trivially_copyable_v<T>)
            {
                std::memcpy(dst, src, count * sizeof(T));
                return;
            }

            for (size_type i {0}; i < count; ++i)
            {
                new (dst + i) T {std::move(src[i])};
                src[i].~T();
            }
        }

        void _destroy_range(T* first, T* last) noexcept
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
                for (; first != last; ++first) first->~T();
        }

        void _shift_up(size_type idx, size_type n)
        {
            size_type count {this->_size - idx};
            if (count == 0) return;

            if constexpr (std::is_trivially_copyable_v<T>)
            {
                std::memmove(this->_data + idx + n, this->_data + idx, count * sizeof(T));
                return;
            }

            size_type moved {0};
            for (size_type i {count}; i-- > 0; )
            {
                new (this->_data + idx + n + i) T {std::move(this->_data[idx + i])};
                this->_data[idx + i].~T();
                ++moved;
            }
        }

        void _reallocate(size_type new_cap)
        {
            T* new_data {new_cap <= N ? this->_inline_ptr() : this->_allocate(new_cap)};
            this->_relocate(new_data, this->_data, this->_size);
            this->_deallocate_heap();
            this->_data     = new_data;
            this->_capacity = new_cap < N ? N : new_cap;
        }

        void _move_from(small_vec&& other)
        {
            if (!other._is_heap())
            {
                this->_relocate(this->_inline_ptr(), other._inline_ptr(), other._size);
                this->_size = std::exchange(other._size, 0);
                this->_capacity = N;
                return;
            }

            this->_data     = std::exchange(other._data, other._inline_ptr());
            this->_size     = std::exchange(other._size, 0);
            this->_capacity = std::exchange(other._capacity, N);
        }

        template <typename Fn>
        [[nodiscard]] iterator _insert_at(size_type idx, Fn&& construct)
        {
            if (this->_would_grow(1))
            {
                T tmp {construct()};
                this->_grow_to(this->_compute_new_cap(this->_size + 1));
                this->_shift_up(idx, 1);
                new (this->_data + idx) T {std::move(tmp)};
            }
            else
            {
                this->_shift_up(idx, 1);
                new (this->_data + idx) T {construct()};
            }

            ++this->_size;
            return this->_data + idx;
        }

        void _grow_to(size_type new_cap)
        {
            assert(new_cap > this->_capacity);
            T* new_data {this->_allocate(new_cap)};
            this->_relocate(new_data, this->_data, this->_size);
            this->_deallocate_heap();
            this->_data     = new_data;
            this->_capacity = new_cap;
        }

    public:
        small_vec() noexcept : base {} { this->_data = this->_inline_ptr(); }

        explicit small_vec(size_type count) : small_vec {} { this->resize(count); }

        small_vec(size_type count, const T& value) : small_vec {}
        {
            this->reserve(count);
            for (size_type _ {0}; _ < count; ++_) this->push_back(value);
        }

        small_vec(std::initializer_list<T> init) : small_vec {}
        {
            this->reserve(init.size());
            for (const T& v : init) this->push_back(v);
        }

        template <typename It, typename = std::enable_if_t<!std::is_integral_v<It>>>
        small_vec(It first, It last) : small_vec {} { for (; first != last; ++first) this->push_back(*first); }

        small_vec(const small_vec& other) : small_vec {}
        {
            this->reserve(other._size);
            for (const T& v : other) this->push_back(v);
        }

        small_vec(small_vec&& other) noexcept : small_vec {} { this->_move_from(std::move(other)); }

        small_vec& operator=(const small_vec& other)
        {
            if (this != &other) [[likely]]
            {
                this->clear();
                this->reserve(other._size);
                for (const T& v : other) this->push_back(v);
            }
            return *this;
        }

        small_vec& operator=(small_vec&& other) noexcept
        {
            if (this != &other) [[likely]]
            {
                this->clear();
                this->_deallocate_heap();
                this->_size     = 0;
                this->_capacity = N;
                this->_data     = this->_inline_ptr();
                this->_move_from(std::move(other));
            }
            return *this;
        }

        ~small_vec() { this->clear(); this->_deallocate_heap(); }

        [[nodiscard]] constexpr bool      empty()    const noexcept { return this->_size == 0; }
        [[nodiscard]] constexpr size_type size()     const noexcept { return this->_size; }
        [[nodiscard]] constexpr size_type capacity() const noexcept { return this->_capacity; }
        [[nodiscard]] constexpr size_type max_size() const noexcept { return static_cast<size_type>(-1) / sizeof(T); }

        void reserve(size_type new_cap) { if (new_cap > this->_capacity) this->_grow_to(new_cap); }

        void shrink_to_fit()
        {
            if (!this->_is_heap() || this->_size == this->_capacity) return;
            this->_reallocate(this->_size);
        }

        void resize(size_type new_size, const T& value)
        {
            if (new_size < this->_size)
                this->_destroy_range(this->_data + new_size, this->_data + this->_size);
            else if (new_size > this->_size)
            {
                T tmp {value};
                this->reserve(new_size);
                for (size_type i {this->_size}; i < new_size; ++i) new (this->_data + i) T {tmp};
            }
            this->_size = new_size;
        }

        void resize(size_type new_size) { this->resize(new_size, T {}); }

        void clear() noexcept
        {
            this->_destroy_range(this->_data, this->_data + this->_size);
            this->_size = 0;
        }

        void push_back(const T& value) { this->emplace_back(value); }
        void push_back(T&& value)      { this->emplace_back(std::move(value)); }

        template <typename... Args>
        reference emplace_back(Args&&... args)
        {
            if (this->_would_grow(1))
            {
                T tmp {std::forward<Args>(args)...};
                this->_grow_to(this->_compute_new_cap(this->_size + 1));
                new (this->_data + this->_size) T {std::move(tmp)};
            }
            else new (this->_data + this->_size) T {std::forward<Args>(args)...};

            return this->_data[this->_size++];
        }

        void pop_back() { assert(this->_size > 0); this->_data[--this->_size].~T(); }

        [[nodiscard]] iterator insert(const_iterator pos, const T& value)
        {
            size_type idx {static_cast<size_type>(pos - this->_data)};
            return this->_insert_at(idx, [&] { return value; });
        }

        [[nodiscard]] iterator insert(const_iterator pos, T&& value)
        {
            size_type idx {static_cast<size_type>(pos - this->_data)};
            return this->_insert_at(idx, [&] { return std::move(value); });
        }

        template <typename... Args>
        [[nodiscard]] iterator emplace(const_iterator pos, Args&&... args)
        {
            size_type idx {static_cast<size_type>(pos - this->_data)};
            return this->_insert_at(idx, [&] { return T {std::forward<Args>(args)...}; });
        }

        [[nodiscard]] iterator erase(const_iterator pos) { return this->erase(pos, pos + 1); }

        [[nodiscard]] iterator erase(const_iterator first, const_iterator last)
        {
            size_type f {static_cast<size_type>(first - this->_data)};
            size_type l {static_cast<size_type>(last - this->_data)};

            this->_destroy_range(this->_data + f, this->_data + l);
            std::memmove(this->_data + f, this->_data + l, (this->_size - l) * sizeof(T));
            this->_size -= (l - f);

            return this->_data + f;
        }
    };

} // namespace zen
