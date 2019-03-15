/*__ GPL 3.0, 2019 Alexander Soloviev (no.friday@yandex.ru) */

#pragma once

#include <cassert>
#include <iostream>
#include <iomanip>

#include "humans.h"
#include "parts.h"

namespace NStats {
    class TPrint {
    public:
        using TIter = TBands::TVec::const_iterator;

        TPrint(const TBands &bands_, size_t slots_ = 0) : bands(bands_)
        {
            slots = slots_ > 0 ? slots_ : bands.Size();
        }

        std::ostream& operator()(std::ostream &os) const noexcept
        {
            using namespace std;

            os
                << fixed << setprecision(1) << setw(5)
                << (bands.Raito() * 100.)
                << "% [" << Dots(bands) << "] "
                << NHumans::Value(((const TBand&)bands).Limit);

            return os;
        }

        std::string Dots(const TBands::TVec &vec) const noexcept
        {
            std::string dots;

            dots.reserve(slots);

            auto put = [&](size_t z, TIter at, TIter end)
            {
                TBand aggr(0, 0);

                for (; at != end; at++) {
                    aggr.Limit += at->Limit;
                    aggr.Value += at->Value;
                }

                const char syms[] = ".,~0123456789+";

                dots.append(1, syms[Class_0_13(aggr)]);
            };

            NParts::Equal<TBands::TVec>(vec, slots)(put);

            assert(dots.size() == slots);

            return dots;
        }

        static unsigned Class_0_13(const TBand &band) noexcept
        {
            if (band.Empty()) {
                return 0;
            } else if (band.Full()) {
                return 13;
            } else {
                const double fill = band.Usage();

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
        const TBands    &bands;
    };

    std::ostream& operator<<(std::ostream &os, const TPrint &pr) noexcept
    {
        return pr(os);
    }
}
