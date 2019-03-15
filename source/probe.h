/*__ GPL 3.0, 2019 Alexander Soloviev (no.friday@yandex.ru) */

#pragma once

#include <unistd.h>
#include <sys/mman.h>

#include <memory>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <functional>

#include "error.h"
#include "file.h"

class TProbe {
public:
    using TFunc = std::function<void(NUtils::TSpan&)>;

    TProbe()
        : items(64 * 1024)
    {
        array = array_t(new uint8_t[items]);
    }

    void operator()(const NOs::TMemRg &mem, const TFunc &feed) const
    {
        const size_t gran = mem.gran();

        if (mem.paged() > 0) {
            char *it = mem;

            NUtils::TSpan accum(0, 0);

            const size_t pages = mem.pages();

            for (size_t page = 0; page < pages; page += items) {
                const size_t chunk = std::min(pages - page, items);
                const size_t bytes = chunk * gran;

                if (mincore(it, bytes, array.get()) < 0 )
                    throw TError("error happens while mincore() invocation");

                for (size_t z = 0; z < chunk; z++) {
                    if (array[z] & 0x01) {
                        NUtils::TSpan span((page + z) * gran, gran);

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
