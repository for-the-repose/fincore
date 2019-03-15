/*__ GPL 3.0, 2019 Alexander Soloviev (no.friday@yandex.ru) */

#pragma once

#include <chrono>
#include <thread>
#include "decay.h"

namespace NUtils {
    template<typename TClock = std::chrono::steady_clock>
    class TTicks {
        using TDelta = typename TClock::duration;
        using TStamp = typename TClock::time_point;
        using TPass  = NDecay::TCfg<TDelta>;
        using TValue = NDecay::TValue;

        static_assert(TClock::is_steady, "clock should be steady");

    public:
        TTicks(unsigned msecs, unsigned cycles_) : cycles(cycles_)
        {
            using namespace std::chrono;

            tick = duration_cast<TDelta>(milliseconds(msecs));

            decay = TPass(tick * 3)(tick);
        }

        bool operator()() noexcept
        {
            if (count++ == 0) {
                start = stamp = TClock::now();

            } else if (count < cycles) {
                stamp += tick;

                const auto now = TClock::now();

                auto skip = (stamp - now) - (TDelta)shift;

                if (count == 2) {
                    spent(TPass::Zero(), (now - start).count());
                } else {
                    spent(decay, (now - start).count());
                }

                if (skip > TDelta::zero()) {
                    std::this_thread::sleep_for(skip);

                    start = TClock::now();

                    auto delta = (TDelta)shift + (start - stamp);

                    shift(decay, delta.count());

                } else {
                    start = TClock::now();
                }
            }

            return count <= cycles;
        }

        template<typename T = TDelta> T used() const noexcept
        {
            return std::chrono::duration_cast<T>((TDelta)spent);
        }

    protected:
        unsigned        count  = 0;
        unsigned        cycles = 0;
        TStamp          stamp;
        TStamp          start;
        TDelta          tick;
        double          decay;
        TValue          shift;
        TValue          spent;
    };
}
