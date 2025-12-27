#ifndef EMBMARTIN_OLEDUtility_H
#define EMBMARTIN_OLEDUtility_H

#include <cstddef>
#include <cstdint>
#include <climits>
#include <type_traits>
#include <iterator>
#include <array>
#include <cmath>

#include "macro.h"

// 属于 OLEDUtility 的头文件，没有合并进来是为了向前兼容
#include "OLED_Font.h"
#include "coordinate.h"

EMBMARTIN_DEBUGING_EXTERN_CONSOLE;

EMBMARTIN_OLED_NAMESPACE_BEGIN

template <typename T>
class IterableUInt
{
    template <typename U>
    friend class IterableUInt;

private:
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>, "T must be an unsigned integral type");

    T _value;

    // 计算位数
    static constexpr size_t bit_count = sizeof(T) * CHAR_BIT;

    // 位引用类
    class Bit
    {
    private:
        T &_trg;
        size_t _pos;

    public:
        constexpr inline Bit(T &trg, size_t pos) noexcept : _trg{trg}, _pos{pos} {}
        constexpr inline Bit &operator=(bool bit) noexcept
        {
            if (bit)
                this->_trg |= T(1) << _pos;
            else
                this->_trg &= ~(T(1) << _pos);
            return *this;
        }
        constexpr inline operator bool()const noexcept
        {
            return bool((this->_trg >> this->_pos) & T(0b1));
        }
    };

    class ConstBit
    {
    private:
        const T &_trg;
        size_t _pos;

    public:
        constexpr inline ConstBit(const T &trg, size_t pos) noexcept : _trg{trg}, _pos{pos} {}
        constexpr inline operator bool() const noexcept
        {
            return bool((this->_trg >> this->_pos) & T(0b1));
        }
    };

    class Iterator
    {
    private:
        T &_trg;
        size_t _pos;

    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = bool;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = Bit;

        constexpr inline Iterator(T &trg, size_t pos = 0) noexcept : _trg{trg}, _pos{pos} {}
        constexpr inline Bit operator*() const noexcept { return {_trg, _pos}; }
        constexpr inline Iterator operator+(size_t offset) const noexcept
        {
            return Iterator(_trg, _pos + offset);
        }
        constexpr inline Iterator operator-(size_t offset) const noexcept
        {
            return Iterator(_trg, _pos - offset);
        }
        constexpr inline Iterator &operator+=(size_t offset) noexcept
        {
            _pos += offset;
            return *this;
        }
        constexpr inline Iterator &operator-=(size_t offset) noexcept
        {
            _pos -= offset;
            return *this;
        }
        constexpr inline Iterator &operator++() noexcept
        {
            ++_pos;
            return *this;
        }
        constexpr inline Iterator &operator--() noexcept
        {
            --_pos;
            return *this;
        }
        constexpr inline Iterator operator++(int) noexcept
        {
            Iterator temp = *this;
            ++_pos;
            return temp;
        }
        constexpr inline Iterator operator--(int) noexcept
        {
            Iterator temp = *this;
            --_pos;
            return temp;
        }
        constexpr inline difference_type operator-(const Iterator &other) const noexcept
        {
            return static_cast<difference_type>(_pos - other._pos);
        }
        constexpr inline bool operator!=(const Iterator &other) const noexcept { return _pos != other._pos; }
        constexpr inline bool operator==(const Iterator &other) const noexcept { return _pos == other._pos; }
        constexpr inline bool operator<(const Iterator &other) const noexcept { return _pos < other._pos; }
        constexpr inline bool operator<=(const Iterator &other) const noexcept { return _pos <= other._pos; }
        constexpr inline bool operator>(const Iterator &other) const noexcept { return _pos > other._pos; }
        constexpr inline bool operator>=(const Iterator &other) const noexcept { return _pos >= other._pos; }
    };

    class ConstIterator
    {
    private:
        const T &_trg;
        size_t _pos;

    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = bool;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = ConstBit;

        constexpr inline ConstIterator(const T &trg, size_t pos = 0) noexcept : _trg{trg}, _pos{pos} {}
        constexpr inline ConstBit operator*() const noexcept { return {_trg, _pos}; }
        constexpr inline ConstIterator operator+(size_t offset) const noexcept
        {
            return ConstIterator(_trg, _pos + offset);
        }
        constexpr inline ConstIterator operator-(size_t offset) const noexcept
        {
            return ConstIterator(_trg, _pos - offset);
        }
        constexpr inline ConstIterator &operator+=(size_t offset) noexcept
        {
            _pos += offset;
            return *this;
        }
        constexpr inline ConstIterator &operator-=(size_t offset) noexcept
        {
            _pos -= offset;
            return *this;
        }
        constexpr inline ConstIterator &operator++() noexcept
        {
            ++_pos;
            return *this;
        }
        constexpr inline ConstIterator &operator--() noexcept
        {
            --_pos;
            return *this;
        }
        constexpr inline ConstIterator operator++(int) noexcept
        {
            ConstIterator temp = *this;
            ++_pos;
            return temp;
        }
        constexpr inline ConstIterator operator--(int) noexcept
        {
            ConstIterator temp = *this;
            --_pos;
            return temp;
        }
        constexpr inline difference_type operator-(const ConstIterator &other) const noexcept
        {
            return static_cast<difference_type>(_pos - other._pos);
        }
        constexpr inline bool operator!=(const ConstIterator &other) const noexcept { return _pos != other._pos; }
        constexpr inline bool operator==(const ConstIterator &other) const noexcept { return _pos == other._pos; }
        constexpr inline bool operator<(const ConstIterator &other) const noexcept { return _pos < other._pos; }
        constexpr inline bool operator<=(const ConstIterator &other) const noexcept { return _pos <= other._pos; }
        constexpr inline bool operator>(const ConstIterator &other) const noexcept { return _pos > other._pos; }
        constexpr inline bool operator>=(const ConstIterator &other) const noexcept { return _pos >= other._pos; }
    };

