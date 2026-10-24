#pragma once

#include <array>
#include <cstddef>
#include <utility>

namespace zen {

    template <typename T>
    class double_buffer {
    private:
        std::array<T, 2> _buffer {};
        std::size_t      _current {0};

        [[nodiscard]] constexpr std::size_t _next_idx() noexcept { return this->_current ^ 1u; }

    public:
        double_buffer() = default;

        explicit double_buffer(T value) noexcept(std::is_nothrow_move_constructible_v<T>)
            : _buffer{ std::move(value), T{} } {}

        [[nodiscard]] constexpr T&       current()       noexcept { return this->_buffer[this->_current]; }
        [[nodiscard]] constexpr const T& current() const noexcept { return this->_buffer[this->_current]; }
        [[nodiscard]] constexpr T&       next()          noexcept { return this->_buffer[this->_next_idx()]; }
        [[nodiscard]] constexpr const T& next()    const noexcept { return this->_buffer[this->_next_idx()]; }

        constexpr void clear_current(T value = T{}) noexcept(std::is_nothrow_default_constructible_v<T>)
        { this->_buffer[this->_current] = value; }

        constexpr void clear_next(T value = T{}) noexcept(std::is_nothrow_default_constructible_v<T>)
        { this->_buffer[this->_next_idx()] = value; }

        constexpr void clear(T value = T{}) noexcept(std::is_nothrow_default_constructible_v<T>)
        { this->clear_current(value); this->clear_next(value); }

        constexpr void flip() noexcept { this->_current = this->_next_idx(); }

        constexpr void clear_next_and_flip(T value = T{}) noexcept(std::is_nothrow_default_constructible_v<T>)
        { this->clear_next(value); this->flip(); }

        constexpr void clear_current_and_flip(T value = T{}) noexcept(std::is_nothrow_default_constructible_v<T>)
        { this->clear_current(value); this->flip(); }
    };

} // namespace zen
