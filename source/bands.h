/*__ GPL 3.0, 2019 Alexander Soloviev (no.friday@yandex.ru) */

#pragma once

#include <cassert>
#include <vector>
#include "parts.h"

namespace NStats {

    struct TBand {
        TBand(size_t at_, size_t limit_)
                : At(at_), Limit(limit_) { }

        bool operator ==(size_t offset) const noexcept {
            return offset >= At && offset < After();
        }

        bool Empty() const noexcept { return Value == 0; }

        bool Full() const noexcept { return Value >= Limit; }

        size_t After() const noexcept { return At + Limit; }

        double Usage() const noexcept {
            return Limit > 0 ? (double)Value / Limit : 0;
        }

        void Inc(NUtils::TSpan &span) noexcept
        {
            assert(span.at >= At);

            if (*this == span.at) {
                size_t piece = Limit - (span.at - At);

                Value += span.advance(piece);

                assert(Value <= Limit);
            }
        }

        size_t At = 0;
        size_t Limit = 0;
        size_t Value = 0;
    };

    class TBands {
    public:
        using TVec = std::vector<TBand>;

        TBands(size_t size, size_t slots) : All(0, size)
        {
            Bands.reserve(slots);
        }

        operator const TVec&() const noexcept { return Bands; }

        operator const TBand&() const noexcept { return All; }

        double Raito() const noexcept { return All.Usage(); }

        size_t Size() const noexcept { return Bands.size(); }

    protected:
        void Accum(NUtils::TSpan span) noexcept
        {
            All.Inc(span);

            assert(!span);
        }

        TBand All;
        TVec  Bands;
    };

    template<template<typename Fwd> class  Algo>
    class TParted : public TBands {
    public:
        using Ref = std::unique_ptr<TParted<Algo>>;
        using TIter = NParts::TRange::const_iterator;

        TParted(size_t size, size_t slots) : TBands(size, slots)
        {
            using namespace NParts;

            auto make = [&](size_t z, TIter &at, TIter &end) {
                Bands.emplace_back(at, end - at);
            };

            Limit = Algo<TRange>(TRange(0, size), slots)(make);

            assert(size == Bands.back().After());
        }

        void operator()(NUtils::TSpan &span) noexcept
        {
            if (span) {
                Accum(span);

                auto it = Bands.begin() + (span.at / Limit);

                assert(it->At <= span.at);

                it = std::find(it, Bands.end(), span.at);

                for (; span && it != Bands.end(); it++) {
                    it->Inc(span);
                }
			}

            assert(!span);
        }

    protected:
        size_t Limit = 0;
    };
}
