/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_STATS
#define H_FINCORE_STATS

#include <cassert>
#include <iostream>
#include <iomanip>

#include "iface.h"

namespace Stats {
    class Sum : public IFact {
    public:
        class Feed : public IFeed {
        public:
            Feed(const Args &args) : total(args.paged()) , used(0) { }

            void operator () (Span &span) noexcept
            {
                used += span.bytes;
            }

            void freeze() noexcept { }

            void desc() const noexcept
            {
                std::cerr 
                    << "usage " << total << " of " << used << std::endl;
            }

            bool diff(const IFeed &feed_, float thresh) const noexcept
            {
                assert(thresh > 0 && thresh <= 1.0);

                const Feed &feed = dynamic_cast<const Feed&>(feed_);

                auto diff = (double)(used + feed.used) / (total + feed.total);

                return (total != feed.total) || diff > thresh;
            }

        protected:
            size_t      total;
            size_t      used;
        };

        FeedRef make(const Args &args)
        {
            auto feed = std::make_shared<Feed>(args);

            return std::static_pointer_cast<IFeed>(feed);
        }
    };


    class Bands : public IFact {
        class Band {
        public:
            Band(size_t at_, size_t limit_)
                    : at(at_), limit(limit_), value(0) { }

            bool operator ==(size_t offset) const noexcept
            {
                return offset >= at && offset < after();
            }

            bool isEmpty() const noexcept
            {
                return value == 0;
            }

            bool isFull() const noexcept
            {
                return value >= limit;
            }

            double usage() const noexcept
            {
                return (double)value / limit;
            }

            size_t after() const noexcept
            {
                return at + limit;
            }

            void inc(Span &span) noexcept
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

    public:
        class Feed : public Sum::Feed {
            using bands_t = std::vector<Band>;

        public:
            Feed(const Args &args, size_t slots) : Sum::Feed(args)
            {
                const size_t bytes = args.paged();
            
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

            void operator()(Span &span) noexcept
            {
                Sum::Feed::operator()(span);

                if (span) {
                    bands_t::iterator it = bands.begin();

                    it += span.at / limit;

                    assert(it->at <= span.at);

                    it = std::find(it, bands.end(), span.at);

                    for (; span && it != bands.end(); it++) {

                        it->inc(span);
                    }
                }

                assert(!span);
            }

            void desc() const noexcept
            {
                std::string dots;

                dots.reserve(bands.size());

                for (const auto &it: bands) {
                    if (it.isEmpty()) {
                        dots.append(1, '.');

                    } else if (it.isFull()) {
                        dots.append(1, '+');

                    } else {
                        const double fill = it.usage();

                        if (fill < 0.001) {
                            dots.append(1, ',');

                        } else if (fill  < 0.01) {
                            dots.append(1, '~');

                        } else {
                            dots.append(1, '0' + int(fill * 10));

                            assert(dots.back() <= '9');
                        }
                    }
                }

                double raito = used * 100. / total;

                std::cerr
                    << std::fixed << std::setprecision(1) << std::setw(5)
                    << raito << "%" << " [" << dots << "]" << std::endl;
            }

            bool diff(const IFeed &feed_, float thresh) const noexcept
            {
                const Feed &feed = dynamic_cast<const Feed&>(feed_);

                return Sum::Feed::diff(feed, thresh)
                            || diff(feed.bands, thresh);
            }

        protected:
            bool diff(const bands_t &vec, float thresh) const noexcept
            {
                size_t slots = std::min(bands.size(), vec.size());

                double diff = 0;

                for (size_t z = 0; z < slots; z++) {
                    const double pa = bands[z].usage();
                    const double pb = vec[z].usage();

                    diff += std::max(pa, pb) - std::min(pa, pb);
                }

                return diff > thresh;
            }

            size_t      limit;
            bands_t     bands;
        };

        Bands(size_t bands_) : bands(bands_)
        {

        }

        FeedRef make(const Args &args)
        {
            auto feed = std::make_shared<Feed>(args, bands);

            return std::static_pointer_cast<IFeed>(feed);
        }

    protected:
        const size_t    bands;
    };
}

#endif/*H_FINCORE_STATS*/
