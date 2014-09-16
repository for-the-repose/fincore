/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_PARTS
#define H_FINCORE_PARTS

#include <functional>
#include <iterator>
#include <cassert>
#include "misc.h"

namespace Parts {
    class Scale {
    public:
        Scale(size_t lower_) : lower(lower_) { }

        size_t operator ()(size_t size) noexcept
        {
            chunk = Misc::Gran2Down(size / lower);
            slots = Misc::DivUp(size, chunk);

            return slots;
        }

    protected:
        size_t      lower;
        size_t      slots   = 0;
        size_t      chunk   = 0;
    };

    class Range {
    public:
        using Ibase = std::iterator<std::random_access_iterator_tag, size_t>;

        class const_iterator : public Ibase {
            using Iter = const_iterator;

        public:
            const_iterator(size_t at_) : at(at_) { }

            operator size_t() const noexcept {
                return at;
            }

            size_t operator *() const noexcept {
                return at;
            }

            Iter& operator+=(size_t inc) noexcept {
                at += inc;

                return *this;
            }

            Iter& operator++() noexcept {
                at++;

                return *this;
            }

            Iter operator++(int) noexcept {
                at++;

                return *this;
            }

        protected:
            size_t      at;
        };

        Range(size_t length) : start(0), stop(length) { }

        Range(size_t start_, size_t stop_)
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
        size_t      start;
        size_t      stop;
    };

    template<typename Fwd, typename Above> class Base_ {
    public:
        using Iter = typename Fwd::const_iterator;
        using Func = std::function<void(size_t seq, Iter&, Iter&)>;

        Base_(const Fwd &fwd_, size_t parts) noexcept
                : total(fwd_.size()), fwd(fwd_)
        {
            limit = parts < total ? Misc::DivUp(total, parts) : 1;
        }

        size_t operator ()(const Func &func) const noexcept
        {
            size_t part = 0, off = 0;

            auto *child = static_cast<const Above*>(this);

            for (Iter it = fwd.begin(); it != fwd.end();) {
                const size_t step = child->advance(off);

                Iter next = std::next(it, step);

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

    template<typename Fwd> class Tailed : public Base_<Fwd, Tailed<Fwd>> {
    public:
        using Base = Base_<Fwd, Tailed<Fwd>>;

        Tailed(const Fwd &fwd_, size_t parts) noexcept : Base(fwd_, parts)
        {

        }

        size_t advance(size_t off) const noexcept
        {
            return std::min(Base::total - off, Base::limit);
        }
    };

    template<typename Fwd> class Equal : public Base_<Fwd, Equal<Fwd>> {
    public:
        using Base = Base_<Fwd, Equal<Fwd>>;

        Equal(const Fwd &fwd_, size_t parts) noexcept : Base(fwd_, parts)
        {
            edge = Base::limit * (Base::total - parts * (Base::limit - 1));
        }

        size_t advance(size_t off) const noexcept
        {
            return Base::limit - bool(off >= edge);
        }

    protected:
        size_t      edge;
    };
}

#endif/*H_FINCORE_PARTS*/
