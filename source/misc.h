/*__ GPL 3.0, 2019 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_MISC
#define H_FINCORE_MISC

namespace Misc {
    template<typename Val>
    Val Diff(const Val &one, const Val &two) {
        return std::max(one, two) - std::min(one, two);
    }

    constexpr size_t Pow10(unsigned xval) {
        return xval < 1 ? 1 : 10 * Pow10(xval - 1);
    }

    constexpr size_t Pow2(unsigned xval) {
        return 1ll << xval;
    }

    unsigned Log1000(size_t val) noexcept
    {
        if (val < Pow10(3))  return 0;
        if (val < Pow10(6))  return 1;
        if (val < Pow10(9))  return 2;
        if (val < Pow10(12)) return 3;
        if (val < Pow10(15)) return 4;
        if (val < Pow10(18)) return 5;

        return 6;
    }

    constexpr size_t DivUp(size_t value, size_t gran) noexcept {
        return (value + gran - 1) / gran;
    }

    constexpr size_t GranDown(size_t value, size_t gran) noexcept {
        return (value / gran) * gran;
    }

    constexpr size_t GranUp(size_t value, size_t gran) noexcept {
        return DivUp(value, gran) * gran;
    }

    size_t Gran2Up(size_t value) noexcept
    {
        if (value > 0) {
            value--;

            value |= value >> 1;
            value |= value >> 2;
            value |= value >> 4;
            value |= value >> 8;
            value |= value >> 16;
            value |= value >> 32;
        }

        return value;
    }

    size_t Gran2Down(size_t value) noexcept {
        return (Gran2Up(value) >> 1) + 1;
    }
}
#endif/*H_FINCORE_MISC*/
