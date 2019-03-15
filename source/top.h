/*__ GPL 3.0, 2019 Alexander Soloviev (no.friday@yandex.ru) */

#pragma once

#include <map>
#include <memory>
#include "walk.h"
#include "probe.h"
#include "humans.h"

class TTop {
public:
    struct TCfg {
        enum EReduct {
            REDUCT_NONE     = 0,
            REDUCT_TOP      = 1
        };

        const TCfg& validate()
        {
            raito = std::min(1., std::max(0., raito));

            return *this;
        }

        unsigned    edge    = -1;
        bool        zeroes  = false;
        bool        summary = false;
        EReduct     reduct  = REDUCT_NONE;
        unsigned    limit   = 16;
        double      raito   = 0.;
    };

    class TEntry {
        using Ref = NUtils::NDir::Ref;

    public:
        using TKey = size_t;

        TEntry(size_t size, size_t used, Ref &&ref) noexcept
            : Used(used), Size(size), Label(std::move(ref)) { }

        TEntry() : TEntry(0, 0, { }) { }

        TEntry(TEntry &&entry) noexcept
        {
            *this = std::move(entry);
        }

        explicit operator bool() const noexcept
        {
            return bool(Label);
        }

        bool operator <(const TEntry &rval) const noexcept
        {
            return Used < rval.Used;
        }

        TEntry& operator +=(const TEntry &rval) noexcept
        {
            assert(Label.depth < rval.Label.depth);

            Used    += rval.Used;
            Size    += rval.Size;

            return *this;
        }

        TEntry& operator =(TEntry &&rval) noexcept
        {
            using namespace std;

            swap(Used, rval.Used);
            swap(Size, rval.Size);
            swap(Label, rval.Label);

            return *this;
        }

        double raito() const noexcept
        {
            if (Size == 0 || Size == Used) {
                return 1.0;
            } else {
                return (double)Used / (double)Size;
            }
        }

        size_t      Used    = 0;
        size_t      Size    = 0;
        Ref         Label;
    };

    class IReduct {
    public:
        virtual ~IReduct() { }
        virtual void push(TEntry) noexcept = 0;
        virtual TEntry pop() noexcept = 0;
    };

    class ReTop : public IReduct {
    public:
        ReTop(size_t limit_) : limit(limit_) { }

    protected:
        void push(TEntry entry) noexcept override
        {
            heap.emplace(entry.Used, std::move(entry));

            if (heap.size() > limit) {
                heap.erase(heap.begin());
            }
        }

        TEntry pop() noexcept override
        {
            TEntry   last;

            if (heap.size() > 0) {
                last = std::move(heap.rbegin()->second);
                heap.erase(std::prev(heap.end()));
            }

            return last;
        }

    private:
        using THeap = std::multimap<TEntry::TKey, TEntry>;

        size_t      limit = 0;
        THeap        heap;
    };

    TTop(const TCfg &cfg_) : cfg(cfg_) { }

    void Do(const std::string &root)
    {
        NUtils::NDir::TWalk walk(root);

        Do(root, walk);
    }

    void Do(std::istream &in)
    {
        NUtils::NDir::TList list(in);

        Do("", list);
    }

protected:
    void Do(const std::string &root, NUtils::NDir::IEnum &walk)
    {
        using namespace NUtils;

        MakeReductor();

        TProbe probe;
        TEntry top(0, 0, NDir::Ref(NOs::EDir, 0, ":summary"));
        TEntry aggr;

        while (walk) {
            auto ref = walk.next();

            if (aggr && !aggr.Label.IsAbove(ref))
                Feed(std::move(aggr));

            if (ref.type == NOs::EDir) {
                if (ref.depth == cfg.edge) {
                    assert(!aggr);

                    aggr = TEntry(0, 0, std::move(ref));
                }

            } else if (ref.type == NOs::EReg) {
                NOs::TFile file;

                const auto path = NDir::TPath(root).add(ref);

                try {
                    file = NOs::TFile(path);
                } catch (TError &error) {
                    std::cerr << "cannot open file " << ref.name << std::endl;

                    continue;
                }

                if (file.size() > 0) {
                    auto map = file.mmap();

                    TEntry entry(((NOs::TMemRg)map).paged(), 0, std::move(ref));

                    probe(map, [&](NUtils::TSpan &span) { entry.Used += span.bytes;});

                    top += entry;

                    if (aggr) {
                        aggr += entry;
                    } else {
                        Feed(std::move(entry));
                    }
                }

            } else if (ref.type == NOs::EAccess) {
                std::cerr << "cannot deep to " << ref.name << std::endl;
            }
        }

        if (cfg.summary)
            Print(top);

        Drain();
    }

    void MakeReductor()
    {
        assert(!reduct);

        if (cfg.reduct == TCfg::REDUCT_TOP) {
            reduct = TRePtr(new ReTop(cfg.limit));
        }
    }

    void Feed(TEntry entry)
    {
        if (entry.Used == 0 && !cfg.zeroes) {
            /* ignore unsued entries        */
        } else if (entry.raito() < cfg.raito) {
            /* ignore entries under edge    */
        } else if (reduct) {
            reduct->push(std::move(entry));
        } else {
            Print(entry);
        }
    }

    void Drain()
    {
        if (auto was = std::exchange(reduct, { }))
            while (auto last = was->pop()) Print(last);
    }

    void Print(const TEntry &entry)
    {
        std::cout
            << std::setw(5) << NHumans::Value(entry.Used)
            << " of "
            << std::setw(5) << NHumans::Value(entry.Size)
            << " "
            << std::setw(2) << entry.Label.depth
            << " "
            << entry.Label.name
            << std::endl;
    }

    using TRePtr = std::unique_ptr<IReduct>;

    const TCfg  &cfg;
    TRePtr      reduct;
};
