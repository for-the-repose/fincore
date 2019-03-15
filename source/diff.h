/*__ GPL 3.0, 2019 Alexander Soloviev (no.friday@yandex.ru) */

#pragma once

#include <cassert>

#include "misc.h"
#include "bands.h"

namespace NStats {

    struct TDiff {
        using TVec = TBands::TVec;

        double operator()(const TBands &one, const TBands &two) const noexcept
        {
            return std::max(Diff(one, two), Spacial(one, two));
        }

        double Spacial(const TVec &one, const TVec &two) const noexcept
        {
            assert(one.size() == two.size());

            double accum = 0;

            for (size_t z = 0; z < one.size(); z++) {
                accum += TDiff::Diff(one[z], two[z]);
            }

            return accum;
        }

        static double Diff(const TBand &one, const TBand &two) noexcept
        {
            double diff = NMisc::Diff(one.Value, two.Value);
            size_t total = one.Limit + two.Limit;

            return total > 0 ? (2 * diff / total) : 0;
        }
    };
}
