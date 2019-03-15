/*__ GPL 3.0, 2019 Alexander Soloviev (no.friday@yandex.ru) */

#pragma once

#include <cassert>
#include <cmath>
#include <chrono>

namespace NDecay {
    template<typename TDelta> struct TCfg {
        TCfg(TDelta depth_) : Depth(depth_) { }

        double operator()(TDelta delta) const noexcept
        {
            double coeff = Depth.count();

            return std::exp(-delta.count() / coeff);
        }

        static double Zero() noexcept { return 0; }

        TDelta Depth{ 0 };
    };

    class TValue {
    public:
        using TUnit = ssize_t;

        operator TUnit() const noexcept {
            return State;
        }

        TUnit operator ()(double pass, TUnit value) noexcept
        {
            State = (1 - pass) * value + pass * State;

            return State + 0.5;
        }

    protected:
        double State = 0;
    };
}

