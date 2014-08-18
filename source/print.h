/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_PRINT
#define H_FINCORE_PRINT

#include <cassert>
#include <iostream>
#include <iomanip>

#include "humans.h"

namespace Stats {
    class Print {
    public:
        Print(const Bands &bands_) : bands(bands_) { }

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

        static std::string Dots(const Bands::Vec &vec) noexcept
        {
            std::string dots;

            dots.reserve(vec.size());

            for (const auto &band: vec) {
                const char syms[] = ".,~0123456789+";

                dots.append(1, syms[Class_0_13(band)]);
            }

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
        const Bands     &bands;
    };

    std::ostream& operator<<(std::ostream &os, const Print &pr) noexcept
    {
        return pr(os);
    }
}

#endif/*H_FINCORE_PRINT*/
