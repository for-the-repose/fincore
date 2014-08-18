/*__ GPL 3.0, 2014 Alexander Soloviev (no.friday@yandex.ru) */

#ifndef H_FINCORE_MONIT
#define H_FINCORE_MONIT

#include <unistd.h>
#include <thread>
#include <chrono>
#include <string>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <cstring>

#include "probe.h"
#include "diff.h"
#include "print.h"


class Monit {
public:
    class Cfg {
    public:
        unsigned    delay   = 0;
        size_t      count   = 1;
        float       thresh  = 0.1;
    };

    Monit(const Cfg &cfg_) : cfg(cfg_) { }

    void Do(std::string &path)
    {
        Probe probe;

        const std::chrono::seconds duration(cfg.delay);

        Stats::Bands::Ref  was;

        for(unsigned count = cfg.count; count > 0; count--) {
            OS::File  file;

            try {
                file = OS::File(path);

            } catch (Error &error) {
                std::cerr << error.what() << std::endl;

                break;
            }

            auto map = file.mmap();

            Stats::Bands::Ref now(new Stats::Bands(map, 48));

            probe(map, [&](Utils::Span &span) { (*now)(span); });

            if (!was.get() || Stats::Diff()(*was, *now) > cfg.thresh) {
                was.reset(now.release());

                std::cout << Stamp() << " " << Stats::Print(*was) << std::endl;
            }

            if (count > 1)
                std::this_thread::sleep_for(duration);
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
