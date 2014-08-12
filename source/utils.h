#ifndef H_FINCORE_UTILS
#define H_FINCORE_UTILS

#include <algorithm>

namespace Utils {

    template<typename Val> Val diff(const Val &one, const Val &two)
    {
        return std::max(one, two) - std::min(one, two);
    }
}

#endif/*H_FINCORE_UTILS*/