public:
    // ------------------------------------------默认构造--------------------------------------------------
    constexpr inline IterableUInt() noexcept : _value{0} {}

    // ------------------------------------------从整数构造--------------------------------------------------
    constexpr inline IterableUInt(T value) noexcept : _value{value} {}
    template <typename U, typename = std::enable_if_t<!std::is_convertible_v<U, T>>>
    explicit constexpr inline IterableUInt(U value) noexcept : _value{static_cast<T>(value)} {}
    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>, typename = void>
    constexpr inline IterableUInt(U value) noexcept : _value{static_cast<T>(value)} {}

    // ------------------------------------------从同类构造--------------------------------------------------
    template <typename U, typename = std::enable_if_t<!std::is_convertible_v<U, T>>>
    explicit constexpr inline IterableUInt(const IterableUInt<U> &other) noexcept : _value{static_cast<T>(other._value)} {}
    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>, typename = void>
    constexpr inline IterableUInt(const IterableUInt<U> &other) noexcept : _value{static_cast<T>(other._value)} {}

    // ------------------------------------------向整型转换--------------------------------------------------
    constexpr inline operator T() const noexcept { return _value; }
    template <typename U, typename = std::enable_if_t<!std::is_convertible_v<T, U>>>
    explicit constexpr inline operator U() const noexcept { return static_cast<U>(_value); }
    template <typename U, typename = std::enable_if_t<std::is_convertible_v<T, U>>, typename = void>
    constexpr inline operator U() const noexcept { return static_cast<U>(_value); }

    // ------------------------------------------二元运算符--------------------------------------------------
    constexpr inline IterableUInt &operator+=(const IterableUInt &other) noexcept
    {
        _value += other._value;
        return *this;
    }

    constexpr inline IterableUInt &operator-=(const IterableUInt &other) noexcept
    {
        _value -= other._value;
        return *this;
    }

    constexpr inline IterableUInt &operator*=(const IterableUInt &other) noexcept
    {
        _value *= other._value;
        return *this;
    }

    constexpr inline IterableUInt &operator/=(const IterableUInt &other) noexcept
    {
        _value /= other._value;
        return *this;
    }

    constexpr inline IterableUInt &operator%=(const IterableUInt &other) noexcept
    {
        _value %= other._value;
        return *this;
    }

    constexpr inline IterableUInt &operator&=(const IterableUInt &other) noexcept
    {
        _value &= other._value;
        return *this;
    }

    constexpr inline IterableUInt &operator|=(const IterableUInt &other) noexcept
    {
        _value |= other._value;
        return *this;
    }

    constexpr inline IterableUInt &operator^=(const IterableUInt &other) noexcept
    {
        _value ^= other._value;
        return *this;
    }

    constexpr inline IterableUInt &operator<<=(size_t shift) noexcept
    {
        _value <<= shift;
        return *this;
    }

    constexpr inline IterableUInt &operator>>=(size_t shift) noexcept
    {
        _value >>= shift;
        return *this;
    }

    // ------------------------------------------一元运算符--------------------------------------------------
    constexpr inline IterableUInt operator+() const noexcept
    {
        return *this;
    }

    constexpr inline IterableUInt operator-() const noexcept
    {
        return IterableUInt(-_value);
    }

    constexpr inline IterableUInt operator~() const noexcept
    {
        return IterableUInt(~_value);
    }

    constexpr inline IterableUInt &operator++() noexcept
    {
        ++_value;
        return *this;
    }

    constexpr inline IterableUInt &operator--() noexcept
    {
        --_value;
        return *this;
    }

    constexpr inline IterableUInt operator++(int) noexcept
    {
        IterableUInt temp(*this);
        ++_value;
        return temp;
    }

    constexpr inline IterableUInt operator--(int) noexcept
    {
        IterableUInt temp(*this);
        --_value;
        return temp;
    }
    // ------------------------------------------比较运算符--------------------------------------------------
    constexpr inline bool operator==(const IterableUInt &other) const noexcept
    {
        return _value == other._value;
    }

    constexpr inline bool operator!=(const IterableUInt &other) const noexcept
    {
        return _value != other._value;
    }

    constexpr inline bool operator<(const IterableUInt &other) const noexcept
    {
        return _value < other._value;
    }

    constexpr inline bool operator<=(const IterableUInt &other) const noexcept
    {
        return _value <= other._value;
    }

    constexpr inline bool operator>(const IterableUInt &other) const noexcept
    {
        return _value > other._value;
    }

    constexpr inline bool operator>=(const IterableUInt &other) const noexcept
    {
        return _value >= other._value;
    }

    constexpr inline bool operator==(T value) const noexcept
    {
        return _value == value;
    }

    constexpr inline bool operator!=(T value) const noexcept
    {
        return _value != value;
    }

    constexpr inline bool operator<(T value) const noexcept
    {
        return _value < value;
    }

    constexpr inline bool operator<=(T value) const noexcept
    {
        return _value <= value;
    }

    constexpr inline bool operator>(T value) const noexcept
    {
        return _value > value;
    }

    constexpr inline bool operator>=(T value) const noexcept
    {
        return _value >= value;
    }

    // ------------------------------------------随机访问--------------------------------------------------
    constexpr inline Iterator begin() noexcept { return Iterator(_value, 0); }
    constexpr inline Iterator end() noexcept { return Iterator(_value, bit_count); }
    constexpr inline ConstIterator begin() const noexcept { return ConstIterator(_value, 0); }
    constexpr inline ConstIterator end() const noexcept { return ConstIterator(_value, bit_count); }
    constexpr inline ConstIterator cbegin() noexcept { return ConstIterator(_value, 0); }
    constexpr inline ConstIterator cend() noexcept { return ConstIterator(_value, bit_count); }
    constexpr inline Bit operator[](size_t pos) noexcept { return Bit(_value, pos); }
    constexpr inline ConstBit operator[](size_t pos) const noexcept { return ConstBit(_value, pos); }
};

