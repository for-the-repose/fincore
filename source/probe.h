/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_PROBE
#define H_FINCORE_PROBE

#include <unistd.h>
#include <sys/mman.h>

#include <memory>
#include <cstdint>
#include <iostream>
#include <algorithm>

#include "error.h"
#include "file.h"
#include "iface.h"

class Probe {
public:
    Probe()
        : items(64 * 1024)
    {
        gran = getpagesize();
        
        array = array_t(new uint8_t[items]);
    }

    Stats::FeedRef operator()(
                const std::string   &path, 
                Stats::IFact        &fact) const
    {
        File file(path);

        const Stats::Args args(gran, file.size());

        Stats::FeedRef feeder = fact.make(args);

        if (args.paged() > 0) {
            char *it = (char*) file.mmap();

            Utils::Span accum(0, 0);

            for (size_t page = 0; page < args.pages; page += items) {
                const size_t chunk = std::min(args.pages - page, items);
                const size_t bytes = chunk * args.gran;

                if (mincore(it, bytes, array.get()) < 0 )
                    throw Error("error happens while mincore() invocation");

                for (size_t z = 0; z < chunk; z++) {
                    if (array[z] & 0x01) {
                        Utils::Span span((page + z) * gran, gran);

                        if (!accum.join(span)) {
                            (*feeder)(accum);

                            accum = span;
                        }
                    }
                }

                it += bytes;
            }

            if (accum) (*feeder)(accum);
        }

        feeder->freeze();

        return feeder;
    }

protected:
    using array_t = std::unique_ptr<uint8_t[]>;

    size_t      items;
    size_t      gran;
    array_t     array;
};

#endif/*H_FINCORE_PROBE*/
