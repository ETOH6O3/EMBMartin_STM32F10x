#pragma once

#include "EMBMartin/macro.h"

EMBMARTIN_NAMESPACE_BEGIN

template <typename T>
class IterableUInt
{
    template <typename U>
    friend class IterableUInt;

private:
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>, "T must be an unsigned integral type");

    T __value;

    // 计算位数
    static constexpr size_t bit_count = sizeof(T) * CHAR_BIT;

    // 位引用类
    class Bit
    {
    private:
        T &_trg;
        size_t __pos;

    public:
        constexpr inline Bit(T &trg, size_t pos) noexcept : _trg{trg}, __pos{pos} {}
        constexpr inline Bit &operator=(bool bit) noexcept
        {
            if (bit)
                this->_trg |= T(1) << __pos;
            else
                this->_trg &= ~(T(1) << __pos);
            return *this;
        }
        constexpr inline operator bool()const noexcept
        {
            return bool((this->_trg >> this->__pos) & T(0b1));
        }
    };

    class ConstBit
    {
    private:
        const T &_trg;
        size_t __pos;

    public:
        constexpr inline ConstBit(const T &trg, size_t pos) noexcept : _trg{trg}, __pos{pos} {}
        constexpr inline operator bool() const noexcept
        {
            return bool((this->_trg >> this->__pos) & T(0b1));
        }
    };

    class Iterator
    {
    private:
        T &_trg;
        size_t __pos;

    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = bool;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = Bit;

        constexpr inline Iterator(T &trg, size_t pos = 0) noexcept : _trg{trg}, __pos{pos} {}
        constexpr inline Bit operator*() const noexcept { return {_trg, __pos}; }
        constexpr inline Iterator operator+(size_t offset) const noexcept
        {
            return Iterator(_trg, __pos + offset);
        }
        constexpr inline Iterator operator-(size_t offset) const noexcept
        {
            return Iterator(_trg, __pos - offset);
        }
        constexpr inline Iterator &operator+=(size_t offset) noexcept
        {
            __pos += offset;
            return *this;
        }
        constexpr inline Iterator &operator-=(size_t offset) noexcept
        {
            __pos -= offset;
            return *this;
        }
        constexpr inline Iterator &operator++() noexcept
        {
            ++__pos;
            return *this;
        }
        constexpr inline Iterator &operator--() noexcept
        {
            --__pos;
            return *this;
        }
        constexpr inline Iterator operator++(int) noexcept
        {
            Iterator temp = *this;
            ++__pos;
            return temp;
        }
        constexpr inline Iterator operator--(int) noexcept
        {
            Iterator temp = *this;
            --__pos;
            return temp;
        }
        constexpr inline difference_type operator-(const Iterator &other) const noexcept
        {
            return static_cast<difference_type>(__pos - other.__pos);
        }
        constexpr inline bool operator!=(const Iterator &other) const noexcept { return __pos != other.__pos; }
        constexpr inline bool operator==(const Iterator &other) const noexcept { return __pos == other.__pos; }
        constexpr inline bool operator<(const Iterator &other) const noexcept { return __pos < other.__pos; }
        constexpr inline bool operator<=(const Iterator &other) const noexcept { return __pos <= other.__pos; }
        constexpr inline bool operator>(const Iterator &other) const noexcept { return __pos > other.__pos; }
        constexpr inline bool operator>=(const Iterator &other) const noexcept { return __pos >= other.__pos; }
    };

    class ConstIterator
    {
    private:
        const T &_trg;
        size_t __pos;

    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = bool;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = ConstBit;

        constexpr inline ConstIterator(const T &trg, size_t pos = 0) noexcept : _trg{trg}, __pos{pos} {}
        constexpr inline ConstBit operator*() const noexcept { return {_trg, __pos}; }
        constexpr inline ConstIterator operator+(size_t offset) const noexcept
        {
            return ConstIterator(_trg, __pos + offset);
        }
        constexpr inline ConstIterator operator-(size_t offset) const noexcept
        {
            return ConstIterator(_trg, __pos - offset);
        }
        constexpr inline ConstIterator &operator+=(size_t offset) noexcept
        {
            __pos += offset;
            return *this;
        }
        constexpr inline ConstIterator &operator-=(size_t offset) noexcept
        {
            __pos -= offset;
            return *this;
        }
        constexpr inline ConstIterator &operator++() noexcept
        {
            ++__pos;
            return *this;
        }
        constexpr inline ConstIterator &operator--() noexcept
        {
            --__pos;
            return *this;
        }
        constexpr inline ConstIterator operator++(int) noexcept
        {
            ConstIterator temp = *this;
            ++__pos;
            return temp;
        }
        constexpr inline ConstIterator operator--(int) noexcept
        {
            ConstIterator temp = *this;
            --__pos;
            return temp;
        }
        constexpr inline difference_type operator-(const ConstIterator &other) const noexcept
        {
            return static_cast<difference_type>(__pos - other.__pos);
        }
        constexpr inline bool operator!=(const ConstIterator &other) const noexcept { return __pos != other.__pos; }
        constexpr inline bool operator==(const ConstIterator &other) const noexcept { return __pos == other.__pos; }
        constexpr inline bool operator<(const ConstIterator &other) const noexcept { return __pos < other.__pos; }
        constexpr inline bool operator<=(const ConstIterator &other) const noexcept { return __pos <= other.__pos; }
        constexpr inline bool operator>(const ConstIterator &other) const noexcept { return __pos > other.__pos; }
        constexpr inline bool operator>=(const ConstIterator &other) const noexcept { return __pos >= other.__pos; }
    };

