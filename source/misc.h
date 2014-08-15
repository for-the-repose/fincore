/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_MISC
#define H_FINCORE_MISC

namespace Utils {

    namespace Misc {
        constexpr size_t Pow10(unsigned xval) {
            return xval < 1 ? 1 : 10 * Pow10(xval - 1);
        }

        constexpr size_t Pow2(unsigned xval) {
            return 1ll << xval;
        }

        unsigned Log1000(size_t val)
        {
            if (val < Pow10(3))
                return 0;
            if (val < Pow10(6))
                return 1;
            if (val < Pow10(9))
                return 2;
            if (val < Pow10(12))
                return 3;
            if (val < Pow10(15))
                return 4;
            if (val < Pow10(18))
                return 5;

            return 6;
        }
    }
}

#endif/*H_FINCORE_MISC*/
