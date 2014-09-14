/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_BANDS
#define H_FINCORE_BANDS

#include <cassert>
#include "parts.h"

namespace Stats {
    class Band {
    public:
        Band(size_t at_, size_t limit_)
                : at(at_), limit(limit_), value(0) { }

        bool operator ==(size_t offset) const noexcept {
            return offset >= at && offset < after();
        }

        bool empty() const noexcept {
            return value == 0;
        }

        bool full() const noexcept {
            return value >= limit;
        }

        double usage() const noexcept {
            return limit > 0 ? (double)value / limit : 0;
        }

        size_t after() const noexcept {
            return at + limit;
        }

        void inc(Utils::Span &span) noexcept
        {
            assert(span.at >= at);

            if (*this == span.at) {
                size_t piece = limit - (span.at - at);

                value += span.advance(piece);

                assert(value <= limit);
            }
        }

        size_t      at;
        size_t      limit;
        size_t      value;
    };

    class Bands {
    public:
        using Vec = std::vector<Band>;

        Bands(size_t size, size_t slots) : all(0, size)
        {
            bands.reserve(slots);
        }

        operator const Vec&() const noexcept {
            return bands;
        }

        operator const Band&() const noexcept {
            return all;
        }

        double raito() const noexcept {
            return all.usage();
        }

        size_t size() const noexcept {
            return bands.size();
        }

    protected:
        void accum(Utils::Span span) noexcept
        {
            all.inc(span);

            assert(!span);
        }

        Band        all;
        Vec         bands;
    };

    template<template<typename Fwd> class  Algo>
    class Parted : public Bands {
    public:
        using Ref = std::unique_ptr<Parted<Algo>>;
        using Iter = Parts::Range::const_iterator;

        Parted(size_t size, size_t slots) : Bands(size, slots)
        {
            using namespace Parts;

            auto make = [&](size_t z, Iter &at, Iter &end) {
                bands.emplace_back(at, end - at);
            };

            limit = Algo<Range>(Range(0, size), slots)(make);

            assert(size == bands.back().after());
        }

        void operator()(Utils::Span &span) noexcept
        {
            if (span) {
                accum(span);

                auto it = bands.begin();

                it += span.at / limit;

                assert(it->at <= span.at);

                it = std::find(it, bands.end(), span.at);

                for (; span && it != bands.end(); it++) {

                    it->inc(span);
                }
            }

            assert(!span);
        }

    protected:
        size_t      limit = 0;
    };
}

#endif/*H_FINCORE_BANDS*/