using IterableUInt8 = IterableUInt<uint8_t>;
using IterableUInt16 = IterableUInt<uint16_t>;
using IterableUInt32 = IterableUInt<uint32_t>;

// 黑白像素类，不含灰
enum class BWPixel : int8_t
{
    Black = 0,
    White = 1,
    Transparent = -1 // 透明
};

// OLED 位图
template <uint8_t X /*行数*/, uint8_t Y /*列数 */>
class OLEDBitMap
{
private:
    std::array<std::array<BWPixel, Y>, X> _pixels;

public:
    constexpr static uint8_t row_num = X;
    constexpr static uint8_t col_num = Y;
    constexpr static auto size = std::make_pair(X, Y);

    constexpr OLEDBitMap(const std::array<uint8_t, ceil_div<uint8_t>(X, 8) * Y> &pic = {}, BWPixel zero_equals_to = BWPixel::Transparent);

    constexpr inline auto &operator[](uint8_t x) noexcept
    {
        EMBMARTIN_NON_BLOCKING_ASSERT(x < X, "out of range", &console);
        return this->_pixels[x];
    };

    constexpr inline const auto &operator[](uint8_t x) const noexcept
    {
        EMBMARTIN_NON_BLOCKING_ASSERT(x < X, "out of range", &console);
        return this->_pixels[x];
    };

