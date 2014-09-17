/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_MONIT
#define H_FINCORE_MONIT

#include <unistd.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <cstring>

#include "probe.h"
#include "diff.h"
#include "print.h"
#include "ticks.h"
#include "parts.h"

class Monit {
public:
    using Ticks = Utils::Ticks<>;
    using Sampled = Stats::Parted<Parts::Tailed>;

    class Cfg {
    public:
        unsigned    delay   = 0;
        size_t      count   = 1;
        float       thresh  = 0.1;
        unsigned    bands   = 48;
        unsigned    subs    = 8192;
    };

    Monit(const Cfg &cfg_) : cfg(cfg_) { }

    void Do(std::string &path)
    {
        Probe probe;
        Parts::Scale scale(cfg.subs);

        Sampled::Ref  was;

        for (Ticks ti(cfg.delay * 1000, cfg.count); ti();) {
            OS::File  file;

            try {
                file = OS::File(path);

            } catch (Error &error) {
                std::cerr << error.what() << std::endl;

                break;
            }

            auto map = file.mmap();

            const size_t bytes = ((const OS::MemRg&)map).paged();

            Sampled::Ref now(new Sampled(bytes, scale(bytes)));

            probe(map, [&](Utils::Span &span) { (*now)(span); });

            if (!was || Stats::Diff()(*was, *now) > cfg.thresh) {
                was.reset(now.release());

                std::cout
                    << Stamp()
                    << " "
                    << Stats::Print(*was, cfg.bands)
                    << std::endl;
            }
        }
    }

    std::string Stamp() const noexcept
    {
        using namespace std;

        time_t now = time(nullptr);
        tm parts;

        memset(&parts, 0, sizeof(parts));

        localtime_r(&now, &parts);
        
        char line[64];

        strftime(line, sizeof(line), "%m-%d %H:%M:%S", &parts);

        return line;
    }

protected:
    const Cfg       &cfg;
};

#endif/*H_FINCORE_MONIT*/
