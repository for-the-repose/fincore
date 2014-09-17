/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_HUMANS
#define H_FINCORE_HUMANS

#include "misc.h"

namespace Humans {

    std::string Value(size_t value)
    {
        if (value < Misc::Pow10(3)) {
            return std::to_string(value);

        } else {
            const char *scale[] = { "", "K", "M", "G", "T", "P", "E", "Z" };

            auto pow = Misc::Log1000(value);
            double small = value / Misc::Pow10((pow - 1) * 3);

            return std::to_string(small / 1000.).substr(0, 4) + *scale[pow];
        }
    }
}

#endif/*H_FINCORE_HUMANS*/
