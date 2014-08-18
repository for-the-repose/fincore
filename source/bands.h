/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_BANDS
#define H_FINCORE_BANDS

#include <cassert>

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
        using Ref = std::unique_ptr<Bands>;
        using Vec = std::vector<Band>;

        Bands(const Utils::Gran &gran, size_t slots)
            : limit(0), all(0, gran.paged())
        {
            const size_t bytes = gran.paged();

            if (slots < bytes) {
                limit = (bytes + slots - 1) / slots;

                assert(limit > 1);

            } else {
                limit = 1;
            }

            size_t edge = limit * (bytes - slots * (limit - 1));

            bands.reserve(slots);

            for (size_t off = 0; off < bytes;) {
                bands.emplace_back(off, limit - bool(off >= edge));

                off += bands.back().limit;
            }

            assert(bands.back().after() == bytes);
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
        void accum(Utils::Span span) noexcept
        {
            all.inc(span);

            assert(!span);
        }

        size_t      limit;
        Band        all;
        Vec         bands;
    };
}

#endif/*H_FINCORE_BANDS*/
