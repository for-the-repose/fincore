/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_DIFF
#define H_FINCORE_DIFF

#include <cassert>

#include "misc.h"
#include "bands.h"

namespace Stats {

    class Diff {
    public:
        using Vec = Bands::Vec;

        double operator()(const Bands &one, const Bands &two) const noexcept
        {
            return std::max(diff(one, two), spacial(one, two));
        }

        double spacial(const Vec &one, const Vec &two) const noexcept
        {
            assert(one.size() == two.size());

            double accum = 0;

            for (size_t z = 0; z < one.size(); z++) {

                accum += Diff::diff(one[z], two[z]);
            }

            return accum;
        }

        static double diff(const Band &one, const Band &two) noexcept
        {
            double diff = Utils::Misc::Diff(one.value, two.value);
            size_t total = one.limit + two.limit;

            return total > 0 ? (2 * diff / total) : 0;
        }
    };
}

#endif/*H_FINCORE_DIFF*/
