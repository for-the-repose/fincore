/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_TICKS
#define H_FINCORE_TICKS

#include <chrono>
#include <thread>
#include "decay.h"

namespace Utils {
    template<typename Clock = std::chrono::steady_clock>
    class Ticks {
        using Times = typename Clock::duration;
        using Stamp = typename Clock::time_point;
        using Pass  = Decay::Cfg<Times>;
        using Value = Decay::Value;

        static_assert(Clock::is_steady, "clock should be steady");

    public:
        Ticks(unsigned msecs, unsigned cycles_) : cycles(cycles_)
        {
            using namespace std::chrono;

            tick = duration_cast<Times>(milliseconds(msecs));

            decay = Pass(tick * 3)(tick);
        }

        bool operator()() noexcept
        {
            if (count++ == 0) {
                start = stamp = Clock::now();

            } else if (count < cycles) {
                stamp += tick;

                const auto now = Clock::now();

                auto skip = (stamp - now) - (Times)shift;

                if (count == 2) {
                    spent(Pass::zero(), (now - start).count());

                } else {
                    spent(decay, (now - start).count());
                }

                if (skip > Times::zero()) {
                    std::this_thread::sleep_for(skip);

                    start = Clock::now();

                    auto delta = (Times)shift + (start - stamp);

                    shift(decay, delta.count());

                } else {
                    start = Clock::now();
                }
            }

            return count <= cycles;
        }

        template<typename Duration = Times>
        Duration used() const noexcept
        {
            using namespace std::chrono;

            return duration_cast<Duration>((Times)spent);
        }

    protected:
        unsigned        count   = 0;
        unsigned        cycles;
        Stamp           stamp;
        Stamp           start;
        Times           tick;
        double          decay;
        Value           shift;
        Value           spent;
    };
}

#endif/*H_FINCORE_TICKS*/
