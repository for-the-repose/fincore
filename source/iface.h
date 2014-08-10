/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_IFACE
#define H_FINCORE_IFACE

#include <memory>
#include <cstdlib>

namespace Stats {
    class Args {
    public:
        Args(size_t gran_, size_t bytes_)
            : gran(gran_), bytes(bytes_)
        {
            pages = (bytes + gran - 1) / gran;

            if (pages < 1) abort();
        }

        size_t bytesPaged() const noexcept
        {
            return pages * gran;
        }

        size_t  gran;   /* page size        */
        size_t  pages;  /* pages in file    */
        size_t  bytes;  /* total bytes      */
    };

    class IFeed {
    public:
        virtual ~IFeed() {

        };

        virtual void operator()(size_t page) noexcept = 0;
        virtual void desc() const noexcept = 0;
        virtual bool diff(const IFeed&, float thresh) const noexcept = 0;
    };

    using FeedRef = std::shared_ptr<IFeed>;

    class IFact {
    public:

        virtual ~IFact() {

        }

        virtual FeedRef make(const Args&) = 0;
    };
}

#endif/*H_FINCODE_IFACE*/
