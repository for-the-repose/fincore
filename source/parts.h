/*__ GPL 3.0, 2019 Alexander Soloviev (no.friday@yandex.ru) */

#pragma once

#include <functional>
#include <iterator>
#include <cassert>
#include "misc.h"

namespace NParts {

    class TScale {
    public:
        TScale(size_t lower_) : lower(lower_) { }

        size_t operator ()(size_t size) noexcept
        {
            chunk = NMisc::Gran2Down(size / lower);
            slots = NMisc::DivUp(size, chunk);

            return slots;
        }

    protected:
        size_t      lower;
        size_t      slots   = 0;
        size_t      chunk   = 0;
    };

    class TRange {
    public:
        using IBase = std::iterator<std::random_access_iterator_tag, size_t>;

        class const_iterator : public IBase {
            using TIter = const_iterator;

        public:
            const_iterator(size_t at_) : at(at_) { }

            operator size_t() const noexcept {
                return at;
            }

            size_t operator *() const noexcept {
                return at;
            }

            TIter& operator+=(size_t inc) noexcept {
                return at += inc, *this;
            }

            TIter& operator++() noexcept {
                return at++, *this;
            }

            TIter operator++(int) noexcept {
                return at++, *this;
            }

        protected:
            size_t      at;
        };

        TRange(size_t length) : TRange(0, length) { }

        TRange(size_t start_, size_t stop_)
            : start(start_), stop(stop_) { }

        size_t size() const noexcept {
            return stop - start;
        }

        const_iterator begin() const noexcept {
            return start;
        }

        const_iterator end() const noexcept {
            return stop;
        }

    protected:
        size_t start = 0;
        size_t stop = 0;
    };

    template<typename Fwd, typename Above> class TBase_ {
    public:
        using TIter = typename Fwd::const_iterator;
        using TFunc = std::function<void(size_t seq, TIter&, TIter&)>;

        TBase_(const Fwd &fwd_, size_t parts) noexcept
                : total(fwd_.size()), fwd(fwd_)
        {
            limit = parts < total ? NMisc::DivUp(total, parts) : 1;
        }

        size_t operator ()(const TFunc &func) const noexcept
        {
            size_t part = 0, off = 0;

            auto *child = static_cast<const Above*>(this);

            for (TIter it = fwd.begin(); it != fwd.end();) {
                const size_t step = child->advance(off);

                TIter next = std::next(it, step);

                func(part, it, next);

                it = next, part++, off += step;
            }

            assert(off == total);

            return limit;
        }

    protected:
        size_t      total   = 0;
        size_t      limit   = 0;
        const Fwd   &fwd;
    };

    template<typename Fwd> class Tailed : public TBase_<Fwd, Tailed<Fwd>> {
    public:
        using TBase = TBase_<Fwd, Tailed<Fwd>>;

        Tailed(const Fwd &fwd_, size_t parts) noexcept : TBase(fwd_, parts)
        {

        }

        size_t advance(size_t off) const noexcept
        {
            return std::min(TBase::total - off, TBase::limit);
        }
    };

    template<typename Fwd> class Equal : public TBase_<Fwd, Equal<Fwd>> {
    public:
        using TBase = TBase_<Fwd, Equal<Fwd>>;

        Equal(const Fwd &fwd_, size_t parts) noexcept : TBase(fwd_, parts)
        {
            edge = TBase::limit * (TBase::total - parts * (TBase::limit - 1));
        }

        size_t advance(size_t off) const noexcept
        {
            return TBase::limit - bool(off >= edge);
        }

    protected:
        size_t edge = 0;
    };
}
