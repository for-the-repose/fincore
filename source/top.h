/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_TOP
#define H_FINCORE_TOP

#include <map>
#include <memory>
#include "walk.h"
#include "probe.h"
#include "humans.h"

class Top {
public:
    class Cfg {
    public:
        enum EReduct {
            REDUCT_NONE     = 0,
            REDUCT_TOP      = 1
        };

        const Cfg& validate()
        {
            using namespace std;

            raito = min(1., max(0., raito));

            return *this;
        }

        unsigned    edge    = -1;
        bool        zeroes  = false;
        bool        summary = false;
        EReduct     reduct  = REDUCT_NONE;
        unsigned    limit   = 16;
        double      raito   = 0.;
    };

    class Entry {
        using Ref = Utils::Dir::Ref;

    public:
        using Key = size_t;

        Entry(size_t size, size_t used, Ref &&ref) noexcept
            : Used(used), Size(size), Label(std::move(ref)) { }

        Entry() : Entry(0, 0, Ref()) { }

        Entry(Entry &&entry) noexcept : Entry()
        {
            *this = std::move(entry);
        }

        operator bool() const noexcept
        {
            return Label;
        }

        bool operator <(const Entry &rval) const noexcept
        {
            return Used < rval.Used;
        }

        Entry& operator +=(const Entry &rval) noexcept
        {
            assert(Label.depth < rval.Label.depth);

            Used    += rval.Used;
            Size    += rval.Size;

            return *this;
        }

        Entry& operator =(Entry &&rval) noexcept
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
        virtual void push(Entry&&) noexcept = 0;
        virtual Entry pop() noexcept = 0;
    };

    class ReTop : public IReduct {
    public:
        ReTop(size_t limit_) : limit(limit_) { }

    protected:
        void push(Entry &&entry) noexcept override
        {
            heap.emplace(entry.Used, std::move(entry));

            if (heap.size() > limit) {

                heap.erase(heap.begin());
            }
        }

        Entry pop() noexcept override
        {
            Entry   last;

            if (heap.size() > 0) {
                last = std::move(heap.rbegin()->second);

                heap.erase(std::prev(heap.end()));
            }

            return std::move(last);
        }

    private:
        using Heap = std::multimap<Entry::Key, Entry>;

        size_t      limit;
        Heap        heap;
    };

    Top(const Cfg &cfg_) : cfg(cfg_) { }

    void Do(const std::string &root)
    {
        Utils::Dir::Walk walk(root);

        Do(root, walk);
    }

    void Do(std::istream &in)
    {
        Utils::Dir::List list(in);

        Do("", list);
    }

protected:
    void Do(const std::string &root, Utils::Dir::Enum &walk)
    {
        using namespace Utils;

        MakeReductor();

        Probe       probe;
        Entry       top(0, 0, Dir::Ref(OS::EDir, 0, ":summary"));
        Entry       aggr;

        while(walk) {
            auto ref = walk.next();

            if (aggr && !aggr.Label.IsAbove(ref))
                Feed(std::move(aggr));

            if (ref.type == OS::EDir) {
                if (ref.depth == cfg.edge) {
                    assert(!aggr);

                    aggr = Entry(0, 0, std::move(ref));
                }

            } else if (ref.type == OS::EReg) {
                OS::File file;

                const auto path = Dir::Path(root).add(ref);

                try {
                    file = OS::File(path);

                } catch (Error &error) {
                    std::cerr << "cannot open file " << ref.name << std::endl;

                    continue;
                }

                if (file.size() > 0) {
                    auto map = file.mmap();

                    Entry entry(((OS::MemRg)map).paged(), 0, std::move(ref));

                    probe(map, [&](Utils::Span &span) { entry.Used += span.bytes;});

                    top += entry;

                    if (aggr) {
                        aggr += entry;

                    } else {

                        Feed(std::move(entry));
                    }
                }

            } else if (ref.type == OS::EAccess) {

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

        if (cfg.reduct == Cfg::REDUCT_TOP) {

            reduct = RePtr(new ReTop(cfg.limit));
        }
    }

    void Feed(Entry entry)
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
        if (reduct) {
            while(Entry last = reduct->pop()) {
                Print(last);
            }

            reduct.reset();
        }
    }

    void Print(const Entry &entry)
    {
        using namespace std;

        cout
            << setw(5) << Humans::Value(entry.Used)
            << " of "
            << setw(5) << Humans::Value(entry.Size)
            << " "
            << setw(2) << entry.Label.depth
            << " "
            << entry.Label.name
            << endl;
    }

    using RePtr = std::unique_ptr<IReduct>;

    const Cfg   &cfg;
    RePtr       reduct;
};

#endif/*H_FINCORE_TOP*/
