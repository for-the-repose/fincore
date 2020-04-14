#pragma once /*__ GPL 3.0, 2020 Alexander Soloviev (no.friday@yandex.ru) */

#include <limits>
#include <memory>

template<typename T> using TBox = std::unique_ptr<T>;

template<typename T> constexpr T Max()
{
    return std::numeric_limits<T>::max();
}
