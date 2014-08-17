/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_PROBE
#define H_FINCORE_PROBE

#include <unistd.h>
#include <sys/mman.h>

#include <memory>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <functional>

#include "error.h"
#include "file.h"

class Probe {
public:
    using Func = std::function<void(Utils::Span&)>;

    Probe()
        : items(64 * 1024)
    {
        array = array_t(new uint8_t[items]);
    }

    void operator()(const OS::MemRg &mem, const Func &feed) const
    {
        const size_t gran = mem.gran();

        if (mem.paged() > 0) {
            char *it = mem;

            Utils::Span accum(0, 0);

            const size_t pages = mem.pages();

            for (size_t page = 0; page < pages; page += items) {
                const size_t chunk = std::min(pages - page, items);
                const size_t bytes = chunk * gran;

                if (mincore(it, bytes, array.get()) < 0 )
                    throw Error("error happens while mincore() invocation");

                for (size_t z = 0; z < chunk; z++) {
                    if (array[z] & 0x01) {
                        Utils::Span span((page + z) * gran, gran);

                        if (!accum.join(span)) {
                            feed(accum);

                            accum = span;
                        }
                    }
                }

                it += bytes;
            }

            if (accum) feed(accum);
        }
    }

protected:
    using array_t = std::unique_ptr<uint8_t[]>;

    size_t      items;
    array_t     array;
};

#endif/*H_FINCORE_PROBE*/
