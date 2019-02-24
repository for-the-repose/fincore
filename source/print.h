/*__ GPL 3.0, 2019 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_PRINT
#define H_FINCORE_PRINT

#include <cassert>
#include <iostream>
#include <iomanip>

#include "humans.h"
#include "parts.h"

namespace Stats {
    class Print {
    public:
        using Iter = Bands::Vec::const_iterator;

        Print(const Bands &bands_, size_t slots_ = 0) : bands(bands_)
        {
            slots = slots_ > 0 ? slots_ : bands.size();
        }

        std::ostream& operator()(std::ostream &os) const noexcept
        {
            using namespace std;

            os
                << fixed << setprecision(1) << setw(5)
                << (bands.raito() * 100.)
                << "% [" << Dots(bands) << "] "
                << Humans::Value(((const Band&)bands).limit);

            return os;
        }

        std::string Dots(const Bands::Vec &vec) const noexcept
        {
            std::string dots;

            dots.reserve(slots);

            auto put = [&](size_t z, Iter at, Iter end)
            {
                Band aggr(0, 0);

                for (; at != end; at++) {
                    aggr.limit += at->limit;
                    aggr.value += at->value;
                }

                const char syms[] = ".,~0123456789+";

                dots.append(1, syms[Class_0_13(aggr)]);
            };

            Parts::Equal<Bands::Vec>(vec, slots)(put);

            assert(dots.size() == slots);

            return dots;
        }

        static unsigned Class_0_13(const Band &band) noexcept
        {
            if (band.empty()) {
                return 0;
            } else if (band.full()) {
                return 13;
            } else {
                const double fill = band.usage();

                if (fill < 0.001) {
                    return 1;
                } else if (fill  < 0.01) {
                    return 2;
                } else {
                    return 3 + int(fill * 10);
                }
            }
        }

    protected:
        size_t          slots = 0;
        const Bands     &bands;
    };

    std::ostream& operator<<(std::ostream &os, const Print &pr) noexcept
    {
        return pr(os);
    }
}

#endif/*H_FINCORE_PRINT*/
