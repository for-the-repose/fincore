/*__ GPL 3.0, 2019 Alexander Soloviev (no.friday@yandex.ru) */

#pragma once

#include "misc.h"

namespace NHumans {

    std::string Value(size_t value)
    {
        if (value < NMisc::Pow10(3)) {
            return std::to_string(value);
        } else {
            const char *scale[] = { "", "K", "M", "G", "T", "P", "E", "Z" };

            auto pow = NMisc::Log1000(value);
            double small = value / NMisc::Pow10((pow - 1) * 3);

            return std::to_string(small / 1000.).substr(0, 4) + *scale[pow];
        }
    }
}
