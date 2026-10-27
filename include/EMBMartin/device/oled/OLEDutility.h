#pragma once

#include <cstddef>
#include <cstdint>
#include <climits>
#include <type_traits>
#include <iterator>
#include <array>
#include <cmath>

#include "EMBMartin/macro.h"

#include "EMBMartin/device/oled/OLED_Font.h"
#include "EMBMartin/algorithm/coordinate.h"
#include "EMBMartin/algorithm/iterable_uint.h"

EMBMARTIN_OLED_NAMESPACE_BEGIN

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
    std::array<std::array<BWPixel, Y>, X> __pixels;

public:
    constexpr static uint8_t row_num = X;
    constexpr static uint8_t col_num = Y;
    constexpr static auto size = std::make_pair(X, Y);

    constexpr OLEDBitMap(const std::array<uint8_t, algorithm::ceil_div<uint8_t>(X, 8) * Y> &pic = {}, BWPixel zero_equals_to = BWPixel::Transparent);

    constexpr inline auto &operator[](uint8_t x) noexcept
    {
        return this->__pixels[x];
    };

    constexpr inline const auto &operator[](uint8_t x) const noexcept
    {
        return this->__pixels[x];
    };

    constexpr inline auto &operator[](const algorithm::Coordinate<uint8_t, 0, X - 1, 0, Y - 1> &c) noexcept
    {
        const auto &[row_index, col_index] = c;
        return this->__pixels[row_index][col_index];
    };

    constexpr inline const auto &operator[](const algorithm::Coordinate<uint8_t, 0, X - 1, 0, Y - 1> &c) const noexcept
    {
        const auto &[row_index, col_index] = c;
        return this->__pixels[row_index][col_index];
    };
};

template <uint8_t X, uint8_t Y>
constexpr OLEDBitMap<X, Y>::OLEDBitMap(const std::array<uint8_t, algorithm::ceil_div<uint8_t>(X, 8) * Y> &pic, BWPixel zero_equals_to)
{
    uint8_t current_start_row = 0;
    uint8_t current_remained_row = X;
    uint8_t current_col = 0;
    for (const algorithm::IterableUInt8 &_8_col_pix : pic)
    {
        for (uint8_t i = 0; (i < 8) && (i < current_remained_row); i++)
        {
            this->__pixels[current_start_row + i][current_col] =
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
    char __char;
    BWPixel __background;

public:
    constexpr static uint8_t row_num = (font_size == OLEDFontSize::F6x8) ? 8 : 16;
    constexpr static uint8_t col_num = static_cast<uint8_t>(font_size);
    constexpr static auto size = std::make_pair(row_num, col_num);

    constexpr inline WesternFont(char c, BWPixel background = BWPixel::Transparent) noexcept
        : __char{c}, __background{background} {}
    constexpr inline operator OLEDBitMap<row_num, col_num>() const noexcept
    {
        if constexpr (font_size == OLEDFontSize::F6x8)
        {
            std::array<uint8_t, 6> data{};
            for (uint8_t i = 0; i < 6; ++i)
            {
                data[i] = OLED_F6x8[__char - ' '][i];
            }
            return {data, __background};
        }
        else // if constexpr (font_size == OLEDFontSize::F8x16)
        {
            std::array<uint8_t, 16> data{};
            for (uint8_t i = 0; i < 16; ++i)
            {
                data[i] = OLED_F8x16[__char - ' '][i];
            }
            return {data, __background};
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