    constexpr inline auto &operator[](const Coordinate<uint8_t, 0, X - 1, 0, Y - 1> &c) noexcept
    {
        const auto &[row_index, col_index] = c;
        EMBMARTIN_NON_BLOCKING_ASSERT((row_index < X) && (col_index < Y), "out of range", &console);
        return this->_pixels[row_index][col_index];
    };

    constexpr inline const auto &operator[](const Coordinate<uint8_t, 0, X - 1, 0, Y - 1> &c) const noexcept
    {
        const auto &[row_index, col_index] = c;
        EMBMARTIN_NON_BLOCKING_ASSERT((row_index < X) && (col_index < Y), "out of range", &console);
        return this->_pixels[row_index][col_index];
    };
};

template <uint8_t X, uint8_t Y>
constexpr OLEDBitMap<X, Y>::OLEDBitMap(const std::array<uint8_t, ceil_div<uint8_t>(X, 8) * Y> &pic, BWPixel zero_equals_to)
{
    uint8_t current_start_row = 0;
    uint8_t current_remained_row = X;
    uint8_t current_col = 0;
    for (const IterableUInt8 &_8_col_pix : pic)
    {
        for (uint8_t i = 0; (i < 8) && (i < current_remained_row); i++)
        {
            this->_pixels[current_start_row + i][current_col] =
                _8_col_pix[i] ? BWPixel::White : zero_equals_to;
        }
        ++current_col;
        if (current_col == Y)
        {
            current_col = 0;
            current_remained_row -= 8;
            current_start_row += 8;
            if (current_start_row >= X)
                return;
        }
    }
}

// OLED 西文字模
template <OLEDFontSize font_size = OLEDFontSize::F8x16>
class WesternFont
{
private:
    char _char;
    BWPixel _background;

public:
    constexpr static uint8_t row_num = (font_size == OLEDFontSize::F6x8) ? 8 : 16;
    constexpr static uint8_t col_num = static_cast<uint8_t>(font_size);
    constexpr static auto size = std::make_pair(row_num, col_num);

    constexpr inline WesternFont(char c, BWPixel background = BWPixel::Transparent) noexcept
        : _char{c}, _background{background} {}
    constexpr inline operator OLEDBitMap<row_num, col_num>() const noexcept
    {
        if constexpr (font_size == OLEDFontSize::F6x8)
        {
            std::array<uint8_t, 6> data{};
            for (uint8_t i = 0; i < 6; ++i)
            {
                data[i] = OLED_F6x8[_char - ' '][i];
            }
            return {data, _background};
        }
        else // if constexpr (font_size == OLEDFontSize::F8x16)
        {
            std::array<uint8_t, 16> data{};
            for (uint8_t i = 0; i < 16; ++i)
            {
                data[i] = OLED_F8x16[_char - ' '][i];
            }
            return {data, _background};
        }
    }
};

template <uint8_t X, uint8_t Y>
class ExplicitFunc
{
private:
    double (*_func)(double);

public:
    constexpr static uint8_t row_num = X;
    constexpr static uint8_t col_num = Y;
    constexpr static auto size = std::make_pair(X, Y);

    constexpr inline ExplicitFunc(double (*func)(double), BWPixel background = BWPixel::Transparent) noexcept
        : _func{func} {}
    constexpr inline operator OLEDBitMap<row_num, col_num>() const noexcept
    {
        OLEDBitMap<row_num, col_num> bitmap{};
        for (uint8_t y = 0; y < col_num; ++y)
        {
            uint8_t rslt = std::round(_func(y));
            if ((rslt < X) && (rslt >= 0))
                bitmap[X - rslt - 1][y] = BWPixel::White;
        }
        return bitmap;
    };
};

EMBMARTIN_OLED_NAMESPACE_END

#endif // ! EMBMARTIN_OLEDUtility_H