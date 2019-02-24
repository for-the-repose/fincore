/*__ GPL 3.0, 2019 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_DECAY
#define H_FINCORE_DECAY

#include <cassert>
#include <cmath>
#include <chrono>

namespace Decay {
    template<typename Times>
    class Cfg {
    public:
        Cfg(Times depth_) : depth(depth_) { }

        double operator()(Times delta) const noexcept
        {
            double coeff = depth.count();

            return std::exp(-delta.count() / coeff);
        }

        static double zero() noexcept {
            return 0;
        }

        Times depth{ 0 };
    };

    class Value {
    public:
        using Unit = ssize_t;

        operator Unit() const noexcept {
            return state;
        }

        Unit operator ()(double pass, Unit value) noexcept
        {
            state = (1 - pass) * value + pass * state;

            return state + 0.5;
        }

    protected:
        double state = 0;
    };
}

#endif/*H_FINCORE_DECAY*/
