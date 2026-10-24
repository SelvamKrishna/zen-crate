#pragma once

#include <cassert>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <stdexcept>

namespace zen {

    template <typename T>
    struct contiguous_container_base {
    public:
        using value_type             = T;
        using size_type              = std::size_t;
        using difference_type        = std::ptrdiff_t;
        using reference              = T&;
        using const_reference        = const T&;
        using pointer                = T*;
        using const_pointer          = const T*;
        using iterator               = T*;
        using const_iterator         = const T*;
        using reverse_iterator       = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        static_assert(std::contiguous_iterator<iterator>);
        static_assert(std::contiguous_iterator<const_iterator>);

    protected:
        T*        _data { nullptr };
        size_type _size { 0 };

        contiguous_container_base() = default;
        contiguous_container_base(T* data, size_type size) : _data { data }, _size { size } {}

        ~contiguous_container_base() = default;

        void _check_range(size_type i) const
        {
            if (i >= this->_size) throw std::out_of_range("zen::contiguous_container_base::at");
        }

    public:
        [[nodiscard]] constexpr reference       operator[](size_type i)       { return this->_data[i]; }
        [[nodiscard]] constexpr const_reference operator[](size_type i) const { return this->_data[i]; }
        [[nodiscard]] constexpr reference       at(size_type i)       { this->_check_range(i); return this->_data[i]; }
        [[nodiscard]] constexpr const_reference at(size_type i) const { this->_check_range(i); return this->_data[i]; }
        [[nodiscard]] constexpr reference       front()       { return this->_data[0]; }
        [[nodiscard]] constexpr const_reference front() const { return this->_data[0]; }
        [[nodiscard]] constexpr reference       back ()       { return this->_data[this->_size - 1]; }
        [[nodiscard]] constexpr const_reference back () const { return this->_data[this->_size - 1]; }
        [[nodiscard]] constexpr pointer         data () noexcept       { return this->_data; }
        [[nodiscard]] constexpr const_pointer   data () const noexcept { return this->_data; }

        [[nodiscard]] constexpr iterator       begin () noexcept       { return this->_data; }
        [[nodiscard]] constexpr const_iterator begin () const noexcept { return this->_data; }
        [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return this->_data; }
        [[nodiscard]] constexpr iterator       end   () noexcept       { return this->_data + this->_size; }
        [[nodiscard]] constexpr const_iterator end   () const noexcept { return this->_data + this->_size; }
        [[nodiscard]] constexpr const_iterator cend  () const noexcept { return this->_data + this->_size; }

        [[nodiscard]] constexpr reverse_iterator       rbegin () noexcept       { return reverse_iterator(this->end()); }
        [[nodiscard]] constexpr const_reverse_iterator rbegin () const noexcept { return const_reverse_iterator(this->end()); }
        [[nodiscard]] constexpr reverse_iterator       rend   () noexcept       { return reverse_iterator(this->begin()); }
        [[nodiscard]] constexpr const_reverse_iterator rend   () const noexcept { return const_reverse_iterator(this->begin()); }
        [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(this->cend()); }
        [[nodiscard]] constexpr const_reverse_iterator crend  () const noexcept { return const_reverse_iterator(this->cbegin()); }
    };

} // namespace zen