public:
    // ------------------------------------------默认构造--------------------------------------------------
    constexpr inline IterableUInt() noexcept : __value{0} {}

    // ------------------------------------------从整数构造--------------------------------------------------
    constexpr inline IterableUInt(T value) noexcept : __value{value} {}
    template <typename U, typename = std::enable_if_t<!std::is_convertible_v<U, T>>>
    explicit constexpr inline IterableUInt(U value) noexcept : __value{static_cast<T>(value)} {}
    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>, typename = void>
    constexpr inline IterableUInt(U value) noexcept : __value{static_cast<T>(value)} {}

    // ------------------------------------------从同类构造--------------------------------------------------
    template <typename U, typename = std::enable_if_t<!std::is_convertible_v<U, T>>>
    explicit constexpr inline IterableUInt(const IterableUInt<U> &other) noexcept : __value{static_cast<T>(other.__value)} {}
    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>, typename = void>
    constexpr inline IterableUInt(const IterableUInt<U> &other) noexcept : __value{static_cast<T>(other.__value)} {}

    // ------------------------------------------向整型转换--------------------------------------------------
    constexpr inline operator T() const noexcept { return __value; }
    template <typename U, typename = std::enable_if_t<!std::is_convertible_v<T, U>>>
    explicit constexpr inline operator U() const noexcept { return static_cast<U>(__value); }
    template <typename U, typename = std::enable_if_t<std::is_convertible_v<T, U>>, typename = void>
    constexpr inline operator U() const noexcept { return static_cast<U>(__value); }

    // ------------------------------------------二元运算符--------------------------------------------------
    constexpr inline IterableUInt &operator+=(const IterableUInt &other) noexcept
    {
        __value += other.__value;
        return *this;
    }

    constexpr inline IterableUInt &operator-=(const IterableUInt &other) noexcept
    {
        __value -= other.__value;
        return *this;
    }

    constexpr inline IterableUInt &operator*=(const IterableUInt &other) noexcept
    {
        __value *= other.__value;
        return *this;
    }

    constexpr inline IterableUInt &operator/=(const IterableUInt &other) noexcept
    {
        __value /= other.__value;
        return *this;
    }

    constexpr inline IterableUInt &operator%=(const IterableUInt &other) noexcept
    {
        __value %= other.__value;
        return *this;
    }

    constexpr inline IterableUInt &operator&=(const IterableUInt &other) noexcept
    {
        __value &= other.__value;
        return *this;
    }

    constexpr inline IterableUInt &operator|=(const IterableUInt &other) noexcept
    {
        __value |= other.__value;
        return *this;
    }

    constexpr inline IterableUInt &operator^=(const IterableUInt &other) noexcept
    {
        __value ^= other.__value;
        return *this;
    }

    constexpr inline IterableUInt &operator<<=(size_t shift) noexcept
    {
        __value <<= shift;
        return *this;
    }

    constexpr inline IterableUInt &operator>>=(size_t shift) noexcept
    {
        __value >>= shift;
        return *this;
    }

    // ------------------------------------------一元运算符--------------------------------------------------
    constexpr inline IterableUInt operator+() const noexcept
    {
        return *this;
    }

    constexpr inline IterableUInt operator-() const noexcept
    {
        return IterableUInt(-__value);
    }

    constexpr inline IterableUInt operator~() const noexcept
    {
        return IterableUInt(~__value);
    }

    constexpr inline IterableUInt &operator++() noexcept
    {
        ++__value;
        return *this;
    }

    constexpr inline IterableUInt &operator--() noexcept
    {
        --__value;
        return *this;
    }

    constexpr inline IterableUInt operator++(int) noexcept
    {
        IterableUInt temp(*this);
        ++__value;
        return temp;
    }

    constexpr inline IterableUInt operator--(int) noexcept
    {
        IterableUInt temp(*this);
        --__value;
        return temp;
    }
    // ------------------------------------------比较运算符--------------------------------------------------
    constexpr inline bool operator==(const IterableUInt &other) const noexcept
    {
        return __value == other.__value;
    }

    constexpr inline bool operator!=(const IterableUInt &other) const noexcept
    {
        return __value != other.__value;
    }

    constexpr inline bool operator<(const IterableUInt &other) const noexcept
    {
        return __value < other.__value;
    }

    constexpr inline bool operator<=(const IterableUInt &other) const noexcept
    {
        return __value <= other.__value;
    }

    constexpr inline bool operator>(const IterableUInt &other) const noexcept
    {
        return __value > other.__value;
    }

    constexpr inline bool operator>=(const IterableUInt &other) const noexcept
    {
        return __value >= other.__value;
    }

    constexpr inline bool operator==(T value) const noexcept
    {
        return __value == value;
    }

    constexpr inline bool operator!=(T value) const noexcept
    {
        return __value != value;
    }

    constexpr inline bool operator<(T value) const noexcept
    {
        return __value < value;
    }

    constexpr inline bool operator<=(T value) const noexcept
    {
        return __value <= value;
    }

    constexpr inline bool operator>(T value) const noexcept
    {
        return __value > value;
    }

    constexpr inline bool operator>=(T value) const noexcept
    {
        return __value >= value;
    }

    // ------------------------------------------随机访问--------------------------------------------------
    constexpr inline Iterator begin() noexcept { return Iterator(__value, 0); }
    constexpr inline Iterator end() noexcept { return Iterator(__value, bit_count); }
    constexpr inline ConstIterator begin() const noexcept { return ConstIterator(__value, 0); }
    constexpr inline ConstIterator end() const noexcept { return ConstIterator(__value, bit_count); }
    constexpr inline ConstIterator cbegin() noexcept { return ConstIterator(__value, 0); }
    constexpr inline ConstIterator cend() noexcept { return ConstIterator(__value, bit_count); }
    constexpr inline Bit operator[](size_t pos) noexcept { return Bit(__value, pos); }
    constexpr inline ConstBit operator[](size_t pos) const noexcept { return ConstBit(__value, pos); }
};

using IterableUInt8 = IterableUInt<uint8_t>;
using IterableUInt16 = IterableUInt<uint16_t>;
using IterableUInt32 = IterableUInt<uint32_t>;
EMBMARTIN_NAMESPACE_END